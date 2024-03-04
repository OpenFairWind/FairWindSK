//
// Created by Raffaele Montella on 03/04/21.
//

#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <utility>
#include <QJsonArray>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>


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


/*
 * getConfig
 * Returns the configuration infos
 */
    QJsonObject FairWindSK::getConfig() {

        // Define the object
        QJsonObject config;

        // Return the config object
        return config;
    }

    AppItem *FairWindSK::getAppItemByHash(QString hash) {
        return m_mapHash2AppItem[hash];
    }

    QString FairWindSK::getAppHashById(QString appId) {
        return m_mapAppId2Hash[appId];
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


        QUrl url = QUrl("http://172.24.1.1:3000/skServer/webapps");
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
        return false;
    }

}
