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

        m_username = "admin";
        m_password = "password";

        m_configFilename = "fairwindsk.json";

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

    void FairWindSK::saveConfig() {


        QFile jsonFile(m_configFilename);
        jsonFile.open(QFile::WriteOnly);
        jsonFile.write(QJsonDocument(m_configuration).toJson());

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
        m_configFilename = settings.value("config", m_configFilename).toString();

        // Store the name of the FairWindSK configuration in the settings
        settings.setValue("config",m_configFilename);

        QString configJson = "";
        QFileInfo check_file(m_configFilename);
        // check if file exists and if yes: Is it really a file and no directory?
        if (check_file.exists() && check_file.isFile()) {
            QFile configFile(m_configFilename);
            if (configFile.open(QFile::ReadOnly | QFile::Text)) {
                QTextStream in(&configFile);
                configJson = in.readAll();
            }
        } else {
            configJson = R"(
{
    "connection": {
        "server": "http://localhost:3000",
        "sleep": 5,
        "retry": 10,
        "token": ""
    },
    "signalk": {
        "pos": "navigation.position",
        "cog": "navigation.courseOverGroundTrue",
        "sog": "navigation.speedOverGround",
        "hdg": "navigation.headingTrue",
        "stw": "navigation.speedThroughWater",
        "dpt": "environment.depth.belowTransducer",
        "wpt": "navigation.course.nextPoint",
        "btw": "navigation.course.calcValues.bearingTrue",
        "dtg": "navigation.course.calcValues.distance",
        "ttg": "navigation.course.calcValues.timeToGo",
        "eta": "navigation.course.calcValues.estimatedTimeOfArrival",
        "xte": "navigation.course.calcValues.crossTrackError",
        "vmg": "performance.velocityMadeGood"
    },
    "bottomBarApps": {
        "mydata": "",
        "mob": "",
        "alarms": "",
        "settings": "admin"
    },
    "units": {
        "vesselSpeed": "kn",
        "windSpeed": "kn",
        "distance": "nm",
        "depth": "m",
        "airTemperature": "C",
        "airPressure": "HPa"
    },
    "apps": [
        {
            "name": "@signalk/freeboard-sk",
            "signalk": {
                "appIcon": "file://icons/chart_icon.png"
            },
            "fairwind": {
                "order": 1
            }
        },
        {
            "name": "@mxtommy/kip",
            "signalk": {
                "appIcon": "file://icons/dashboard_icon.png"
            },
            "fairwind": {
                "order": 2
            }
        },
        {
            "name": "sk-depth-gauge",
            "signalk": {
                "displayName": "Depth",
                "appIcon": "file://icons/sonar_icon.png"
            },
            "fairwind": {
                "order": 3
            }
        },
        {
            "name": "signalk-top3ais",
            "signalk": {
                "displayName": "AIS",
                "appIcon": "file://icons/radar_icon.png"
            },
            "fairwind": {
                "order": 4
            }
        },
        {
            "name": "signalk-multiplex-viewer",
            "signalk": {
                "displayName": "Signal K Multiplex Viewer",
                "appIcon": "file://icons/signalkmultiplexviewer_icon.png"
            }
        },
        {
            "name": "signalk-browser",
            "signalk": {
                "displayName": "Signal K Browser",
                "appIcon": "file://icons/signalkbrowser_icon.png"
            }
        },
        {
            "name": "@signalk/sailgauge",
            "signalk": {
                "displayName": "Sail Gauge",
                "appIcon": "file://icons/sailgauge_icon.png"
            }
        },

            "description": "OpenCPN Open Source Chart Plotter",
            "fairwind": {
                "active": true,
                "arguments": [
                    "-fullscreen"
                ]
            },
            "name": "file:///Applications/OpenCPN.app/Contents/MacOS/OpenCPN",
            "signalk": {
                "appIcon": "file:///Applications/OpenCPN.app/Contents/SharedSupport/opencpn.png",
                "displayName": "OpenCPN"
            }
        },
        {
            "name": "admin",
            "fairwind": {
                "active": false
            }
        },
        {
            "name": "admin/#/appstore/apps",
            "fairwind": {
                "active": false
            }
        },
        {
            "name": "__SETTINGS__",
            "signalk": {
                "displayName": "Settings"
            },
            "fairwind": {
                "active": false
            }
        },
        {
            "name": "http:///",
            "description": "Signal K Server",
            "signalk": {
                "displayName": "Signal K",
                "appIcon": "file://icons/signalkserver_icon.png"
            },
            "fairwind": {
                "active": true,
                "order": 1000
            }
        },
        {
            "name": "http://spotify.com",
            "description": "Spotify web application",
            "signalk": {
                "displayName": "Spotify",
                "appIcon": "file://icons/spotify_icon.png"
            },
            "fairwind": {
                "active": true
            }
        },
        {
            "name": "http://netflix.com",
            "description": "Netflix web application",
            "signalk": {
                "displayName": "Netflix",
                "appIcon": "file://icons/netflix_icon.png"
            },
            "fairwind": {
                "active": true
            }
        },
        {
            "name": "http://youtube.com",
            "description": "Youtube web application",
            "signalk": {
                "displayName": "Youtube",
                "appIcon": "file://icons/youtube_icon.png"
            },
            "fairwind": {
                "active": true
            }
        }
    ]
}        )";
            QFile configFile(m_configFilename);
            if (configFile.open(QFile::WriteOnly | QFile::Text)) {
                QTextStream out(&configFile);
                out << configJson << Qt::endl;
            }

        }

        m_configuration = QJsonDocument::fromJson(configJson.toUtf8()).object();

        if (m_debug) {

            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);

            qDebug() << "Configration from ini file loaded.";
        }

    }

    bool FairWindSK::startSignalK() {
        bool result = false;

        auto signalKServerUrl = getSignalKServerUrl();

        if (!signalKServerUrl.isEmpty()) {
            // Define the parameters map
            QMap<QString, QVariant> params;

            // Set some defaults
            params["active"] = true;


            // Setup the debug mode
            params["debug"] = m_debug;

            // Set the url
            params["url"] = signalKServerUrl + "/signalk";

            QString token = getToken();
            int nRetry = getRetry();
            int mSleep = getSleep();

            // Check if the token is defined or if username/password are defined
            if (!token.isEmpty() || (!m_username.isEmpty() && !m_password.isEmpty())) {

                // Check if the token is defined
                if (!token.isEmpty()) {

                    // Set the token
                    params["token"] = token;
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
                        qDebug() << "Trying to connect to the " << signalKServerUrl << " Signal K server ("
                                 << count
                                 << "/" << nRetry << ")...";
                    }

                    // Try to connect
                    result = m_signalkClient.init(params);

                    // Check if the connection is successful
                    if (result) {

                        // Set the token
                        setToken(m_signalkClient.getToken());

                        // Exit the loop
                        break;
                    }

                    // Process the events
                    QApplication::processEvents();

                    // Increase the number of retry
                    count++;

                    // Wait for m_mSleep microseconds
                    QThread::msleep(mSleep);

                    // Loop until the number of retry
                } while (count < nRetry);

                if (m_debug) {
                    if (result) {
                        qDebug() << "Connected to " << signalKServerUrl;
                    } else {
                        qDebug() << "No response from the " << signalKServerUrl << " Signal K server!";
                    }
                }
            }
        }

        // Return the result
        return result;
    }

    bool FairWindSK::loadApps() {

        // Set the result value
        bool result = false;

        // Get the app keys
        auto keys = m_mapHash2AppItem.keys();

        // Remove all app items
        for (const auto& key: keys) {

            // Remove the item
            delete m_mapHash2AppItem[key];
        }

        // Remove the map content
        m_mapHash2AppItem.empty();

        // Reset the counter
        int count = 100;

        auto signalKServerUrl = getSignalKServerUrl();
        if (!signalKServerUrl.isEmpty()) {

            // Set the URL for the application list
            QUrl url = QUrl(signalKServerUrl + "/skServer/webapps");

            // Create the network access manager
            QNetworkAccessManager networkAccessManager;

            // Create the event loop component
            QEventLoop loop;

            // Connect the network manager to the event loop
            connect(&networkAccessManager, &QNetworkAccessManager::finished, &loop,&QEventLoop::quit);

            // Create the get request
            QNetworkReply *reply = networkAccessManager.get(QNetworkRequest(url));

            // Wait until the request is satisfied
            loop.exec();

            // Check if the response has been successful
            if (reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 200) {

                // Create a json document with the request response
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());

                // Check if the debug is active
                if (m_debug) {

                    // Show the document
                    qDebug() << doc;
                }



                // Get the application array
                auto appsJsonArray = doc.array();

                // For each item of the array...
                for (auto appJsonItem: appsJsonArray) {

                    // Check if the item is an object
                    if (appJsonItem.isObject()) {

                        // Get the json object
                        auto appJsonObject = appJsonItem.toObject();

                        // Create the fairwind element if not present or is not an object
                        if (!appJsonObject.contains("fairwind") ||
                            (appJsonObject.contains("fairwind") && !appJsonObject["fairwind"].isObject())) {
                            appJsonObject["fairwind"] = QJsonObject(QJsonDocument::fromJson(
                                    (QString(R"({ "active": true, "order": %1 })")).arg(
                                            count).toUtf8()).object());

                        }

                        // Check if the app is a signalk-webapp
                        if (appJsonObject.contains("keywords") &&
                            appJsonObject["keywords"].isArray() &&
                            appJsonObject["keywords"].toArray().contains("signalk-webapp")) {

                            // Create an app item object
                            auto appItem = new AppItem(appJsonObject);

                            // Add the item to the lookup table
                            m_mapHash2AppItem[appItem->getName()] = appItem;

                            // Increase the app counter
                            count++;
                        }
                    }
                }
            }

            // Set the result as successful
            result = true;
        } else {

            // Check if the debug is active
            if (m_debug) {

                // Show a debug message
                qDebug() << "Troubles on getting apps from " << signalKServerUrl;
            }
        }


        auto configuration = getConfiguration();
        if (configuration.contains("apps") && configuration["apps"].isArray()) {
            auto appsJsonArray = configuration["apps"].toArray();
            for (auto app: appsJsonArray) {
                if (app.isObject()) {
                    auto appJsonObject = app.toObject();
                    if (appJsonObject.contains("name") && appJsonObject["name"].isString()) {
                        auto appName = appJsonObject["name"].toString();

                        if (m_mapHash2AppItem.contains(appName)) {

                            m_mapHash2AppItem[appName]->update(appJsonObject);
                        } else {
                            auto appItem = new AppItem(appJsonObject);
                            if (appItem->getOrder() == 0) {
                                appItem->setOrder(count);
                                count++;
                            }
                            m_mapHash2AppItem[appName] = appItem;


                        }
                    }

                }
            }
        }

        qDebug() << m_mapHash2AppItem.keys();


        // Return the result
        return result;
    }

    QList<QString> FairWindSK::getAppsHashes() {
        return m_mapHash2AppItem.keys();
    }



    AppItem *FairWindSK::getAppItemByHash(QString hash) {
        return m_mapHash2AppItem[hash];
    }

    QString FairWindSK::getAppHashById(QString appId) {
        return m_mapAppId2Hash[appId];
    }

    QString FairWindSK::getSignalKServerUrl(){

        QString signalKServerUrl = "";
        if (m_configuration.contains("connection")) {
            auto connectionJsonObject = m_configuration["connection"].toObject();
            if (connectionJsonObject.contains("server") & connectionJsonObject["server"].isString()) {
                signalKServerUrl = connectionJsonObject["server"].toString();
            }
        }
        return signalKServerUrl;
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

    signalk::Client *FairWindSK::getSignalKClient() {
        return &m_signalkClient;
    }

    QString FairWindSK::getToken() {
        QString token = "";
        if (m_configuration.contains("connection")) {
            auto connectionJsonObject = m_configuration["connection"].toObject();
            if (connectionJsonObject.contains("token") & connectionJsonObject["token"].isString()) {
                token = connectionJsonObject["token"].toString();
            }
        }
        return token;
    }

    void FairWindSK::setToken(const QString& token) {

        modifyJsonValue(m_configuration,"connection.token",QJsonValue(token));
        saveConfig();
    }

    int FairWindSK::getSleep() {
        int mSleep = 0;

        if (m_configuration.contains("connection")) {
            auto connectionJsonObject = m_configuration["connection"].toObject();
            if (connectionJsonObject.contains("sleep") & connectionJsonObject["sleep"].isDouble()) {
                mSleep = connectionJsonObject["sleep"].toInt();
            }
        }
        return mSleep;
    }

    int FairWindSK::getRetry() {
        int nRetry = 1;

        if (m_configuration.contains("connection")) {
            auto connectionJsonObject = m_configuration["connection"].toObject();
            if (connectionJsonObject.contains("retry") & connectionJsonObject["retry"].isDouble()) {
                nRetry = connectionJsonObject["retry"].toInt();
            }
        }
        return nRetry;
    }

    void FairWindSK::setSignalKServerUrl(const QString& signalKServerUrl) {
        modifyJsonValue(m_configuration,"connection.server",QJsonValue(signalKServerUrl));
        saveConfig();
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

    void FairWindSK::modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue) {
        const int indexOfDot = path.indexOf('.');
        const QString propertyName = path.left(indexOfDot);
        const QString subPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

        QJsonValue subValue = obj[propertyName];

        if (subPath.isEmpty()) {
            subValue = newValue;
        } else {
            QJsonObject obj = subValue.toObject();
            modifyJsonValue(obj, subPath, newValue);
            subValue = obj;
        }

        obj[propertyName] = subValue;
    }

}

