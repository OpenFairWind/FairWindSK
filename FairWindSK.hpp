//
// Created by Raffaele Montella on 03/04/21.
//

#ifndef FAIRWINDSK_FAIRWINDSK_HPP
#define FAIRWINDSK_FAIRWINDSK_HPP

#include <QString>
#include <QList>
#include <QMap>
#include <QJsonDocument>
#include "AppItem.hpp"
#include "signalk/Document.hpp"
#include "SignalKClient.hpp"

namespace fairwindsk {

    class AppItem;

    /*
     * FairWind
     * Singleton used to handle the entire FairWind ecosystem in a centralized way
     */
    class FairWindSK: public QObject {
        Q_OBJECT



    public:
        static FairWindSK *getInstance();

        QJsonObject getConfiguration();

        void loadConfig();
        bool startSignalK();
        bool loadApps();

        AppItem *getAppItemByHash(QString hash);
        QString getAppHashById(QString appId);
        QList<QString> getAppsHashes();



        signalk::Document *getSignalKDocument();

        QString getSignalKServerUrl();
        QString getUsername();
        QString getPassword();

        QString getMyDataApp();
        QString getMOBApp();
        QString getAlarmsApp();
        QString getSettingsApp();

        bool isDebug() const;

        SignalKClient *getSignalKClient();

    private:

        QMap<QString, AppItem *> m_mapHash2AppItem;
        QMap<QString, QString> m_mapAppId2Hash;


        FairWindSK();
        inline static FairWindSK *m_instance = nullptr;

        signalk::Document m_signalkDocument;

        SignalKClient m_signalkClient;

        QString m_signalKServerUrl;
        QString m_username;
        QString m_password;

        int m_mSleep;
        int m_nRetry;

        bool m_debug;

        QString getAppNameByKeyFromConfiguration(const QString& key);
    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP