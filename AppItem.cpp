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
        this->m_active = app.m_active;
        this->m_order = app.m_order;

    }

/*
 * Public Constructor
 */
    AppItem::AppItem(QJsonObject jsonApp, bool active, int order) {
        // Get the app's infos and store them for future usage
        m_jsonApp = std::move(jsonApp);
        m_active = active;
        m_order = order;

    }

/*
     * setOrder
     * Sets the app's order
     */
    void AppItem::setOrder(int order) {
        m_order = order;
    }

    /*
     * getOrder
     * Returns the app's order
     */
    int AppItem::getOrder() {
        return m_order;
    }

    /*
     * setActive
     * Sets the app's active state
     */
    void AppItem::setActive(bool active) {
        m_active = active;
    }

    /*
     * getActive
     * Returns the app's active state
     */
    bool AppItem::getActive() {
        return m_active;
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
        if (m_jsonApp.contains("signalk") &&  m_jsonApp.value("signalk").isObject()) {
            auto signalkJsonObject = m_jsonApp.value("signalk").toObject();
            if (signalkJsonObject.contains("appIcon") && signalkJsonObject.value("appIcon").isString()) {
                auto appIcon = signalkJsonObject.value("appIcon").toString();

                if (!appIcon.isEmpty()) {
                    // Get the icon URL
                    QString url = FairWindSK::getInstance()->getSignalKServerUrl() + "/" + getName() + "/" + appIcon;

                    qDebug() << url;

                    QNetworkAccessManager nam;
                    QEventLoop loop;
                    QObject::connect(&nam,&QNetworkAccessManager::finished,&loop,&QEventLoop::quit);
                    QNetworkReply *reply = nam.get(QNetworkRequest(url));
                    loop.exec();

                    pixmap.loadFromData(reply->readAll());

                    delete reply;
                }

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
        QString url;
        url = FairWindSK::getInstance()->getSignalKServerUrl() + "/" + getName() + "/";
        return url;
    }

    void AppItem::setWeb(ui::web::Web *pWeb) {
        m_pWeb = pWeb;
    }

    ui::web::Web *AppItem::getWeb() {
        return m_pWeb;
    }

    bool AppItem::operator<(const AppItem &o) const {
        return std::tie(m_order) < std::tie(o.m_order);
    }
}
