//
// Created by Raffaele Montella on 09/05/24.
//

#include "Configuration.hpp"

#include <utility>
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <fstream>
#include <QSettings>

namespace fairwindsk {
    Configuration::Configuration() = default;

    Configuration::Configuration(const Configuration &configuration) {
        m_jsonData = configuration.m_jsonData;
    }

    Configuration::Configuration(const QString& filename) {
        load(filename);
    }

    Configuration::~Configuration() = default;

    void Configuration::save(const QString& filename) {

        std::ofstream file(filename.toUtf8());
        file << m_jsonData;

    }

    bool Configuration::load(const QString& filename) {
        bool result = false;

        std::ifstream fileCheck(filename.toUtf8());
        if (fileCheck) {
            if (nlohmann::json::accept(fileCheck)) {
                std::ifstream file(filename.toUtf8());
                m_jsonData = nlohmann::json::parse(file);
                result = true;
            }
        }

        return result;
    }

    nlohmann::json &Configuration::getRoot() {
        return m_jsonData;
    }

    QString Configuration::getSignalKServerUrl(){

        QString signalKServerUrl = "";

        if (m_jsonData.contains("connection") && m_jsonData["connection"].is_object()) {
            auto connectionJsonObject = m_jsonData["connection"];
            if (connectionJsonObject.contains("server") & connectionJsonObject["server"].is_string()) {
                signalKServerUrl = QString::fromStdString(connectionJsonObject["server"].get<std::string>());
            }
        }
        return signalKServerUrl;
    }

    QString Configuration::getAppNameByKeyFromConfiguration(const QString& key) {
        std::string result;


        if (m_jsonData.contains("bottomBarApps") && m_jsonData["bottomBarApps"].is_object()) {
            auto apps = m_jsonData["bottomBarApps"];

            if (apps.contains(key.toStdString()) && apps[key.toStdString()].is_string()) {
                result = apps[key.toStdString()].get<std::string>();
            }
        }

        return QString::fromStdString(result);
    }


    QString Configuration::getMyDataApp() {
        return getAppNameByKeyFromConfiguration("mydata");
    }

    QString Configuration::getMOBApp() {
        return getAppNameByKeyFromConfiguration("mob");
    }

    QString Configuration::getAlarmsApp() {
        return getAppNameByKeyFromConfiguration("alarms");
    }

    QString Configuration::getSettingsApp() {
        return getAppNameByKeyFromConfiguration("settings");
    }



    QString Configuration::getToken() {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        auto token = settings.value("token", "").toString();

        return token;
    }

    void Configuration::setToken(const QString& token) {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Set the token
        settings.setValue("token", token);
    }

    int Configuration::getSleep() {
        int mSleep = 0;

        if (m_jsonData.contains("connection")) {
            auto connectionJsonObject = m_jsonData["connection"];
            if (connectionJsonObject.contains("sleep") & connectionJsonObject["sleep"].is_string()) {
                mSleep = connectionJsonObject["sleep"].get<int>();
            }
        }

        return mSleep;
    }

    int Configuration::getRetry() {
        int nRetry = 1;

        if (m_jsonData.contains("connection")) {
            auto connectionJsonObject = m_jsonData["connection"];
            if (connectionJsonObject.contains("retry") & connectionJsonObject["retry"].is_string()) {
                nRetry = connectionJsonObject["retry"].get<int>();
            }
        }

        return nRetry;
    }

    void Configuration::setSignalKServerUrl(const QString& signalKServerUrl) {

        if (m_jsonData.contains("connection")) {
            m_jsonData["connection"]["server"] = signalKServerUrl.toStdString();
        }
    }

    void Configuration::setVirtualKeyboard(bool value) {
        if (m_jsonData.contains("main")) {
            m_jsonData["main"]["virtualKeyboard"] = value;
        }
    }

    bool Configuration::getVirtualKeyboard() {
        bool result = false;

        if (m_jsonData.contains("main")) {
            auto mainJsonObject = m_jsonData["main"];
            if (mainJsonObject.contains("virtualKeyboard") & mainJsonObject["virtualKeyboard"].is_boolean()) {
                result = mainJsonObject["virtualKeyboard"].get<bool>();
            }
        }

        return result;
    }

    QString Configuration::getUnits(const QString &units) {
        std::string result;

        if (m_jsonData.contains("units")) {
            auto unitsJsonObject = m_jsonData["units"];
            if (unitsJsonObject.contains("vesselSpeed") & unitsJsonObject["vesselSpeed"].is_string()) {
                result = unitsJsonObject["vesselSpeed"].get<std::string>();
            }
        }

        return QString::fromStdString(result);
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

    void Configuration::setRoot(nlohmann::json &jsonData) {
        m_jsonData = jsonData;
    }

    void Configuration::setFilename(QString filename) {
        m_filename = filename;
    }

    QString Configuration::getFilename() {
        return m_filename;
    }

    void Configuration::save() {
        save(m_filename);
    }

    bool Configuration::load() {
        return load(m_filename);
    }


} // fairwindsk