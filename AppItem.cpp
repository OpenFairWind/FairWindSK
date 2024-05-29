//
// Created by Raffaele Montella on 27/03/21.
//

#include <QCryptographicHash>
#include <utility>
#include <QNetworkReply>
#include <QEventLoop>
#include <QtCore/qjsonarray.h>
#include <QPixmap>
#include <iostream>

#include "AppItem.hpp"

#include "FairWindSK.hpp"

namespace fairwindsk {

    AppItem::AppItem() = default;

/*
 * Public Constructor
 */
    AppItem::AppItem(const fairwindsk::AppItem &app) {

        // Retrieve the app's infos from the provided App instance
        m_jsonApp = app.m_jsonApp;


    }

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
        QPixmap pixmap = QPixmap::fromImage(QImage(":/resources/images/icons/webapp-256x256.png"));
        auto appIcon = getAppIcon();

        if (!appIcon.isEmpty()) {
            if (appIcon.startsWith("file://")) {
                auto iconFilename = appIcon.replace("file://","");

                pixmap.load(iconFilename);
            } else {
                // Get the icon URL
                QString url =
                        FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl() + "/" + getName() + "/" + appIcon;

                QNetworkAccessManager nam;
                QEventLoop loop;
                QObject::connect(&nam, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
                QNetworkReply *reply = nam.get(QNetworkRequest(url));
                loop.exec();

                pixmap.loadFromData(reply->readAll());

                delete reply;
            }
        }

        return pixmap;
    }

    /*
     * getHash
     * Returns the app's generated hash
     */
    QString AppItem::getDisplayName() {
        QString displayName = getName();
        if (m_jsonApp.contains("signalk") &&  m_jsonApp["signalk"].is_object()) {
            auto signalkJsonObject = m_jsonApp["signalk"];
            if (signalkJsonObject.contains("displayName") && signalkJsonObject["displayName"].is_string()) {
                displayName = QString::fromStdString(signalkJsonObject["displayName"].get<std::string>());
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


        QString url = getName();

        if (url.startsWith("http:///")) {

            // Create the url string substituting the placeholder with the server url
            url.replace("http:///", FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl());

        } else if (url.startsWith("https:///")) {

            // Create the url string substituting the placeholder with the server url
            url.replace("https:///", FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl());

        } if (url.startsWith("http://") || url.startsWith("https://")) {



        } else {
            url = FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl() + "/" + url + "/";
        }
        return url;
    }

    void AppItem::setWidget(QWidget *pWidget) {
        m_pWidget = pWidget;
    }

    QWidget *AppItem::getWidget() {
        return m_pWidget;
    }

    bool AppItem::operator<(const AppItem &o) const {
        return m_jsonApp["fairwind"]["order"].get<int>() < o.m_jsonApp["fairwind"]["order"].get<int>();
    }

    QStringList AppItem::getArguments() {
        QStringList result;
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].is_object()) {
            auto fairWindJsonObject = m_jsonApp["fairwind"];
            qDebug() << fairWindJsonObject.dump();

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
        QString result = "";
        if (m_jsonApp.contains("signalk") &&  m_jsonApp["signalk"].is_object()) {
            auto signalkJsonObject = m_jsonApp["signalk"];
            if (signalkJsonObject.contains("appIcon") && signalkJsonObject["appIcon"].is_string()) {
                result = QString::fromStdString(signalkJsonObject["appIcon"].get<std::string>());
            }
        }
        return result;
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
        nlohmann::json result;
        result["name"] = m_jsonApp["name"];
        result["description"] = m_jsonApp["description"];

        result["signalk"]["displayName"] = m_jsonApp["signalk"]["displayName"];
        result["signalk"]["appIcon"] = m_jsonApp["signalk"]["appIcon"];

        result["fairwind"] = m_jsonApp["fairwind"];

        return result;
    }
}
