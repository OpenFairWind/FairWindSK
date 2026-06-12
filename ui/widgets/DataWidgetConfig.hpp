#ifndef FAIRWINDSK_UI_WIDGETS_DATAWIDGETCONFIG_HPP
#define FAIRWINDSK_UI_WIDGETS_DATAWIDGETCONFIG_HPP

#include <QList>
#include <QString>
#include <nlohmann/json.hpp>

namespace fairwindsk::ui::widgets {

    enum class DataWidgetKind {
        Numeric,
        Gauge,
        Position,
        DateTime,
        Waypoint
    };

    struct DataWidgetDefinition {
        QString id;
        QString name;
        QString icon;
        QString signalKPath;
        QString sourceUnit;
        QString defaultUnit;
        QString updatePolicy = QStringLiteral("instant");
        QString dateTimeFormat;
        DataWidgetKind kind = DataWidgetKind::Numeric;
        int period = 1000;
        int minPeriod = 200;
        double minimum = 0.0;
        double maximum = 100.0;
        bool defaultTopEnabled = false;
        bool defaultBottomEnabled = false;
        bool expandHorizontally = false;
        bool expandVertically = false;
    };

    QString dataWidgetConfigKey();
    QString dataWidgetKindId(DataWidgetKind kind);
    QString dataWidgetKindLabel(DataWidgetKind kind);
    DataWidgetKind dataWidgetKindFromId(const QString &id);
    QList<DataWidgetDefinition> dataWidgetDefinitions(const nlohmann::json &root);
    DataWidgetDefinition dataWidgetDefinition(const nlohmann::json &root, const QString &id);
    bool isDataWidgetId(const nlohmann::json &root, const QString &id);
    QString uniqueDataWidgetId(const nlohmann::json &root, const QString &name);
    nlohmann::json dataWidgetDefinitionToJson(const DataWidgetDefinition &definition);
    void setDataWidgetDefinitions(nlohmann::json &root, const QList<DataWidgetDefinition> &definitions);
    bool upsertDataWidgetDefinition(nlohmann::json &root, const DataWidgetDefinition &definition);
    bool removeDataWidgetDefinition(nlohmann::json &root, const QString &id);
}

#endif // FAIRWINDSK_UI_WIDGETS_DATAWIDGETCONFIG_HPP
