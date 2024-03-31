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
#include "signalk/Client.hpp"

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

        QString getVesselSpeedUnits();
        QString getWindSpeedUnits();
        QString getDistanceUnits();
        QString getDepthUnits();

        bool getVirtualkeyboard();

        QString getSignalKServerUrl();
        void setSignalKServerUrl(QString signalKServerUrl);

        QString getToken();
        void setToken(QString token);

        QString getMyDataApp();
        QString getMOBApp();
        QString getAlarmsApp();
        QString getSettingsApp();

        int getSleep();
        int getRetry();

        bool isDebug() const;

        signalk::Client *getSignalKClient();

        static void setVirtualKeyboard(bool value);
        static bool getVirtualKeyboard();



    private:

        QMap<QString, AppItem *> m_mapHash2AppItem;
        QMap<QString, QString> m_mapAppId2Hash;


        FairWindSK();
        inline static FairWindSK *m_instance = nullptr;



        signalk::Client m_signalkClient;

        QString m_signalKServerUrl;
        QString m_username;
        QString m_password;
        QString m_token;

        int m_mSleep;
        int m_nRetry;

        bool m_debug;

        QJsonObject m_configuration;

        QString getAppNameByKeyFromConfiguration(const QString& key);
    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP