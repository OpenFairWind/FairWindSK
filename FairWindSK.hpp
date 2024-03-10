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

        bool startSignalK();
        void loadConfig();
        void loadApps();

        AppItem *getAppItemByHash(QString hash);
        QString getAppHashById(QString appId);
        QList<QString> getAppsHashes();

        bool useVirtualKeyboard();

        signalk::Document *getSignalKDocument();

        QString getSignalKServerUrl();
        QString getUsername();
        QString getPassword();

        bool isDebug();

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

        bool m_debug;
    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP