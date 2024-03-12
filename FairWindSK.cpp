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
        m_signalKServerUrl = "http://172.24.1.1:3000";
        m_username = "admin";
        m_password = "password";
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

        // Get the name of the FairWind++ configuration file
        m_signalKServerUrl = settings.value("signalk-server", m_signalKServerUrl).toString();

        // Store the configuration in the settings
        settings.setValue("signalk-server", m_signalKServerUrl);

        // Get the name of the FairWind++ configuration file
        m_username = settings.value("username", m_username).toString();

        // Store the configuration in the settings
        settings.setValue("username", m_username);

        // Get the name of the FairWind++ configuration file
        m_password = settings.value("password", m_password).toString();

        // Store the configuration in the settings
        settings.setValue("password", m_password);

        // Get the name of the FairWind++ configuration file
        m_mSleep = settings.value("sleep", m_mSleep).toInt();

        // Store the configuration in the settings
        settings.setValue("sleep", m_mSleep);

        // Get the name of the FairWind++ configuration file
        m_nRetry = settings.value("retry", m_nRetry).toInt();

        // Store the configuration in the settings
        settings.setValue("retry", m_nRetry);


        if (m_debug) {

            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);

            qDebug() << "Configration from ini file loaded.";
        }
    }

    bool FairWindSK::startSignalK() {
        bool result = false;

        QMap<QString,QVariant> params;
        params["debug"] = m_debug;
        params["active"] = true;
        params["restore"] = true;
        params["username"] = m_username;
        params["password"] = m_password;

        // Number of connection tentatives
        int count = 1;

        // Start the connection
        do {

            if (m_debug) {
                qDebug() << "Trying to connect to the " << m_signalKServerUrl << " Signal K server (" << count << "/" << m_nRetry << ")...";
            }

            // Try to connect
            result = m_signalkClient.init(params);

            // Check if the connection is successful
            if (result) {

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

            int count = 0;

            auto appsJsonArray = doc.array();
            for (auto appJsonItem: appsJsonArray) {

                if (appJsonItem.isObject()) {
                    auto appItem = new AppItem(appJsonItem.toObject(), true, count);

                    m_mapHash2AppItem[appItem->getName()] = appItem;

                    count++;
                }
            }
            result = true;
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

    QString FairWindSK::getUsername()  {
        return m_username;
    }

    QString FairWindSK::getPassword()  {
        return m_password;
    }

    bool FairWindSK::isDebug() const {
        return m_debug;
    }

    /*
 * getConfig
 * Returns the configuration infos
 */
    QJsonObject FairWindSK::getConfiguration() {

        // Define the result
        QJsonObject result;

        // Get the configuration
        auto configurationJsonObject = m_signalkClient.signalkGet(m_signalKServerUrl + "/plugins/dynamo-signalk-fairwindsk-plugin/config");

        // Check if the configuration object has the key Options
        if (configurationJsonObject.contains("configuration") && configurationJsonObject["configuration"].isObject()) {

            // Get the Options object
            result = configurationJsonObject["configuration"].toObject();
        }

        if (m_debug)
            qDebug() << "Configuration:" << result;

        // Return the result
        return result;
    }
}
