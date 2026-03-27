//
// Created by Raffaele Montella on 03/04/21.
//
#include <QApplication>
#include <QAbstractButton>
#include <QThread>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QScreen>
#include <QToolButton>
#include <QPushButton>


#include <QLoggingCategory>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <utility>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QUrl>
#include <QPointer>
#include <QTimer>
#include <algorithm>

#include "Units.hpp"


using namespace Qt::StringLiterals;

namespace fairwindsk {
    namespace {
        constexpr int kAppsRequestTimeoutMs = 5000;

        struct UiMetrics {
            int fontPointSize = 12;
            int controlHeight = 32;
            int tabHeight = 32;
            int actionIconSize = 24;
            int prominentIconSize = 56;
            int toggleWidth = 46;
            int toggleHeight = 24;
            int horizontalPadding = 12;
            int verticalPadding = 6;
        };

        UiMetrics uiMetricsForPreset(const QString &preset) {
            UiMetrics metrics;

            if (preset == "small") {
                metrics.fontPointSize = 11;
                metrics.controlHeight = 28;
                metrics.tabHeight = 28;
                metrics.actionIconSize = 20;
                metrics.prominentIconSize = 40;
                metrics.toggleWidth = 40;
                metrics.toggleHeight = 22;
                metrics.horizontalPadding = 10;
                metrics.verticalPadding = 4;
            } else if (preset == "large") {
                metrics.fontPointSize = 14;
                metrics.controlHeight = 40;
                metrics.tabHeight = 40;
                metrics.actionIconSize = 32;
                metrics.prominentIconSize = 72;
                metrics.toggleWidth = 56;
                metrics.toggleHeight = 30;
                metrics.horizontalPadding = 14;
                metrics.verticalPadding = 8;
            } else if (preset == "xlarge") {
                metrics.fontPointSize = 16;
                metrics.controlHeight = 48;
                metrics.tabHeight = 48;
                metrics.actionIconSize = 40;
                metrics.prominentIconSize = 88;
                metrics.toggleWidth = 64;
                metrics.toggleHeight = 34;
                metrics.horizontalPadding = 18;
                metrics.verticalPadding = 10;
            } else {
                metrics.fontPointSize = 12;
                metrics.controlHeight = 34;
                metrics.tabHeight = 34;
                metrics.actionIconSize = 24;
                metrics.prominentIconSize = 56;
                metrics.toggleWidth = 46;
                metrics.toggleHeight = 24;
                metrics.horizontalPadding = 12;
                metrics.verticalPadding = 6;
            }

            return metrics;
        }

        QString resolvedUiScalePreset(const Configuration &configuration) {
            if (configuration.getUiScaleMode() != "auto") {
                return configuration.getUiScalePreset();
            }

            const auto *screen = QGuiApplication::primaryScreen();
            if (!screen) {
                return "normal";
            }

            const auto geometry = screen->availableGeometry();
            const auto shortestSide = std::min(geometry.width(), geometry.height());
            const auto dpi = screen->logicalDotsPerInch();

            if (shortestSide >= 1800 || dpi >= 170.0) {
                return "xlarge";
            }
            if (shortestSide >= 1200 || dpi >= 130.0) {
                return "large";
            }
            if (shortestSide <= 720) {
                return "large";
            }
            return "normal";
        }

