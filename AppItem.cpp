//
// Created by Raffaele Montella on 27/03/21.
//

#include <QCryptographicHash>
#include <utility>
#include <QNetworkReply>
#include <QEventLoop>
#include <QtCore/qjsonarray.h>
#include <QPixmap>

#include "AppItem.hpp"

#include "FairWindSK.hpp"

namespace fairwindsk {

    AppItem::AppItem() {}

/*
 * Public Constructor
 */
    AppItem::AppItem(const fairwindsk::AppItem &app) {
        // Retrieve the app's infos from the provided App instance
        this->m_jsonApp = app.m_jsonApp;


    }

/*
 * Public Constructor
 */
    AppItem::AppItem(QJsonObject jsonApp) {
        // Get the app's infos and store them for future usage
        m_jsonApp = std::move(jsonApp);

    }

/*
     * setOrder
     * Sets the app's order
     */
    void AppItem::setOrder(int order) {

        // Set the order value
        modifyJsonValue(m_jsonApp,"fairwind.order", order);

    }

    void AppItem::modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue) {
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

    /*
     {
            "name": "http://youtube.com",
            "signalk": {
                "displayName": "Youtube",
                "appIcon": "icons/youtube_icon.png"
            },
            "fairwind": {
                "active": true,
                "order": 0
            }
        }
     */
    void AppItem::update(QJsonObject jsonApp) {


        QJsonObject jsonSignalk(m_jsonApp["signalk"].toObject());
        auto srcSignalk = jsonApp["signalk"].toObject();
        for (auto it = srcSignalk.constBegin(); it != srcSignalk.constEnd(); it++) {
            jsonSignalk.insert(it.key(), it.value());
        }

        QJsonObject jsonFairwind(m_jsonApp["fairwind"].toObject());
        auto srcFairwind = jsonApp["fairwind"].toObject();
        for (auto it = srcFairwind.constBegin(); it != srcFairwind.constEnd(); it++) {

            jsonFairwind.insert(it.key(), it.value());
        }

        m_jsonApp.insert("signalk",jsonSignalk);
        m_jsonApp.insert("fairwind",jsonFairwind);


    }

    /*
     * getOrder
     * Returns the app's order
     */
    int AppItem::getOrder() {
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"].toObject();
            if (fairwindJsonObject.contains("order") && fairwindJsonObject["order"].isDouble()) {
                return fairwindJsonObject["order"].toInt();
            }
        }
        return false;
    }

    /*
     * setActive
     * Sets the app's active state
     */
    void AppItem::setActive(bool active) {

        // Set the active value
        modifyJsonValue(m_jsonApp,"fairwind.order", active);
    }

    /*
     * getActive
     * Returns the app's active state
     */
    bool AppItem::getActive() {
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"].toObject();
            if (fairwindJsonObject.contains("active") && fairwindJsonObject["active"].isBool()) {
                return fairwindJsonObject["active"].toBool();
            }
        }
        return false;
    }

    /*
     * getName
     * Returns the app's name
     */
    QString AppItem::getName() {
        QString name;
        if (m_jsonApp.contains("name") && m_jsonApp.value("name").isString()) {
            name = m_jsonApp.value("name").toString();
        }
        return name;
    }

    /*
     * getDesc
     * Returns the app's description
     */
    QString AppItem::getDescription() {
        QString desc;
        if (m_jsonApp.contains("description") && m_jsonApp.value("description").isString()) {
            desc = m_jsonApp.value("description").toString();
        }
        return desc;
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
                        FairWindSK::getInstance()->getSignalKServerUrl() + "/" + getName() + "/" + appIcon;

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
        if (m_jsonApp.contains("signalk") &&  m_jsonApp.value("signalk").isObject()) {
            auto signalkJsonObject = m_jsonApp.value("signalk").toObject();
            if (signalkJsonObject.contains("displayName") && signalkJsonObject.value("displayName").isString()) {
                displayName = signalkJsonObject.value("displayName").toString();
            }
        }
        return displayName;
    }

    /*
     * getVersion
     * Returns the app's version
     */
    QString AppItem::getVersion() {
        QString version;
        if (m_jsonApp.contains("version") && m_jsonApp.value("version").isString()) {
            version = m_jsonApp.value("version").toString();
        }
        return version;
    }

    /*
     * getAuthor
     * Returns the app's author
     */
    QString AppItem::getAuthor() {
        QString author;
        if (m_jsonApp.contains("author") && m_jsonApp.value("author").isString()) {
            author = m_jsonApp.value("author").toString();
        }
        return author;
    }

    /*
     * getContributors
     * Returns the app's contributors
     */
    QVector<QString> AppItem::getContributors() {
        QVector<QString> contributors;
        if (m_jsonApp.contains("contributors") && m_jsonApp.value("contributors").isArray()) {
            QJsonArray contributorsJsonArray = m_jsonApp.value("contributors").toArray();
            for (auto jsonItem: contributorsJsonArray) {
                if (jsonItem.isString() && !jsonItem.toString().isEmpty()) {
                    contributors.append(jsonItem.toString());
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
        if (m_jsonApp.contains("copyright") && m_jsonApp.value("copyright").isString()) {
            copyright = m_jsonApp.value("copyright").toString();
        }
        return copyright;
    }

    /*
     * getCopyright
     * Returns the app's vendor
     */
    QString AppItem::getVendor() {
        QString vendor;
        if (m_jsonApp.contains("vendor") && m_jsonApp.value("vendor").isString()) {
            vendor = m_jsonApp.value("vendor").toString();
        }
        return vendor;
    }

    /*
     * getLicense
     * Returns the app's license
     */
    QString AppItem::getLicense() {
        QString license;
        if (m_jsonApp.contains("license") && m_jsonApp.value("license").isString()) {
            license = m_jsonApp.value("license").toString();
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
            url.replace("http:///", FairWindSK::getInstance()->getSignalKServerUrl());

        } else if (url.startsWith("https:///")) {

            // Create the url string substituting the placeholder with the server url
            url.replace("https:///", FairWindSK::getInstance()->getSignalKServerUrl());

        } if (url.startsWith("http://") || url.startsWith("https://")) {



        } else {
            url = FairWindSK::getInstance()->getSignalKServerUrl() + "/" + url + "/";
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
        return int(m_jsonApp["fairwind"].toObject()["order"].toDouble()) < int(o.m_jsonApp["fairwind"].toObject()["order"].toDouble());
    }

    QStringList AppItem::getArguments() {
        QStringList result;
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairWindJsonObject = m_jsonApp["fairwind"].toObject();
            qDebug() << fairWindJsonObject.keys();

            if (fairWindJsonObject.contains("arguments") && fairWindJsonObject["arguments"].isArray()) {
                auto argumentsJsonArray = fairWindJsonObject["arguments"].toArray();
                for (auto argument: argumentsJsonArray) {
                    if (argument.isString()) {
                        result.append(argument.toString());
                    }
                }
            }
        }
        return result;
    }

    QString AppItem::getAppIcon() {
        QString result = "";
        if (m_jsonApp.contains("signalk") &&  m_jsonApp.value("signalk").isObject()) {
            auto signalkJsonObject = m_jsonApp.value("signalk").toObject();
            if (signalkJsonObject.contains("appIcon") && signalkJsonObject.value("appIcon").isString()) {
                result = signalkJsonObject.value("appIcon").toString();
            }
        }
        return result;
    }

    QString AppItem::getSettings(QString pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"].toObject();
            if (fairwindJsonObject.contains("settings") && fairwindJsonObject["settings"].isString()) {
                result = fairwindJsonObject["settings"].toString();
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

    QString AppItem::getAbout(QString pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"].toObject();
            if (fairwindJsonObject.contains("about") && fairwindJsonObject["about"].isString()) {
                result = fairwindJsonObject["about"].toString();
            }
        }
        return result;
    }

    QString AppItem::getHelp(QString pluginUrl) {
        QString result = "";
        if (m_jsonApp.contains("fairwind") && m_jsonApp["fairwind"].isObject()) {
            auto fairwindJsonObject = m_jsonApp["fairwind"].toObject();
            if (fairwindJsonObject.contains("help") && fairwindJsonObject["help"].isString()) {
                result = fairwindJsonObject["help"].toString();
            }
        }
        return result;
    }
}
