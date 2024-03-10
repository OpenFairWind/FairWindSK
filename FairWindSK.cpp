//
// Created by Raffaele Montella on 03/04/21.
//

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


    bool FairWindSK::startSignalK() {
        QMap<QString,QVariant> params;
        params["debug"] = m_debug;
        params["active"] = true;
        params["restore"] = true;
        params["username"] = m_username;
        params["password"] = m_password;
        return m_signalkClient.init(params);
    }

/*
 * getConfig
 * Returns the configuration infos
 */
    QJsonObject FairWindSK::getConfiguration() {

        // Define the result
        QJsonObject result;

        // Be sure the user is logged as admin
        //m_signalkClient.login();

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

    bool FairWindSK::isDebug() {
        return m_debug;
    }

    void FairWindSK::loadApps() {

        // Remove all app items
        for (auto key: m_mapHash2AppItem.keys()) {
            delete m_mapHash2AppItem[key];
        }

        // Remove the map content
        m_mapHash2AppItem.empty();

        QString settingsString = R"({ "name": "admin" })";
        QJsonDocument settingsJsonDocument = QJsonDocument::fromJson(settingsString.toUtf8());

        qDebug() << settingsJsonDocument;

        auto settingsAppItem = new AppItem(settingsJsonDocument.object(), false, 0);
        m_mapHash2AppItem[settingsAppItem->getName()] = settingsAppItem;

        qDebug() << "name:" << settingsAppItem->getName() << "url:" << settingsAppItem->getUrl();


        QUrl url = QUrl(m_signalKServerUrl + "/skServer/webapps");
        QNetworkAccessManager networkAccessManager;
        QEventLoop loop;
        connect(&networkAccessManager, &QNetworkAccessManager::finished, &loop,&QEventLoop::quit);
        QNetworkReply *reply = networkAccessManager.get(QNetworkRequest(url));
        loop.exec();

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());

        qDebug()<< doc;

        int count = 0;

        auto appsJsonArray = doc.array();
        for (auto appJsonItem : appsJsonArray) {

            if (appJsonItem.isObject()) {
                auto appItem = new AppItem(appJsonItem.toObject(), true, count);

                m_mapHash2AppItem[appItem->getName()] = appItem;

                count++;
            }
        }

    }

    QList<QString> FairWindSK::getAppsHashes() {
        return m_mapHash2AppItem.keys();
    }

    bool FairWindSK::useVirtualKeyboard() {
        // Define the result
        bool result = false;

        // Get the configuration
        auto configuration = getConfiguration();

        // Check if the value is present in configuration
        if (configuration.contains("virtualkeyboard") && configuration["virtualkeyboard"].isBool()) {

            // Get the value
            result = configuration["virtualkeyboard"].toBool();
        }

        return result;
    }

    void FairWindSK::loadConfig() {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        m_signalKServerUrl = settings.value("signalk-server", "http://172.24.1.1:3000").toString();

        // Store the configuration in the settings
        settings.setValue("signalk-server", m_signalKServerUrl);

        // Get the name of the FairWind++ configuration file
        m_username = settings.value("username", "admin").toString();

        // Store the configuration in the settings
        settings.setValue("username", m_username);

        // Get the name of the FairWind++ configuration file
        m_password = settings.value("password", "password").toString();

        // Store the configuration in the settings
        settings.setValue("password", m_password);

        // Get the name of the FairWind++ configuration file
        m_debug = settings.value("debug", false).toBool();

        // Store the configuration in the settings
        settings.setValue("debug", m_debug);

        if (m_debug) {
            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);
        }
    }

}
