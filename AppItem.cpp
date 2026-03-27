//
// Created by Raffaele Montella on 27/03/21.
//

#include <QCryptographicHash>
#include <utility>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QtCore/qjsonarray.h>
#include <QPixmap>
#include <QHash>
#include <iostream>

#include "AppItem.hpp"

#include <QWidget>

#include "FairWindSK.hpp"

namespace fairwindsk {
    namespace {
        constexpr int kCatalogRequestTimeoutMs = 5000;

        struct LegacyCatalog {
            QHash<QString, QString> iconsByName;
            QHash<QString, QString> displayNamesByName;
            bool loaded = false;
        };

        QByteArray performBlockingGet(const QUrl &url, int *statusCode = nullptr) {
            if (statusCode) {
                *statusCode = -1;
            }
            if (!url.isValid()) {
                return {};
            }

            QNetworkAccessManager networkAccessManager;
            QNetworkRequest request(url);
            request.setTransferTimeout(kCatalogRequestTimeoutMs);

            QPointer<QNetworkReply> reply = networkAccessManager.get(request);
            if (!reply) {
                return {};
            }

            QEventLoop loop;
            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeoutTimer.start(kCatalogRequestTimeoutMs);
            loop.exec();

            if (!reply) {
                return {};
            }

            if (!reply->isFinished()) {
                reply->abort();
            }

            if (statusCode) {
                *statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            }

            if (!reply->isOpen()) {
                reply->deleteLater();
                return {};
            }

            const QByteArray payload = reply->readAll();
            reply->deleteLater();
            return payload;
        }

        nlohmann::json fetchJsonArray(const QUrl &url) {
            if (!url.isValid()) {
                return {};
            }

            int statusCode = -1;
            const QByteArray payload = performBlockingGet(url, &statusCode);
            if (statusCode < 200 || statusCode >= 300 || payload.isEmpty()) {
                return {};
            }

            try {
                return nlohmann::json::parse(payload.constData(), payload.constData() + payload.size());
            } catch (const std::exception &) {
                return {};
            }
        }

        QString iconStringFromJson(const nlohmann::json &jsonApp) {
            if (jsonApp.contains("signalk") && jsonApp["signalk"].is_object()) {
                const auto &signalkJsonObject = jsonApp["signalk"];
                if (signalkJsonObject.contains("appIcon") && signalkJsonObject["appIcon"].is_string()) {
                    return QString::fromStdString(signalkJsonObject["appIcon"].get<std::string>());
                }
            }
            if (jsonApp.contains("appIcon") && jsonApp["appIcon"].is_string()) {
                return QString::fromStdString(jsonApp["appIcon"].get<std::string>());
            }
            if (jsonApp.contains("icon") && jsonApp["icon"].is_string()) {
                return QString::fromStdString(jsonApp["icon"].get<std::string>());
            }
            return {};
        }

        const LegacyCatalog &legacyCatalogForServer(const QString &serverUrl) {
            static QHash<QString, LegacyCatalog> cachedCatalogsByServer;
            auto &catalog = cachedCatalogsByServer[serverUrl];

            if (!serverUrl.isEmpty() && !catalog.loaded) {
                catalog.loaded = true;
                const auto legacyCatalog = fetchJsonArray(QUrl(serverUrl + "/skServer/webapps"));
                if (legacyCatalog.is_array()) {
                    for (const auto &appJson : legacyCatalog) {
                        if (!appJson.is_object() || !appJson.contains("name") || !appJson["name"].is_string()) {
                            continue;
                        }
                        const QString legacyName = QString::fromStdString(appJson["name"].get<std::string>());
                        const QString legacyIcon = iconStringFromJson(appJson);
                        if (!legacyName.isEmpty() && !legacyIcon.isEmpty()) {
                            catalog.iconsByName.insert(legacyName, legacyIcon);
                        }

                        QString legacyDisplayName;
                        if (appJson.contains("signalk") && appJson["signalk"].is_object()) {
                            const auto &signalkJsonObject = appJson["signalk"];
                            if (signalkJsonObject.contains("displayName") && signalkJsonObject["displayName"].is_string()) {
                                legacyDisplayName = QString::fromStdString(signalkJsonObject["displayName"].get<std::string>());
                            }
                        }
                        if (legacyDisplayName.isEmpty() && appJson.contains("displayName") && appJson["displayName"].is_string()) {
                            legacyDisplayName = QString::fromStdString(appJson["displayName"].get<std::string>());
                        }
                        if (!legacyName.isEmpty() && !legacyDisplayName.isEmpty()) {
                            catalog.displayNamesByName.insert(legacyName, legacyDisplayName);
                        }
                    }
                }
            }

            return catalog;
        }

        QString iconFromLegacyCatalog(const QString &serverUrl, const QString &appName) {
            if (serverUrl.isEmpty() || appName.isEmpty()) {
                return {};
            }

            return legacyCatalogForServer(serverUrl).iconsByName.value(appName);
        }

        QString displayNameFromLegacyCatalog(const QString &serverUrl, const QString &appName) {
            if (serverUrl.isEmpty() || appName.isEmpty()) {
                return {};
            }

            return legacyCatalogForServer(serverUrl).displayNamesByName.value(appName);
        }

        QPixmap loadRemotePixmap(const QList<QUrl> &candidateUrls, const QPixmap &fallback) {
            for (const auto &iconUrl : candidateUrls) {
                if (!iconUrl.isValid() || iconUrl.scheme().isEmpty()) {
                    continue;
                }

                int statusCode = -1;
                const QByteArray payload = performBlockingGet(iconUrl, &statusCode);
                if (statusCode < 200 || statusCode >= 300) {
                    continue;
                }

                QPixmap pixmap = fallback;
                if (pixmap.loadFromData(payload)) {
                    return pixmap;
                }
            }

            return fallback;
        }

        QPixmap bundledFallbackIcon(const QString &appName, const QString &displayName, const QString &appUrl) {
            const QString combined = (appName + " " + displayName + " " + appUrl).toLower();
            QString resourcePath = QStringLiteral(":/resources/images/icons/webapp-256x256.png");

            if (combined.contains(QStringLiteral("youtube"))) {
                resourcePath = QStringLiteral(":/resources/images/icons/youtube_icon.png");
            } else if (combined.contains(QStringLiteral("server-admin-ui")) ||
                       combined.contains(QStringLiteral("signalk server")) ||
                       combined.contains(QStringLiteral("signalk admin"))) {
                resourcePath = QStringLiteral(":/resources/images/icons/signalkserver_icon.png");
            } else if (combined.contains(QStringLiteral("http:///")) ||
                       combined.contains(QStringLiteral("https:///")) ||
                       combined.contains(QStringLiteral("signalk browser"))) {
                resourcePath = QStringLiteral(":/resources/images/icons/signalkbrowser_icon.png");
            } else if (combined.startsWith(QStringLiteral("http://")) ||
                       combined.startsWith(QStringLiteral("https://"))) {
                resourcePath = QStringLiteral(":/resources/images/icons/web_icon.png");
            }

            return QPixmap::fromImage(QImage(resourcePath));
        }
    }

    AppItem::AppItem() = default;

/*
 * Public Constructor
 */
    AppItem::AppItem(nlohmann::json jsonApp) {

        // Get the app's infos and store them for future usage
        m_jsonApp = std::move(jsonApp);

    }

/*
     * setOrder
     * Sets the app's order
     */
    void AppItem::setOrder(int order) {

        // Set the order value
        m_jsonApp["fairwind"]["order"] = order;

    }

    void AppItem::update(const nlohmann::json& jsonApp) {

        //m_jsonApp.update(jsonApp);
        m_jsonApp.update(jsonApp, true);
        m_hasCachedIcon = false;
        m_cachedIcon = QPixmap();
    }


    /*
     * getOrder
     * Returns the app's order
     */
    int AppItem::getOrder() {
        int result = 0;

        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"];
            if (fairwindJsonObject.contains("order") && fairwindJsonObject["order"].is_number_integer()) {
                result = fairwindJsonObject["order"].get<int>();
            }
        }
        return result;
    }

    /*
     * setActive
     * Sets the app's active state
     */
    void AppItem::setActive(bool active) {

        // Set the active value
        m_jsonApp["fairwind"]["active"] = active;
    }

    /*
     * getActive
     * Returns the app's active state
     */
    bool AppItem::getActive() {
        bool result = false;
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"];
            if (fairwindJsonObject.contains("active") && fairwindJsonObject["active"].is_boolean()) {
                result = fairwindJsonObject["active"].get<bool>();
            }
        }
        return result;
    }

    /*
     * getName
     * Returns the app's name
     */
    QString AppItem::getName() {
        std::string name;
        if (m_jsonApp.contains("name") && m_jsonApp["name"].is_string()) {
            name = m_jsonApp["name"].get<std::string>();
        }
        return QString::fromStdString(name);
    }

    /*
     * getDesc
     * Returns the app's description
     */
    QString AppItem::getDescription() {
        std::string desc;
        if (m_jsonApp.contains("description") && m_jsonApp["description"].is_string()) {
            desc = m_jsonApp["description"].get<std::string>();
        }
        return QString::fromStdString(desc);
    }

    /*
     * getIcon
     * Returns the app's icon
     */
    QPixmap AppItem::getIcon() {
        if (m_hasCachedIcon && !m_cachedIcon.isNull()) {
            return m_cachedIcon;
        }

        const QString appName = getName();
        const QString displayName = getDisplayName();
        const QString appUrl = getUrl();
        // Use a default pixmap so the UI never shows an empty placeholder.
        QPixmap pixmap = bundledFallbackIcon(appName, displayName, appUrl);
        // Read the optional app icon path from the application metadata.
        QString appIcon = getAppIcon();

        // Obtain the base server URL to construct the remote icon endpoint.
        const auto signalKServerUrl = FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl();
        // Avoid issuing a network request if the server URL is not available.
        if (signalKServerUrl.isEmpty()) {
            m_cachedIcon = pixmap;
            m_hasCachedIcon = true;
            return pixmap;
        }

        if (appIcon.isEmpty()) {
            appIcon = iconFromLegacyCatalog(signalKServerUrl, getName());
        }

        // If there is still no icon path, return the default without doing any extra work.
        if (appIcon.isEmpty()) {
            m_cachedIcon = pixmap;
            m_hasCachedIcon = true;
            return pixmap;
        }

        // Handle icons bundled with the plugin and referenced using a file:// URL.
        if (appIcon.startsWith("file://")) {
            // Strip the scheme prefix to obtain a regular filesystem path.
            const auto iconFilename = QString(appIcon).replace("file://", "");
            // Attempt to load the provided file, keeping the default if loading fails.
            pixmap.load(iconFilename);
            m_cachedIcon = pixmap;
            m_hasCachedIcon = true;
            return pixmap;
        }

        QList<QUrl> candidateUrls;
        candidateUrls.append(QUrl(appIcon));
        candidateUrls.append(QUrl(appUrl).resolved(QUrl(appIcon)));
        candidateUrls.append(QUrl(signalKServerUrl + "/" + appName + "/" + appIcon));

        m_cachedIcon = loadRemotePixmap(candidateUrls, pixmap);
        m_hasCachedIcon = true;
        return m_cachedIcon;
    }

    /*
     * getHash
     * Returns the app's generated hash
     */
    QString AppItem::getDisplayName() {
        QString displayName = getName();
        if (m_jsonApp.contains("displayName") && m_jsonApp["displayName"].is_string()) {
            displayName = QString::fromStdString(m_jsonApp["displayName"].get<std::string>());
        }
        if (m_jsonApp.contains("signalk") &&  m_jsonApp["signalk"].is_object()) {
            auto signalkJsonObject = m_jsonApp["signalk"];
            if (signalkJsonObject.contains("displayName") && signalkJsonObject["displayName"].is_string()) {
                displayName = QString::fromStdString(signalkJsonObject["displayName"].get<std::string>());
            }
        }
        if (displayName.isEmpty() || displayName == getName()) {
            const auto signalKServerUrl = FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl();
            const auto legacyDisplayName = displayNameFromLegacyCatalog(signalKServerUrl, getName());
            if (!legacyDisplayName.isEmpty()) {
                displayName = legacyDisplayName;
            }
        }
        return displayName;
    }

    /*
     * getVersion
     * Returns the app's version
     */
    QString AppItem::getVersion() {
        std::string version;
        if (m_jsonApp.contains("version") && m_jsonApp["version"].is_string()) {
            version = m_jsonApp["version"].get<std::string>();
        }
        return QString::fromStdString(version);
    }

    /*
     * getAuthor
     * Returns the app's author
     */
    QString AppItem::getAuthor() {
        std::string author;
        if (m_jsonApp.contains("author") && m_jsonApp["author"].is_string()) {
            author =  m_jsonApp["author"].get<std::string>();
        }
        return QString::fromStdString(author);
    }

    /*
     * getContributors
     * Returns the app's contributors
     */
    QVector<QString> AppItem::getContributors() {
        QVector<QString> contributors;
        if (m_jsonApp.contains("contributors") && m_jsonApp["contributors"].is_array()) {
            auto contributorsJsonArray = m_jsonApp["contributors"];
            for (const auto& jsonItem: contributorsJsonArray) {
                if (jsonItem.is_string() && !jsonItem.get<std::string>().empty()) {
                    contributors.append(QString::fromStdString(jsonItem.get<std::string>()));
                }

            }
        }
        return contributors;
    }

    /*
     * getCopyright
     * Returns the app's copyright
     */
    QString AppItem::getCopyright() {
        QString copyright;
        if (m_jsonApp.contains("copyright") && m_jsonApp["copyright"].is_string()) {
            copyright = QString::fromStdString(m_jsonApp["copyright"].get<std::string>());
        }
        return copyright;
    }

    /*
     * getCopyright
     * Returns the app's vendor
     */
    QString AppItem::getVendor() {
        QString vendor;
        if (m_jsonApp.contains("vendor") && m_jsonApp["vendor"].is_string()) {
            vendor = QString::fromStdString(m_jsonApp["vendor"].get<std::string>());
        }
        return vendor;
    }

    /*
     * getLicense
     * Returns the app's license
     */
    QString AppItem::getLicense() {
        QString license;
        if (m_jsonApp.contains("license") && m_jsonApp["license"].is_string()) {
            license = QString::fromStdString(m_jsonApp["license"].get<std::string>());
        }
        return license;
    }

    /*
     * getLicense
     * Returns the app's license
     */
    QString AppItem::getUrl() {


        QString url;

        if (m_jsonApp.contains("location") && m_jsonApp["location"].is_string()) {
            url = QString::fromStdString(m_jsonApp["location"].get<std::string>());
        }

        if (url.isEmpty()) {
            url = getName();
        }

        if (url.startsWith("http:///")) {

            // Create the url string substituting the placeholder with the server url
            url.replace("http:///", FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl());

        } else if (url.startsWith("https:///")) {

            // Create the url string substituting the placeholder with the server url
            url.replace("https:///", FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl());

        } if (url.startsWith("http://") || url.startsWith("https://")) {



        } else {
            const auto serverUrl = FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl();
            url = QUrl(serverUrl + "/").resolved(QUrl(url)).toString();
        }
        return url;
    }

    void AppItem::setWidget(QWidget *pWidget) {
        m_pWidget = pWidget;
    }

    QWidget *AppItem::getWidget() {
        return m_pWidget;
    }

    void AppItem::setProcess(QProcess *pProcess) {
        m_pProcess = pProcess;
    }

    QProcess *AppItem::getProcess() {
        return m_pProcess;
    }

    bool AppItem::operator<(const AppItem &o) const {
        return m_jsonApp["fairwind"]["order"].get<int>() < o.m_jsonApp["fairwind"]["order"].get<int>();
    }

    QStringList AppItem::getArguments() {
        QStringList result;
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairWindJsonObject = m_jsonApp["fairwind"];

            if (fairWindJsonObject.contains("arguments") && fairWindJsonObject["arguments"].is_array()) {
                auto argumentsJsonArray = fairWindJsonObject["arguments"];
                for (const auto& argument: argumentsJsonArray) {
                    if (argument.is_string()) {
                        result.append(QString::fromStdString(argument.get<std::string>()));
                    }
                }
            }
        }
        return result;
    }

    QString AppItem::getAppIcon() {
        return iconStringFromJson(m_jsonApp);
    }

    void AppItem::setName(const QString& name) {
        // Set the name value
        m_jsonApp["name"] = name.toStdString();
    }

    void AppItem::setDescription(const QString& description) {
        // Set the name value
        m_jsonApp["description"] = description.toStdString();
    }

    void AppItem::setAppIcon(const QString& appIcon) {
        // Set the name value
        m_jsonApp["signalk"]["appIcon"] = appIcon.toStdString();
        m_hasCachedIcon = false;
        m_cachedIcon = QPixmap();
    }

    void AppItem::setSettingsUrl(const QString& settingsUrl) {
        // Set the name value
        m_jsonApp["fairwind"]["settings"] = settingsUrl.toStdString();
    }

    void AppItem::setAboutUrl(const QString& aboutUrl) {
        // Set the name value
        m_jsonApp["fairwind"]["about"] = aboutUrl.toStdString();
    }

    void AppItem::setHelpUrl(const QString& helpUrl) {
        // Set the name value
        m_jsonApp["fairwind"]["help"] = helpUrl.toStdString();
    }

    QString AppItem::getSettingsUrl(const QString& pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"];
            if (fairwindJsonObject.contains("settings") && fairwindJsonObject["settings"].is_string()) {
                result = QString::fromStdString(fairwindJsonObject["settings"].get<std::string>());
            }
        }
        if (result.isEmpty()) {
            result = getName();
        }
        if (result.startsWith("file://")) {
            result = result.replace("file://","");
        } else if (!result.startsWith("http://") && !result.startsWith("https://")) {
            result = pluginUrl + result;
        }
        return result;
    }

    QString AppItem::getAboutUrl(const QString& pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"];
            if (fairwindJsonObject.contains("about") && fairwindJsonObject["about"].is_string()) {
                result = QString::fromStdString(fairwindJsonObject["about"].get<std::string>());
            }
        }
        return result;
    }

    QString AppItem::getHelpUrl(const QString& pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"];
            if (fairwindJsonObject.contains("help") && fairwindJsonObject["help"].is_string()) {
                result = QString::fromStdString(fairwindJsonObject["help"].get<std::string>());
            }
        }
        return result;
    }

    void AppItem::setDisplayName(const QString& displayName) {
        // Set the name value
        m_jsonApp["signalk"]["displayName"] = displayName.toStdString();
    }

    nlohmann::json AppItem::asJson() {
        return m_jsonApp;
    }

    /*
     * ~AppItem
     * AppItem's destructor
     */
    AppItem::~AppItem() {
    }
}
