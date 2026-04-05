//
// Created by Raffaele Montella on 09/05/24.
//

#include "Configuration.hpp"

#include <algorithm>
#include <iostream>

#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QSettings>
#include <QByteArray>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>

namespace fairwindsk {
    namespace {
        QString defaultSettingsFilename() {
            QString settingsDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
            if (settingsDir.trimmed().isEmpty()) {
                settingsDir = QDir::homePath();
            }
            QDir().mkpath(settingsDir);
            return QDir(settingsDir).filePath(QStringLiteral("fairwindsk.ini"));
        }
    }

    Configuration::Configuration() {
        m_filename = "configuration.json";
        setDefault();
    };

    Configuration::Configuration(const Configuration &configuration) {
        m_jsonData = configuration.m_jsonData;
        m_filename = configuration.m_filename;
    }

    Configuration::Configuration(const QString& filename) {
        m_filename = filename;
        if (!load()) {
            setDefault();
        }
    }



    void Configuration::save() {
        save(m_filename);
    }

    void Configuration::setDefault() {
        const QString fileName(":/resources/json/configuration.json");

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            try {
                const auto data = file.readAll();
                m_jsonData = nlohmann::json::parse(data.constBegin(), data.constEnd());
            } catch (const nlohmann::json::parse_error &) {
                m_jsonData = nlohmann::json::object();
            }
        } else {
            m_jsonData = nlohmann::json::object();
        }

        file.close();
    }

    void Configuration::save(const QString& filename) {
        const QFileInfo fileInfo(filename);
        if (!fileInfo.absolutePath().isEmpty()) {
            QDir().mkpath(fileInfo.absolutePath());
        }

        QSaveFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            const QByteArray data = QByteArray::fromStdString(m_jsonData.dump(2));
            file.write(data);
            file.commit();
        }
    }

    bool Configuration::load() {
        return load(m_filename);
    }

    bool Configuration::load(const QString& filename) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        const QByteArray data = file.readAll();
        file.close();

        try {
            m_jsonData = nlohmann::json::parse(data.constBegin(), data.constEnd());
            if (!m_jsonData.is_object()) {
                setDefault();
                return false;
            }
            return true;
        } catch (const nlohmann::json::parse_error &) {
            setDefault();
            return false;
        }
    }

    nlohmann::json &Configuration::getRoot() {
        return m_jsonData;
    }

    QString Configuration::getSignalKServerUrl(){
        return getString("connection", "server");
    }

    QString Configuration::getSignalKPath(const QString &key) const {
        return getString("signalk", key, QString());
    }

    QString Configuration::getToken() {
        // Initialize the QT managed settings
        QSettings settings(settingsFilename(), QSettings::IniFormat);

        // Get the name of the FairWind++ configuration file
        auto token = settings.value("token", "").toString();

        return token;
    }

    void Configuration::setToken(const QString& token) {
        // Initialize the QT managed settings
        QSettings settings(settingsFilename(), QSettings::IniFormat);

        // Set the token
        settings.setValue("token", token);
        settings.sync();
    }

    QString Configuration::settingsFilename() {
        return defaultSettingsFilename();
    }

    void Configuration::setSignalKServerUrl(const QString& signalKServerUrl) {
        ensureObject("connection")["server"] = signalKServerUrl.toStdString();
    }

    QString Configuration::getAutopilotApp() {
        return getString("applications", "autopilot");
    }

    QString Configuration::getAnchorApp() {
        return getString("applications", "anchor");
    }

    void Configuration::setVirtualKeyboard(bool value) {
        ensureObject("main")["virtualKeyboard"] = value;
    }



    bool Configuration::getVirtualKeyboard() {
        return getBool("main", "virtualKeyboard");
    }

    void Configuration::setUiScaleMode(const QString &value) {
        ensureObject("main")["uiScaleMode"] = value.toStdString();
    }

    QString Configuration::getUiScaleMode() const {
        return getString("main", "uiScaleMode", "auto");
    }

    void Configuration::setUiScalePreset(const QString &value) {
        ensureObject("main")["uiScalePreset"] = value.toStdString();
    }

    QString Configuration::getUiScalePreset() const {
        return getString("main", "uiScalePreset", "normal");
    }

    void Configuration::setComfortViewMode(const QString &value) {
        ensureObject("main")["comfortViewMode"] = value.toStdString();
    }

    QString Configuration::getComfortViewMode() const {
        return getString("main", "comfortViewMode", "manual");
    }

    void Configuration::setComfortViewPreset(const QString &value) {
        ensureObject("main")["comfortViewPreset"] = value.toStdString();
    }

    QString Configuration::getComfortViewPreset() const {
        return getString("main", "comfortViewPreset", "day");
    }

    void Configuration::setComfortThemeStyleSheet(const QString &preset, const QString &styleSheet) {
        const QString normalizedPreset = preset.trimmed().toLower();
        if (normalizedPreset.isEmpty()) {
            return;
        }

        ensureObject("comfortViewThemes")[normalizedPreset.toStdString()] = styleSheet.toStdString();
    }

    QString Configuration::getComfortThemeStyleSheet(const QString &preset) const {
        return getString("comfortViewThemes", preset.trimmed().toLower(), QString());
    }

    void Configuration::clearComfortThemeStyleSheet(const QString &preset) {
        const QString normalizedPreset = preset.trimmed().toLower();
        if (normalizedPreset.isEmpty() || !m_jsonData.contains("comfortViewThemes") || !m_jsonData["comfortViewThemes"].is_object()) {
            return;
        }

        m_jsonData["comfortViewThemes"].erase(normalizedPreset.toStdString());
    }

    void Configuration::setLauncherRows(const int value) {
        ensureObject("main")["launcherRows"] = std::max(1, value);
    }

    int Configuration::getLauncherRows() const {
        return getInt("main", "launcherRows", 2);
    }

    void Configuration::setLauncherColumns(const int value) {
        ensureObject("main")["launcherColumns"] = std::max(1, value);
    }

    int Configuration::getLauncherColumns() const {
        return getInt("main", "launcherColumns", 4);
    }

    void Configuration::setCoordinateFormat(const QString &value) {
        ensureObject("main")["coordinateFormat"] = value.toStdString();
    }

    QString Configuration::getCoordinateFormat() const {
        return getString("main", "coordinateFormat", "dm_symbols_decimal");
    }

    void Configuration::setWindowMode(QString value) {
        ensureObject("main")["windowMode"] = value.toStdString();
    }

    QString Configuration::getWindowMode() {
        return getString("main", "windowMode", "windowed");
    }

    int Configuration::getWindowWidth() {
        return getInt("main", "windowWidth");
    }

    void Configuration::setWindowWidth(int value) {
        ensureObject("main")["windowWidth"] = value;
    }

    int Configuration::getWindowHeight() {
        return getInt("main", "windowHeight");
    }

    void Configuration::setWindowHeight(int value) {
        ensureObject("main")["windowHeight"] = value;
    }

    int Configuration::getWindowLeft() {
        return getInt("main", "windowLeft");
    }

    void Configuration::setWindowLeft(int value) {
        ensureObject("main")["windowLeft"] = value;
    }

    int Configuration::getWindowTop() {
        return getInt("main", "windowTop");
    }

    void Configuration::setWindowTop(int value) {
        ensureObject("main")["windowTop"] = value;
    }





    void Configuration::setAutopilot(const QString& value) {
        ensureObject("main")["autopilot"] = value.toStdString();
    }

    QString Configuration::getAutopilot() {
        return getString("main", "autopilot");
    }

    QString Configuration::getUnits(const QString &units) const {
        return getString("units", units);
    }

    QString Configuration::getVesselSpeedUnits() {
        return getUnits("vesselSpeed");
    }

    QString Configuration::getDepthUnits() {
        return getUnits("depth");
    }

    QString Configuration::getWindSpeedUnits() {
        return getUnits("windSpeed");
    }

    QString Configuration::getDistanceUnits() {
        return getUnits("distance");
    }

    QString Configuration::getRangeUnits() {
        return getUnits("range");
    }

    void Configuration::setRoot(const nlohmann::json &jsonData) {
        m_jsonData = jsonData;
    }

    void Configuration::setFilename(QString filename) {
        m_filename = std::move(filename);
    }

    QString Configuration::getFilename() {
        return m_filename;
    }

    int Configuration::findApp(const QString& name) {
        int result = -1;

        if (m_jsonData.contains("apps") && m_jsonData["apps"].is_array()) {
            auto appsJsonArray = m_jsonData["apps"];

            int idx=0;
            for (auto app: appsJsonArray) {
                if (app.is_object()) {

                    if (app.contains("name") && app["name"].is_string()) {
                        auto appName = QString::fromStdString(app["name"].get<std::string>());
                        if (appName == name) {
                            result = idx;
                            break;
                        }
                    }
                    idx++;
                }
            }
        }

        return result;
    }

    nlohmann::json &Configuration::ensureObject(const char *key) {
        if (!m_jsonData.is_object()) {
            m_jsonData = nlohmann::json::object();
        }
        if (!m_jsonData.contains(key) || !m_jsonData[key].is_object()) {
            m_jsonData[key] = nlohmann::json::object();
        }
        return m_jsonData[key];
    }

    QString Configuration::getString(const char *section, const char *key, const QString &defaultValue) const {
        if (m_jsonData.contains(section) && m_jsonData[section].is_object()) {
            const auto &jsonSection = m_jsonData[section];
            if (jsonSection.contains(key) && jsonSection[key].is_string()) {
                return QString::fromStdString(jsonSection[key].get<std::string>());
            }
        }
        return defaultValue;
    }

    QString Configuration::getString(const char *section, const QString &key, const QString &defaultValue) const {
        return getString(section, key.toUtf8().constData(), defaultValue);
    }

    int Configuration::getInt(const char *section, const char *key, const int defaultValue) const {
        if (m_jsonData.contains(section) && m_jsonData[section].is_object()) {
            const auto &jsonSection = m_jsonData[section];
            if (jsonSection.contains(key) && jsonSection[key].is_number_integer()) {
                return jsonSection[key].get<int>();
            }
        }
        return defaultValue;
    }

    bool Configuration::getBool(const char *section, const char *key, const bool defaultValue) const {
        if (m_jsonData.contains(section) && m_jsonData[section].is_object()) {
            const auto &jsonSection = m_jsonData[section];
            if (jsonSection.contains(key) && jsonSection[key].is_boolean()) {
                return jsonSection[key].get<bool>();
            }
        }
        return defaultValue;
    }

    Configuration::~Configuration() = default;
} // fairwindsk
