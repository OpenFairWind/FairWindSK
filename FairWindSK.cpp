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



        // Se the default configuration file name
        m_configFilename = "fairwindsk.json";

        // Create the WebEngine profile file name
        auto profileName = QString::fromLatin1("FairWindSK.%1").arg(qWebEngineChromiumVersion());

        // Create the WebEngine profile
        m_profile = new QWebEngineProfile(profileName);

        // Create an authentication cookie from the configuration
        auto authenticationCookie = QNetworkCookie("JAUTHENTICATION", fairwindsk::Configuration::getToken().toUtf8());

        // Set the cookie
        m_profile->cookieStore()->setCookie(authenticationCookie,QUrl(m_configuration.getSignalKServerUrl()));
    }



/*
 * getInstance
 * Either returns the available instance or creates a new one
 */
    FairWindSK *FairWindSK::getInstance() {

        // Check if there is no previous instance
        if (m_instance == nullptr) {

            // Create the instance
            m_instance = new FairWindSK();
        }

        // Return the instance
        return m_instance;
    }

    /*
     * loadConfig()
     * Load the configuration from the json file
     */
    void FairWindSK::loadConfig() {

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        m_debug = settings.value("debug", m_debug).toBool();

        // Store the configuration in the settings
        settings.setValue("debug", m_debug);

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "QWebEngineProfile " << m_profile->isOffTheRecord() << " data store: " << m_profile->persistentStoragePath();


            // Write a message
            qDebug() << "Loading configuration from ini file...";
        }

        // Get the name of the FairWindSK configuration file
        m_configFilename = settings.value("config", m_configFilename).toString();

        // Store the name of the FairWindSK configuration in the settings
        settings.setValue("config",m_configFilename);

        // Set the configuration file name
        m_configuration.setFilename(m_configFilename);

        // Check if the file exists
        if (QFileInfo::exists(m_configFilename)) {

            // Load the configuration
            m_configuration.load();
        }
        else {
            // Set the default
            m_configuration.setDefault();

            // Save the configuration file
            //m_configuration.save();
        }

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "Configuration from ini file loaded.";

            // Set the QT logging
            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);
        }

    }

    /*
     * startSignalK()
     * Starts the Signal K client
     */
    bool FairWindSK::startSignalK() {

        // Set the result as false
        bool result = false;

        // Get the Signal K server URL
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        // Check if the Signal K URL is not empty
        if (!signalKServerUrl.isEmpty()) {

            // Define the parameters map
            QMap<QString, QVariant> params;

            // Set some defaults
            params["active"] = true;

            // Setup the debug mode
            params["debug"] = m_debug;

            // Set the url
            params["url"] = signalKServerUrl + "/signalk";

            // Get the token
            QString token = fairwindsk::Configuration::getToken();

            // Check if the token is defined
            if (!token.isEmpty()) {

                // Set the token
                params["token"] = token;

                // Number of connection tentatives
                int count = 1;

                // Start the connection
                do {

                    // Check if the debug is active
                    if (isDebug()) {

                        // Write a message
                        qDebug() << "Trying to connect to the " << signalKServerUrl << " Signal K server ("
                                 << count
                                 << "/" << m_nRetry << ")...";
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
                    QThread::msleep(m_mSleep);

                    // Loop until the number of retry
                } while (count < m_nRetry);

                // Check if the debug is active
                if (isDebug()) {

                    // Check if the client is connected
                    if (result) {

                        // Write a message
                        qDebug() << "Connected to " << signalKServerUrl;
                    } else {

                        // Write a message
                        qDebug() << "No response from the " << signalKServerUrl << " Signal K server!";
                    }
                }
            }
        }

        // Return the result
        return result;
    }

    /*
     * loadApps()
     * Load the applications
     */
    bool FairWindSK::loadApps() {

        // Set the result value
        bool result = false;

        // Get the configuration root JSON object
        auto configurationJsonObject = m_configuration.getRoot();

        // Check if apps is not defined in configuration
        if (!configurationJsonObject.contains("apps")) {

            // Add the apps array
            configurationJsonObject["apps"] = nlohmann::json::array();
        }

        // Get the app keys
        auto keys = m_mapHash2AppItem.keys();

        // Remove all app items
        for (const auto& key: keys) {

            // Remove the item
            delete m_mapHash2AppItem[key];
        }

        // Remove the map content
        m_mapHash2AppItem.clear();

        // Reset the counter
        int count = 100;

        // Get the signalk server url from the configuration
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        // Check if it not empty
        if (!signalKServerUrl.isEmpty()) {

            // Set the URL for the application list
            const auto url = QUrl(signalKServerUrl + "/skServer/webapps");

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
                if (isDebug()) {

                    // Write a message
                    qDebug() << QString::fromStdString(doc.dump(2));
                }

                // Check if the json document is an array
                if (doc.is_array()) {

                    // Get the application array
                    auto appsJsonArray = doc;

                    // For each item of the array...
                    for (const auto& appJsonItem: appsJsonArray) {

                        // Check if the item is an object
                        if (appJsonItem.is_object()) {

                            // Get the json object
                            auto appJsonObject = appJsonItem;

                            // Check if keywords key is present and if the value is an array
                            if (appJsonObject.contains("keywords") && appJsonObject["keywords"].is_array()) {

                                // Get the keyword array
                                //auto keywordsJsonArray = appJsonObject["keywords"];
                                std::vector<std::string> keywords = appJsonObject["keywords"]; //keywordsJsonArray;

                                // Load the string vector into a QStringList
                                QStringList stringListKeywords;
                                std::transform(
                                        keywords.begin(), keywords.end(),
                                        std::back_inserter(stringListKeywords), [](const std::string &v){
                                            return QString::fromStdString(v);
                                        }
                                        );

                                // Check if it is a web application
                                if (stringListKeywords.contains("signalk-webapp")) {

                                    // Check if the fairwind element if not present or is not an object
                                    if (
                                            !appJsonObject.contains("fairwind") ||
                                            (
                                                    appJsonObject.contains("fairwind") &&
                                                    appJsonObject["fairwind"].is_null())) {

                                        // Define the fairwind json object
                                        nlohmann::json fairwindJsonObject;

                                        // Set the object active
                                        fairwindJsonObject["active"] = true;

                                        // Use the ordinal as order number
                                        fairwindJsonObject["order"] = count;

                                        // Assign the fairwind json object to the lement
                                        appJsonObject["fairwind"] = fairwindJsonObject;

                                    }

                                    // Create an app item object
                                    auto appItem = new AppItem(appJsonObject);

                                    // Find the app in the configuration object
                                    int idx = m_configuration.findApp(appItem->getName());

                                    // Check the app was been found
                                    if (idx == -1) {

                                        /// Add the app to the configuration
                                        m_configuration.getRoot()["apps"].push_back(appItem->asJson());
                                    }

                                    // Add the item to the lookup table
                                    m_mapHash2AppItem[appItem->getName()] = appItem;

                                    // Increase the app counter
                                    count++;

                                    // Check if the debug is active
                                    if (isDebug())
                                    {
                                        // Write a message
                                        qDebug() << "Added (app from the Signal k server): " << QString::fromStdString(appItem->asJson().dump(2));
                                    }
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
                // Show a message
                qDebug() << "Troubles on getting apps from " << signalKServerUrl;
            }
        }

        // Get the apps array
        auto appsJsonArray = configurationJsonObject["apps"];

        // For each app in the apps array
        for (auto app: appsJsonArray) {

            // Check if the app is an object
            if (app.is_object()) {

                // Check if the app has the key name and if the value is a string
                if (app.contains("name") && app["name"].is_string()) {

                    // Get the app name
                    auto appName = QString::fromStdString(app["name"].get<std::string>());

                    // Check if the app is one of the ones already added
                    if (m_mapHash2AppItem.contains(appName)) {

                        // Get the app item object
                        auto appItem = m_mapHash2AppItem[appName];

                        // Get the index of the application within the apps array
                        int idx = m_configuration.findApp(appName);

                        // Check if the app is present
                        if (idx != -1) {

                            // Get the item as a json
                            auto j = appItem->asJson();

                            // Update the item with app data
                            j.update(app, true);

                            // Assign the item to the configuration
                            m_configuration.getRoot()["apps"].at(idx) = j;

                            // Update the app with the configuration
                            m_mapHash2AppItem[appName]->update(j);

                            // Check if the debug is active
                            if (isDebug())
                            {
                                // Write a message
                                qDebug() << "Updated (app from the Signal k server updated by the configuration file): " << QString::fromStdString(m_configuration.getRoot()["apps"].at(idx).dump(2));
                            }
                        }
                    } else {

                        // Check if it is not a Signal K app
                        if (appName.startsWith("http://") || appName.startsWith("https://") || appName.startsWith("file://")) {

                            // Create a new app item with the configuration
                            auto appItem = new AppItem(app);

                            // Check if the order is 0
                            if (appItem->getOrder() == 0) {

                                // Update the order
                                appItem->setOrder(count);

                                // Get the index of the application within the apps array
                                int idx = m_configuration.findApp(appName);

                                // Check if the app is present
                                if (idx != -1) {

                                    // Update the configuration
                                    m_configuration.getRoot()["apps"].at(idx)["fairwind"]["order"] = count;

                                    // Check if the debug is active
                                    if (isDebug()) {

                                        // Write a message
                                        qDebug() << "Added (app from the configuration file): " << QString::fromStdString(m_configuration.getRoot()["apps"].at(idx).dump(2));
                                    }

                                    // Increase the counter
                                    count++;
                                }
                            }

                            // Add the application to the hash map
                            m_mapHash2AppItem[appName] = appItem;

                            // Check if the debug is active
                            if (isDebug()) {

                                // Write a message
                                qDebug() << "Added: " << appItem->asJson().dump(2);
                            }
                        } else {

                            // Get the index of the application within the apps array
                            int idx = m_configuration.findApp(appName);

                            // Check if the app is present
                            if (idx != -1) {

                                // Update the configuration
                                m_configuration.getRoot()["apps"].at(idx)["fairwind"]["order"] = 10000+count;

                                // Set the application as inactive by default
                                m_configuration.getRoot()["apps"].at(idx)["fairwind"]["active"] = false;

                                // Increase the caunter
                                count++;

                                // The app is not present on the Signal K server)
                                qDebug() << "Deactivated (Signal K application present in the configuration file, but not on the Signal K server): " << QString::fromStdString(m_configuration.getRoot()["apps"].at(idx).dump(2));

                            } else {
                                // Check if the debug is active
                                if (isDebug()) {

                                    // Write a message
                                    qDebug() << "Error!";
                                }
                            }
                        }
                    }
                }
            }
        }

        // Get the configuration json data root
        auto jsonData =m_configuration.getRoot();

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "Resume";
        }

        // Check if the configuration has an apps element and if it is an array
        if (jsonData.contains("apps") && jsonData["apps"].is_array()) {

            // For each item of the apps array...
            for (const auto &app: jsonData["apps"].items()) {

                // Get the application data
                const auto& jsonApp = app.value();

                // Create an application object
                auto appItem = new AppItem(jsonApp);

                // Check if the debug is active
                if (isDebug())
                {
                    // Write a message
                    qDebug() << "App: " << appItem->getName() << " active: " << appItem->getActive() << " order: " << appItem->getOrder();
                }
            }
        }

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

        // Check if the profile is allocated
        if (m_profile) {

            // Delete the profile
            delete m_profile;

            // Set the profile pointer to null
            m_profile = nullptr;
        }

    }


}

