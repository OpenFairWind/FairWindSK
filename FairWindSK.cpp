//
// Created by Raffaele Montella on 03/04/21.
//
#include <QApplication>
#include <QThread>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QJsonArray>

#include <QLoggingCategory>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <utility>

using namespace Qt::StringLiterals;

namespace fairwindsk {
/*
 * FairWind
 * Private constructor - called by getInstance in order to ensure
 * the singleton design pattern
 */
    FairWindSK::FairWindSK() {
        qDebug() << "FairWindSK constructor";

        m_debug = true;
        m_mSleep = 2500;
        m_nRetry = 10;
        m_signalKServerUrl = "";
        m_token = "";
        m_username = "admin";
        m_password = "password";

        QString text = " {"
                       "  \"signalk\": {"
                       "    \"pos\": \"navigation.position\","
                       "    \"cog\": \"navigation.courseOverGroundTrue\","
                       "    \"sog\": \"navigation.speedOverGround\","
                       "    \"hdg\": \"navigation.headingTrue\","
                       "    \"stw\": \"navigation.speedThroughWater\","
                       "    \"dpt\": \"environment.depth.belowTransducer\","
                       "    \"wpt\": \"navigation.course.nextPoint\","
                       "    \"btw\": \"navigation.course.calcValues.bearingTrue\","
                       "    \"dtg\": \"navigation.course.calcValues.distance\","
                       "    \"ttg\": \"navigation.course.calcValues.timeToGo\","
                       "    \"eta\": \"navigation.course.calcValues.estimatedTimeOfArrival\","
                       "    \"xte\": \"navigation.course.calcValues.crossTrackError\","
                       "    \"vmg\": \"performance.velocityMadeGood\""
                       "  },"
                       "  \"bottomBarApps\": {"
                       "    \"mydata\": \"\","
                       "    \"mob\": \"\","
                       "    \"alarms\": \"\","
                       "    \"settings\": \"admin\""
                       "  },"
                       "  \"units\": {"
                       "    \"vesselSpeed\": \"kn\","
                       "    \"windSpeed\": \"kn\","
                       "    \"distance\": \"nm\","
                       "    \"depth\": \"m\""
                       "  }"
                       "}";

        m_configuration = QJsonDocument::fromJson(text.toUtf8()).object();

    }



/*
 * getInstance
 * Either returns the available instance or creates a new one
 */
    FairWindSK *FairWindSK::getInstance() {
        if (m_instance == nullptr) {
            m_instance = new FairWindSK();
        }
        return m_instance;
    }