        QString buildUiStyleSheet(const UiMetrics &metrics) {
            return QString(
                       "QWidget { font-size: %1pt; }"
                       "QToolButton, QPushButton, QComboBox, QLineEdit, QTextEdit, QPlainTextEdit, "
                       "QSpinBox, QDoubleSpinBox, QDateTimeEdit { min-height: %2px; }"
                       "QToolButton, QPushButton {"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #6b7280, stop:1 #4b5563);"
                       " color: #f9fafb;"
                       " border: 1px solid #9ca3af;"
                       " border-radius: 6px;"
                       " padding: %4px %5px;"
                       " }"
                       "QToolButton:hover, QPushButton:hover {"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #7c8593, stop:1 #5b6472);"
                       " }"
                       "QToolButton:pressed, QPushButton:pressed,"
                       "QToolButton:checked, QPushButton:checked {"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #374151, stop:1 #1f2937);"
                       " padding-top: %7px;"
                       " padding-bottom: %8px;"
                       " }"
                       "QTabWidget::pane { border: 1px solid #4b5563; top: -1px; }"
                       "QTabBar::tab {"
                       " min-height: %3px;"
                       " padding: %4px %5px;"
                       " color: #f9fafb;"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #6b7280, stop:1 #4b5563);"
                       " border: 1px solid #9ca3af;"
                       " border-bottom-color: #4b5563;"
                       " border-top-left-radius: 6px;"
                       " border-top-right-radius: 6px;"
                       " margin-right: 2px;"
                       " }"
                       "QTabBar::tab:selected {"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3b82f6, stop:1 #1d4ed8);"
                       " color: #eff6ff;"
                       " border-color: #60a5fa;"
                       " padding-top: %7px;"
                       " padding-bottom: %8px;"
                       " }"
                       "QTabBar::tab:hover:!selected {"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #7c8593, stop:1 #5b6472);"
                       " }"
                       "QCheckBox { spacing: %5px; color: #f9fafb; }"
                       "QCheckBox::indicator {"
                       " width: %9px;"
                       " height: %10px;"
                       " border-radius: %11px;"
                       " border: 1px solid #9ca3af;"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #6b7280, stop:1 #4b5563);"
                       " }"
                       "QCheckBox::indicator:checked {"
                       " border-color: #60a5fa;"
                       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3b82f6, stop:1 #1d4ed8);"
                       " }"
                       "QCheckBox::indicator:unchecked:hover, QCheckBox::indicator:checked:hover {"
                       " border-color: #dbeafe;"
                       " }"
                       "QHeaderView::section { font-size: %1pt; padding: %4px %5px; }"
                       "QMenu::item { padding: %4px %6px; }")
                .arg(metrics.fontPointSize)
                .arg(metrics.controlHeight)
                .arg(metrics.tabHeight)
                .arg(metrics.verticalPadding)
                .arg(metrics.horizontalPadding)
                .arg(metrics.horizontalPadding * 2)
                .arg(std::max(1, metrics.verticalPadding - 1))
                .arg(metrics.verticalPadding + 1)
                .arg(metrics.toggleWidth)
                .arg(metrics.toggleHeight)
                .arg(metrics.toggleHeight / 2);
        }

        void applyIconMetrics(QWidget *widget, const UiMetrics &metrics) {
            if (auto *toolButton = qobject_cast<QToolButton *>(widget)) {
                const bool prominent =
                    toolButton->toolButtonStyle() == Qt::ToolButtonTextUnderIcon ||
                    toolButton->iconSize().width() >= 48 ||
                    toolButton->minimumHeight() >= 64;
                const QSize iconSize(prominent ? metrics.prominentIconSize : metrics.actionIconSize,
                                     prominent ? metrics.prominentIconSize : metrics.actionIconSize);
                if (!toolButton->icon().isNull()) {
                    toolButton->setIconSize(iconSize);
                }
                if (prominent) {
                    toolButton->setMinimumHeight(std::max(toolButton->minimumHeight(), iconSize.height() + 28));
                }
                return;
            }

            if (auto *pushButton = qobject_cast<QPushButton *>(widget)) {
                if (!pushButton->icon().isNull()) {
                    pushButton->setIconSize(QSize(metrics.actionIconSize, metrics.actionIconSize));
                }
            }
        }

        nlohmann::json fetchJsonPayload(const QUrl &url, const bool debug) {
            if (!url.isValid()) {
                return {};
            }

            QNetworkAccessManager networkAccessManager;
            QNetworkRequest request(url);
            request.setTransferTimeout(kAppsRequestTimeoutMs);
            QPointer<QNetworkReply> reply = networkAccessManager.get(request);
            if (!reply) {
                return {};
            }

            QEventLoop loop;
            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeoutTimer.start(kAppsRequestTimeoutMs);
            loop.exec();

            if (!reply) {
                return {};
            }

            if (!reply->isFinished()) {
                reply->abort();
            }

            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray payload = reply->readAll();
            reply->deleteLater();

            if (statusCode < 200 || statusCode >= 300 || payload.isEmpty()) {
                return {};
            }

            try {
                const auto json = nlohmann::json::parse(payload.constData(), payload.constData() + payload.size());
                if (debug) {
                    qDebug() << "Apps payload from" << url << ":" << QString::fromStdString(json.dump(2));
                }
                return json;
            } catch (const std::exception &exception) {
                if (debug) {
                    qWarning() << "Unable to parse apps payload from" << url << ":" << exception.what();
                }
                return {};
            }
        }

        void ensureFairwindMetadata(nlohmann::json &appJsonObject, const int order) {
            if (!appJsonObject.contains("fairwind") || !appJsonObject["fairwind"].is_object()) {
                appJsonObject["fairwind"] = nlohmann::json::object();
            }

            auto &fairwindJsonObject = appJsonObject["fairwind"];
            if (!fairwindJsonObject.contains("active") || !fairwindJsonObject["active"].is_boolean()) {
                fairwindJsonObject["active"] = true;
            }
            if (!fairwindJsonObject.contains("order") || !fairwindJsonObject["order"].is_number_integer()) {
                fairwindJsonObject["order"] = order;
            }
        }
    }
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
        m_profile = new QWebEngineProfile(profileName, this);

        if (qApp) {
            qApp->installEventFilter(this);
        }

        updateWebProfileCookie();
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

