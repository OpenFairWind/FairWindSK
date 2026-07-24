#include "Configuration.hpp"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ConfigurationTest final : public QObject {
    Q_OBJECT

private slots:
    void readsAndWritesSettings();
    void savesAndLoadsAtomically();
    void rejectsInvalidDocuments();
    void copiesIndependently();
};

void ConfigurationTest::readsAndWritesSettings() {
    fairwindsk::Configuration configuration;
    configuration.setRoot(nlohmann::json::object());

    configuration.setSignalKServerUrl(QStringLiteral("https://signalk.example:3443"));
    configuration.setSignalKConnectionEnabled(false);
    configuration.setVirtualKeyboard(true);
    configuration.setLanguage(QStringLiteral("it-IT"));
    configuration.setLauncherRows(4);
    configuration.setLauncherColumns(6);
    configuration.setCoordinateFormat(QStringLiteral("decimal_degrees"));
    configuration.setDiagnosticsLogLevel(QStringLiteral("debug"));

    QCOMPARE(configuration.getSignalKServerUrl(), QStringLiteral("https://signalk.example:3443"));
    QVERIFY(!configuration.getSignalKConnectionEnabled());
    QVERIFY(configuration.getVirtualKeyboard());
    QCOMPARE(configuration.getLanguage(), QStringLiteral("it"));
    configuration.setLanguage(QStringLiteral("fr-FR"));
    QCOMPARE(configuration.getLanguage(), QStringLiteral("fr"));
    configuration.setLanguage(QStringLiteral("es_ES"));
    QCOMPARE(configuration.getLanguage(), QStringLiteral("es"));
    QCOMPARE(configuration.getLauncherRows(), 4);
    QCOMPARE(configuration.getLauncherColumns(), 6);
    QCOMPARE(configuration.getCoordinateFormat(), QStringLiteral("decimal_degrees"));
    QCOMPARE(configuration.getDiagnosticsLogLevel(), QStringLiteral("debug"));
}

void ConfigurationTest::savesAndLoadsAtomically() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("nested/configuration.json"));

    fairwindsk::Configuration source;
    source.setRoot(nlohmann::json{{"connection", {{"server", "http://localhost:3000"}, {"active", true}}},
                                  {"main", {{"language", "en"}}}});
    source.save(path);
    QVERIFY(QFile::exists(path));

    fairwindsk::Configuration loaded(path);
    QCOMPARE(loaded.getSignalKServerUrl(), QStringLiteral("http://localhost:3000"));
    QVERIFY(loaded.getSignalKConnectionEnabled());
    QCOMPARE(loaded.getLanguage(), QStringLiteral("en"));
}

void ConfigurationTest::rejectsInvalidDocuments() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString invalidJsonPath = directory.filePath(QStringLiteral("invalid.json"));
    QFile invalidJson(invalidJsonPath);
    QVERIFY(invalidJson.open(QIODevice::WriteOnly));
    QCOMPARE(invalidJson.write("{broken"), qint64(7));
    invalidJson.close();

    fairwindsk::Configuration configuration;
    QVERIFY(!configuration.load(invalidJsonPath));
    QVERIFY(configuration.getRoot().is_object());

    const QString arrayPath = directory.filePath(QStringLiteral("array.json"));
    QFile arrayFile(arrayPath);
    QVERIFY(arrayFile.open(QIODevice::WriteOnly));
    QCOMPARE(arrayFile.write("[]"), qint64(2));
    arrayFile.close();
    QVERIFY(!configuration.load(arrayPath));
    QVERIFY(configuration.getRoot().is_object());
}

void ConfigurationTest::copiesIndependently() {
    fairwindsk::Configuration original;
    original.setRoot(nlohmann::json::object());
    original.setSignalKServerUrl(QStringLiteral("http://original"));

    fairwindsk::Configuration copy(original);
    copy.setSignalKServerUrl(QStringLiteral("http://copy"));

    QCOMPARE(original.getSignalKServerUrl(), QStringLiteral("http://original"));
    QCOMPARE(copy.getSignalKServerUrl(), QStringLiteral("http://copy"));
}

QTEST_APPLESS_MAIN(ConfigurationTest)

#include "ConfigurationTest.moc"