    void FairWindSK::loadConfig() {

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        m_debug = settings.value("debug", m_debug).toBool();

        // Store the configuration in the settings
        settings.setValue("debug", m_debug);

        if (m_debug) {
            qDebug() << "Loading configuration from ini file...";
        }

        // Get the name of the FairWindSK configuration file
        m_signalKServerUrl = settings.value("signalk-server", m_signalKServerUrl).toString();

        // Store the configuration in the settings
        settings.setValue("signalk-server", m_signalKServerUrl);

        // Get the token of the FairWindSK configuration file
        m_token = settings.value("token", m_token).toString();

        // Store the configuration in the settings
        settings.setValue("token", m_token);

        // Get the name of the FairWind++ configuration file
        m_mSleep = settings.value("sleep", m_mSleep).toInt();

        // Store the configuration in the settings
        settings.setValue("sleep", m_mSleep);

        // Get the name of the FairWind++ configuration file
        m_nRetry = settings.value("retry", m_nRetry).toInt();

        // Store the configuration in the settings
        settings.setValue("retry", m_nRetry);

        // Get the name of the FairWind++ configuration file
        m_configuration = QJsonDocument::fromJson(
                settings.value("configuration",
                               QJsonDocument(m_configuration).toJson()
                               ).toString().toUtf8()
                ).object();

        // Store the configuration in the settings
        settings.setValue("configuration",
                          QJsonDocument(m_configuration).toJson()
                          );


        if (m_debug) {

            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);

            qDebug() << "Configration from ini file loaded.";
        }

    }

    bool FairWindSK::startSignalK() {
        bool result = false;

        // Check if the Signal K Server url is defined
        if (!m_signalKServerUrl.isEmpty()) {

            // Define the parameters map
            QMap<QString, QVariant> params;

            // Set some defaults
            params["active"] = true;


            // Setup the debug mode
            params["debug"] = m_debug;

            // Set the url
            params["url"] = m_signalKServerUrl + "/signalk";

            // Check if the token is defined or if username/password are defined
            if (!m_token.isEmpty() || (!m_username.isEmpty() && !m_password.isEmpty())) {

                // Check if the token is defined
                if (!m_token.isEmpty()) {

                    // Set the token
                    params["token"] = m_token;
                } else {

                    // Set username and password
                    params["username"] = m_username;
                    params["password"] = m_password;
                }


                // Number of connection tentatives
                int count = 1;

                // Start the connection
                do {

                    if (m_debug) {
                        qDebug() << "Trying to connect to the " << m_signalKServerUrl << " Signal K server (" << count
                                 << "/" << m_nRetry << ")...";
                    }

                    // Try to connect
                    result = m_signalkClient.init(params);

                    // Check if the connection is successful
                    if (result) {

                        // Set the token
                        setToken(m_signalkClient.getToken());

                        // Get all the document
                        QJsonObject allSignalK = m_signalkClient.getAll();

                        // Update the local document
                        m_signalkDocument.insert("", allSignalK);

                        // Check if the debug is active
                        if (m_debug) {

                            // Printout the document
                            qDebug() << allSignalK;
                        }

                        // Exit the loop
                        break;
                    }

                    // Process the events
                    QApplication::processEvents();

                    // Increase the number of retry
                    count++;

                    // Wait for m_mSleep microseconds
                    QThread::msleep(m_mSleep);

                    // Loop until the number of retry
                } while (count < m_nRetry);

                if (m_debug) {
                    if (result) {
                        qDebug() << "Connected to " << m_signalKServerUrl;
                    } else {
                        qDebug() << "No response from the " << m_signalKServerUrl << " Signal K server!";
                    }
                }
            }
        }

        // Return the result
        return result;
    }

    bool FairWindSK::loadApps() {

        bool result = false;

        // Remove all app items
        for (const auto& key: m_mapHash2AppItem.keys()) {
            delete m_mapHash2AppItem[key];
        }

        // Remove the map content
        m_mapHash2AppItem.empty();

        QString settingsString = R"({ "name": "admin" })";
        QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());

        if (m_debug) {
            qDebug() << settingsJsonDocument;
        }

        auto settingsAppItem = new AppItem(settingsJsonDocument.object(), false, 0);
        m_mapHash2AppItem[settingsAppItem->getName()] = settingsAppItem;

        if (m_debug) {
            qDebug() << "name:" << settingsAppItem->getName() << "url:" << settingsAppItem->getUrl();
        }

        QString appstoreString = R"({"name": "admin/#/appstore/apps"})";
        QJsonDocument appstoreJsonDocument = QJsonDocument::fromJson(appstoreString.toUtf8());
        auto appstoreAppItem = new AppItem(appstoreJsonDocument.object(), false, 0);
        m_mapHash2AppItem[appstoreAppItem->getName()] = appstoreAppItem;

        QUrl url = QUrl(m_signalKServerUrl + "/skServer/webapps");
        QNetworkAccessManager networkAccessManager;
        QEventLoop loop;
        connect(&networkAccessManager, &QNetworkAccessManager::finished, &loop,&QEventLoop::quit);
        QNetworkReply *reply = networkAccessManager.get(QNetworkRequest(url));
        loop.exec();

        if (reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 200) {

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());

            if (m_debug) {
                qDebug() << doc;
            }

            // Reset the counter
            int count = 0;

            // Get the application array
            auto appsJsonArray = doc.array();

            // For each item of the array...
            for (auto appJsonItem: appsJsonArray) {

                // Check if the item is an object
                if (appJsonItem.isObject()) {

                    // Get the json object
                    auto appJsonObject = appJsonItem.toObject();

                    // Check if the app is a signalk-webapp
                    if (appJsonObject.contains("keywords") &&
                        appJsonObject["keywords"].isArray() &&
                        appJsonObject["keywords"].toArray().contains("signalk-webapp")) {

                        // Create an app item object
                        auto appItem = new AppItem(appJsonObject, true, count);

                        // Add the item to the lookup table
                        m_mapHash2AppItem[appItem->getName()] = appItem;

                        // Increase the app counter
                        count++;
                    }
                }
            }
            result = true;
        } else {
            if (m_debug) {
                qDebug() << "Helper plugin not installed on " << m_signalKServerUrl;
            }
        }

        return result;
    }

    QList<QString> FairWindSK::getAppsHashes() {
        return m_mapHash2AppItem.keys();
    }

    /*
 * getSignalKDocument
 * Returns the SignalK document
 */
    signalk::Document *FairWindSK::getSignalKDocument() {
        return &m_signalkDocument;
    }

    AppItem *FairWindSK::getAppItemByHash(QString hash) {
        return m_mapHash2AppItem[hash];
    }

    QString FairWindSK::getAppHashById(QString appId) {
        return m_mapAppId2Hash[appId];
    }

    QString FairWindSK::getSignalKServerUrl(){
        return m_signalKServerUrl;
    }

    bool FairWindSK::isDebug() const {
        return m_debug;
    }

    /*
 * getConfig
 * Returns the configuration infos
 */
    QJsonObject FairWindSK::getConfiguration() {

        // Return the result
        return m_configuration;
    }

    QString FairWindSK::getAppNameByKeyFromConfiguration(const QString& key) {
        QString result = "";

        auto configuration = getConfiguration();
        if (configuration.contains("bottomBarApps") && configuration["bottomBarApps"].isObject()) {
            auto apps = configuration["bottomBarApps"].toObject();

            if (apps.contains(key) && apps[key].isString()) {
                result = apps[key].toString();
            }
        }

        return result;
    }


    QString FairWindSK::getMyDataApp() {
        return getAppNameByKeyFromConfiguration("mydata");
    }

    QString FairWindSK::getMOBApp() {
        return getAppNameByKeyFromConfiguration("mob");
    }

    QString FairWindSK::getAlarmsApp() {
        return getAppNameByKeyFromConfiguration("alarms");
    }

    QString FairWindSK::getSettingsApp() {
        return getAppNameByKeyFromConfiguration("settings");
    }

    SignalKClient *FairWindSK::getSignalKClient() {
        return &m_signalkClient;
    }

    QString FairWindSK::getToken() {
        return m_token;
    }

    void FairWindSK::setToken(QString token) {
        m_token = std::move(token);

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Store the configuration in the settings
        settings.setValue("token", m_token);
    }

    int FairWindSK::getSleep() {
        return m_mSleep;
    }

    int FairWindSK::getRetry() {
        return m_nRetry;
    }

    void FairWindSK::setSignalKServerUrl(QString signalKServerUrl) {
        m_signalKServerUrl = std::move(signalKServerUrl);

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Store the configuration in the settings
        settings.setValue("signalk-server", m_signalKServerUrl);
    }

    void FairWindSK::setVirtualKeyboard(bool value) {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Store the configuration in the settings
        settings.setValue("virtualKeyboard", value);
    }

    bool FairWindSK::getVirtualKeyboard() {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        auto value = settings.value("virtualKeyboard", false).toBool();


        return value;
    }

    QString FairWindSK::getVesselSpeedUnits() {
        return m_configuration["units"].toObject()["vesselSpeed"].toString();
    }

    QString FairWindSK::getDepthUnits() {
        return m_configuration["units"].toObject()["depth"].toString();
    }

    QString FairWindSK::getWindSpeedUnits() {
        return m_configuration["units"].toObject()["windSpeed"].toString();
    }

    QString FairWindSK::getDistanceUnits() {
        return m_configuration["units"].toObject()["distance"].toString();
    }


}