        updateWebProfileCookie();
        applyUiPreferences();

    }

    void FairWindSK::applyUiPreferences(const Configuration *configuration) {
        if (!qApp) {
            return;
        }

        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        const auto preset = resolvedUiScalePreset(effectiveConfiguration);
        const auto metrics = uiMetricsForPreset(preset);

        QFont appFont = qApp->font();
        appFont.setPointSize(metrics.fontPointSize);
        qApp->setFont(appFont);
        qApp->setStyleSheet(buildUiStyleSheet(metrics));

        const auto widgets = QApplication::allWidgets();
        for (auto *widget : widgets) {
            if (!widget) {
                continue;
            }

            applyIconMetrics(widget, metrics);
            widget->updateGeometry();
            widget->update();
        }
    }

    void FairWindSK::updateWebProfileCookie() {
        if (!m_profile) {
            return;
        }

        const auto serverUrl = m_configuration.getSignalKServerUrl();
        const auto token = fairwindsk::Configuration::getToken();
        if (serverUrl.isEmpty() || token.isEmpty()) {
            return;
        }

        auto authenticationCookie = QNetworkCookie("JAUTHENTICATION", token.toUtf8());
        authenticationCookie.setPath("/");
        m_profile->cookieStore()->setCookie(authenticationCookie, QUrl(serverUrl));
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
            }

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
                    updateWebProfileCookie();
                    Units::getInstance()->refreshSignalKPreferences();

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
        auto &configurationJsonObject = m_configuration.getRoot();

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
        m_mapAppId2Hash.clear();

        // Reset the counter
        int count = 100;

        // Get the signalk server url from the configuration
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        // Check if it not empty
        if (!signalKServerUrl.isEmpty()) {
            auto appsPayload = fetchJsonPayload(QUrl(signalKServerUrl + "/signalk/v1/apps/list"), isDebug());
            if (!appsPayload.is_array()) {
                appsPayload = fetchJsonPayload(QUrl(signalKServerUrl + "/skServer/webapps"), isDebug());
            }

            if (appsPayload.is_array()) {
                for (auto appJsonObject : appsPayload) {
                    if (!appJsonObject.is_object()) {
                        continue;
                    }

                    if (appJsonObject.contains("keywords") && appJsonObject["keywords"].is_array()) {
                        std::vector<std::string> keywords = appJsonObject["keywords"];
                        QStringList stringListKeywords;
                        std::transform(
                            keywords.begin(), keywords.end(),
                            std::back_inserter(stringListKeywords), [](const std::string &value) {
                                return QString::fromStdString(value);
                            }
                        );
                        if (!stringListKeywords.contains("signalk-webapp")) {
                            continue;
                        }
                    }

                    ensureFairwindMetadata(appJsonObject, count);

                    auto *appItem = new AppItem(appJsonObject);
                    const QString appName = appItem->getName();
                    const int idx = m_configuration.findApp(appName);

                    if (idx == -1) {
                        m_configuration.getRoot()["apps"].push_back(appItem->asJson());
                    }

                    m_mapHash2AppItem[appName] = appItem;
                    m_mapAppId2Hash[appName] = appName;
                    count++;

                    if (isDebug()) {
                        qDebug() << "Added (app from the Signal K Apps API): "
                                 << QString::fromStdString(appItem->asJson().dump(2));
                    }
                }

                result = true;
            }
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
                            m_mapAppId2Hash[appName] = appName;

                            // Check if the debug is active
                            if (isDebug()) {

                                // Write a message
                                qDebug() << "Added: " << QString::fromStdString(appItem->asJson().dump(2));
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
                AppItem appItem(jsonApp);

                // Check if the debug is active
                if (isDebug())
                {
                    // Write a message
                    qDebug() << "App: " << appItem.getName() << " active: " << appItem.getActive() << " order: " << appItem.getOrder();
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
        return m_mapHash2AppItem.value(hash, nullptr);
    }

    QString FairWindSK::getAppHashById(const QString& appId) {
        if (m_mapHash2AppItem.contains(appId)) {
            return appId;
        }
        return m_mapAppId2Hash.value(appId);
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
        applyUiPreferences();
    }

    signalk::Client *FairWindSK::getSignalKClient() {
        return &m_signalkClient;
    }

    QWebEngineProfile *FairWindSK::getWebEngineProfile() {
        return m_profile;
    }



    bool FairWindSK::checkAutopilotApp() {

        // Set by default result as false
        auto result = false;

        // Check if the anchor application is defined and installed
        if (!m_configuration.getAutopilotApp().isEmpty() && getAppsHashes().contains(m_configuration.getAutopilotApp())) {

            // Set the result as true
            result = true;
        }

        // Return the result
        return result;
    }

    bool FairWindSK::checkAnchorApp() {

        // Set by default result as false
        auto result = false;

        // Check if the anchor application is defined and installed
        if (!m_configuration.getAnchorApp().isEmpty() && getAppsHashes().contains(m_configuration.getAnchorApp())) {

            // Set the result as true
            result = true;
        }

        // Return the result
        return result;
    }

    FairWindSK::~FairWindSK() {
        if (qApp) {
            qApp->removeEventFilter(this);
        }

        // Get the application hashes
        auto hashes = m_mapHash2AppItem.keys();

        // For each hash...
        for (const auto& hash:hashes) {

            // Delete the application item
            delete m_mapHash2AppItem[hash];
        }

    }

    bool FairWindSK::eventFilter(QObject *watched, QEvent *event) {
        if (watched && (event->type() == QEvent::Show || event->type() == QEvent::Polish)) {
            if (auto *widget = qobject_cast<QWidget *>(watched)) {
                const auto metrics = uiMetricsForPreset(resolvedUiScalePreset(m_configuration));
                applyIconMetrics(widget, metrics);
            }
        }

        return QObject::eventFilter(watched, event);
    }

}
