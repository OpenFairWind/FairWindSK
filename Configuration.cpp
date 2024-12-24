//
// Created by Raffaele Montella on 09/05/24.
//

#include "Configuration.hpp"

#include <fstream>
#include <iostream>

#include <utility>
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QSettings>

namespace fairwindsk {
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
        load();
    }

    Configuration::~Configuration() = default;

    void Configuration::save() {
        save(m_filename);
    }

    void Configuration::setDefault() {
        QString data;
        QString fileName(":/resources/json/configuration.json");

        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly)) {

            data = file.readAll();
            m_jsonData= nlohmann::json::parse(data.toStdString());
        }

        file.close();
    }

    void Configuration::save(const QString& filename) {

        std::ofstream file(filename.toUtf8());
        file << m_jsonData.dump(2);

    }

    bool Configuration::load() {
        return load(m_filename);
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

    void Configuration::setSignalKServerUrl(const QString& signalKServerUrl) {

        if (m_jsonData.contains("connection")) {
            m_jsonData["connection"]["server"] = signalKServerUrl.toStdString();
        }
    }

    QString Configuration::getAutopilotApp() {
        QString result = "";
        if (
                m_jsonData.contains("applications") &&
                m_jsonData["applications"].contains("autopilot") &&
                m_jsonData["applications"]["autopilot"].is_string()) {
            result = QString::fromStdString(m_jsonData["applications"]["autopilot"].get<std::string>());
        }
        return result;
    }

    QString Configuration::getAnchorApp() {
        QString result = "";
        if (
                m_jsonData.contains("applications") &&
                m_jsonData["applications"].contains("anchor") &&
                m_jsonData["applications"]["anchor"].is_string()) {
            result = QString::fromStdString(m_jsonData["applications"]["anchor"].get<std::string>());
        }
        return result;
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
            auto unitsKey = units.toStdString();
            if (unitsJsonObject.contains(unitsKey) & unitsJsonObject[unitsKey].is_string()) {
                result = unitsJsonObject[unitsKey].get<std::string>();
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

    QString Configuration::getRangeUnits() {
        return getUnits("range");
    }

    void Configuration::setRoot(nlohmann::json &jsonData) {
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


} // fairwindsk