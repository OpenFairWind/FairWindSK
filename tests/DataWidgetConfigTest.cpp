#include "ui/widgets/DataWidgetConfig.hpp"

#include <QtTest>

using fairwindsk::ui::widgets::DataWidgetDefinition;
using fairwindsk::ui::widgets::DataWidgetKind;
using fairwindsk::ui::widgets::DataWidgetVisualizationMode;

class DataWidgetConfigTest final : public QObject {
    Q_OBJECT

private slots:
    void providesLegacyDefaults();
    void normalizesStoredDefinitions();
    void createsUniqueIdentifiers();
    void upsertsAndRemovesDefinitions();
    void serializesDefinition();
};

void DataWidgetConfigTest::providesLegacyDefaults() {
    const nlohmann::json root = nlohmann::json::object();
    const auto definitions = fairwindsk::ui::widgets::dataWidgetDefinitions(root);

    QVERIFY(definitions.size() >= 10);
    QVERIFY(fairwindsk::ui::widgets::isDataWidgetId(root, QStringLiteral("position")));
    const auto speed = fairwindsk::ui::widgets::dataWidgetDefinition(root, QStringLiteral("sog"));
    QCOMPARE(speed.signalKPath, QStringLiteral("navigation.speedOverGround"));
    QCOMPARE(speed.sourceUnit, QStringLiteral("ms-1"));
    QCOMPARE(speed.defaultUnit, QStringLiteral("kn"));
}

void DataWidgetConfigTest::normalizesStoredDefinitions() {
    nlohmann::json root;
    root["dataWidgets"] = nlohmann::json::array({
        {{"name", "Engine RPM"}, {"type", "gauge"}, {"updatePolicy", "FIXED"},
         {"period", 25}, {"minPeriod", -4}, {"minimum", 100.0}, {"maximum", 50.0},
         {"valueTextColor", " red "}, {"valueTextSize", 200}},
        {{"id", "duplicate"}, {"name", "First"}},
        {{"id", "duplicate"}, {"name", "Second"}}
    });

    const auto definitions = fairwindsk::ui::widgets::dataWidgetDefinitions(root);
    QCOMPARE(definitions.size(), 2);
    const auto engine = definitions.first();
    QCOMPARE(engine.id, QStringLiteral("engine_rpm"));
    QCOMPARE(engine.kind, DataWidgetKind::Gauge);
    QCOMPARE(engine.visualizationMode, DataWidgetVisualizationMode::Gauge);
    QCOMPARE(engine.updatePolicy, QStringLiteral("fixed"));
    QCOMPARE(engine.period, 100);
    QCOMPARE(engine.minPeriod, 0);
    QCOMPARE(engine.maximum, 200.0);
    QCOMPARE(engine.valueTextColor, QStringLiteral("#ff0000"));
    QCOMPARE(engine.valueTextSize, 48);
}

void DataWidgetConfigTest::createsUniqueIdentifiers() {
    nlohmann::json root;
    root["dataWidgets"] = nlohmann::json::array({
        {{"id", "engine_rpm"}, {"name", "Existing"}},
        {{"id", "engine_rpm_2"}, {"name", "Existing 2"}}
    });

    QCOMPARE(fairwindsk::ui::widgets::uniqueDataWidgetId(root, QStringLiteral(" Engine RPM ")),
             QStringLiteral("engine_rpm_3"));
    QCOMPARE(fairwindsk::ui::widgets::uniqueDataWidgetId(root, QStringLiteral("***")),
             QStringLiteral("data_widget"));
}

void DataWidgetConfigTest::upsertsAndRemovesDefinitions() {
    nlohmann::json root;
    root["dataWidgets"] = nlohmann::json::array();
    DataWidgetDefinition definition;
    definition.id = QStringLiteral("battery_voltage");
    definition.name = QStringLiteral("Battery Voltage");
    definition.signalKPath = QStringLiteral("electrical.batteries.house.voltage");

    QVERIFY(fairwindsk::ui::widgets::upsertDataWidgetDefinition(root, definition));
    QCOMPARE(fairwindsk::ui::widgets::dataWidgetDefinitions(root).size(), 1);

    definition.name = QStringLiteral("House Battery");
    QVERIFY(fairwindsk::ui::widgets::upsertDataWidgetDefinition(root, definition));
    QCOMPARE(fairwindsk::ui::widgets::dataWidgetDefinitions(root).size(), 1);
    QCOMPARE(fairwindsk::ui::widgets::dataWidgetDefinition(root, definition.id).name,
             QStringLiteral("House Battery"));

    QVERIFY(fairwindsk::ui::widgets::removeDataWidgetDefinition(root, definition.id));
    QVERIFY(fairwindsk::ui::widgets::dataWidgetDefinitions(root).isEmpty());
    QVERIFY(!fairwindsk::ui::widgets::removeDataWidgetDefinition(root, definition.id));
}

void DataWidgetConfigTest::serializesDefinition() {
    DataWidgetDefinition definition;
    definition.id = QStringLiteral("rudder");
    definition.name = QStringLiteral("Rudder Angle");
    definition.kind = DataWidgetKind::Numeric;
    definition.visualizationMode = DataWidgetVisualizationMode::Text;
    definition.updatePolicy = QStringLiteral("invalid");
    definition.valueTextColor = QStringLiteral("not-a-color");

    const auto json = fairwindsk::ui::widgets::dataWidgetDefinitionToJson(definition);
    QCOMPARE(QString::fromStdString(json["type"].get<std::string>()), QStringLiteral("numerical"));
    QCOMPARE(QString::fromStdString(json["updatePolicy"].get<std::string>()), QStringLiteral("instant"));
    QCOMPARE(QString::fromStdString(json["valueTextColor"].get<std::string>()), QString());
}

QTEST_APPLESS_MAIN(DataWidgetConfigTest)

#include "DataWidgetConfigTest.moc"
