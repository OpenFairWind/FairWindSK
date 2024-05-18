//
// Created by Raffaele Montella on 03/04/21.
//
#include <QApplication>
#include <QThread>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>


#include <QLoggingCategory>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <utility>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>

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

        m_configFilename = "fairwindsk.json";

        auto profileName = QString::fromLatin1("FairWindSK.%1").arg(qWebEngineChromiumVersion());

        m_profile = new QWebEngineProfile(profileName );


        auto cookie = QNetworkCookie("JAUTHENTICATION", fairwindsk::Configuration::getToken().toUtf8());
        m_profile->cookieStore()->setCookie(cookie,QUrl(m_configuration.getSignalKServerUrl()));

        if (isDebug()) {
            qDebug() << "QWenEngineProfile " << m_profile->isOffTheRecord() << " data store: " << m_profile->persistentStoragePath();
        }

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
        m_configFilename = settings.value("config", m_configFilename).toString();

        // Store the name of the FairWindSK configuration in the settings
        settings.setValue("config",m_configFilename);

        // Set the configuration file name
        m_configuration.setFilename(m_configFilename);

        // Check if the file exists
        if(QFileInfo::exists(m_configFilename)) {

            // Load the configuration
            m_configuration.load();
        }
        else
        {
            // Set the default
            m_configuration.setDefault();

            // Save the configuration file
            m_configuration.save();
        }

        if (m_debug) {

            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);

            qDebug() << "Configuration from ini file loaded.";
        }

    }

    bool FairWindSK::startSignalK() {
        bool result = false;

        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        if (!signalKServerUrl.isEmpty()) {
            // Define the parameters map
            QMap<QString, QVariant> params;

            // Set some defaults
            params["active"] = true;


            // Setup the debug mode
            params["debug"] = m_debug;

            // Set the url
            params["url"] = signalKServerUrl + "/signalk";

            QString token = fairwindsk::Configuration::getToken();
            int nRetry = 5;
            int mSleep = 1000;

            // Check if the token is defined
            if (!token.isEmpty()) {

                // Set the token
                params["token"] = token;

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
                        fairwindsk::Configuration::setToken(m_signalkClient.getToken());

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


        auto signalKServerUrl = m_configuration.getSignalKServerUrl();
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
                nlohmann::json doc = nlohmann::json::parse(reply->readAll());

                // Check if the debug is active
                if (m_debug) {

                    // Show the document
                    qDebug() << doc.dump(2);
                }

                if (doc.is_array()) {

                    // Get the application array
                    auto appsJsonArray = doc;

                    // For each item of the array...
                    for (const auto& appJsonItem: appsJsonArray) {

                        // Check if the item is an object
                        if (appJsonItem.is_object()) {

                            // Get the json object
                            auto appJsonObject = appJsonItem;

                            // Create the fairwind element if not present or is not an object
                            if (!appJsonObject.contains("fairwind")) {

                                nlohmann::json fairwindJsonObject;
                                fairwindJsonObject["active"] = true;
                                fairwindJsonObject["order"] = count;

                            }

                            // Check if the app is a signalk-webapp
                            if (appJsonObject.contains("keywords") && appJsonObject["keywords"].is_array()) {
                                auto keywordsJsonArray = appJsonObject["keywords"];
                                std::vector<std::string> keywords = keywordsJsonArray;
                                QStringList stringListKeywords;
                                std::transform(
                                        keywords.begin(), keywords.end(),
                                        std::back_inserter(stringListKeywords), [](const std::string &v){
                                            return QString::fromStdString(v);
                                        }
                                        );

                                if (stringListKeywords.contains("signalk-webapp")) {


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


        auto configuration = m_configuration.getRoot();
        if (configuration.contains("apps") && configuration["apps"].is_array()) {
            auto appsJsonArray = configuration["apps"];
            for (auto app: appsJsonArray) {
                if (app.is_object()) {

                    if (app.contains("name") && app["name"].is_string()) {
                        auto appName = QString::fromStdString(app["name"].get<std::string>());

                        if (m_mapHash2AppItem.contains(appName)) {

                            m_mapHash2AppItem[appName]->update(app);
                        } else {
                            auto appItem = new AppItem(app);
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



    AppItem *FairWindSK::getAppItemByHash(const QString& hash) {
        return m_mapHash2AppItem[hash];
    }

    QString FairWindSK::getAppHashById(const QString& appId) {
        return m_mapAppId2Hash[appId];
    }



    bool FairWindSK::isDebug() const {
        return m_debug;
    }

    /*
 * getConfig
 * Returns the configuration infos
 */
    Configuration *FairWindSK::getConfiguration() {

        // Return the result
        return &m_configuration;
    }

    void FairWindSK::setConfiguration(Configuration *configuration) {
        m_configuration.setRoot(configuration->getRoot());
    }

    signalk::Client *FairWindSK::getSignalKClient() {
        return &m_signalkClient;
    }

    QWebEngineProfile *FairWindSK::getWebEngineProfile() {
        return m_profile;
    }

    FairWindSK::~FairWindSK() {
        if (m_profile) {
            delete m_profile;
            m_profile = nullptr;
        }
    }


}

