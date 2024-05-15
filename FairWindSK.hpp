//
// Created by Raffaele Montella on 03/04/21.
//

#ifndef FAIRWINDSK_FAIRWINDSK_HPP
#define FAIRWINDSK_FAIRWINDSK_HPP

#include <QString>
#include <QList>
#include <QMap>
#include <QString>
#include <nlohmann/json.hpp>
#include <QWebEngineProfile>

#include "AppItem.hpp"
#include "signalk/Client.hpp"
#include "Configuration.hpp"

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

        Configuration *getConfiguration();
        void setConfiguration(Configuration *configuration);


        bool startSignalK();
        bool loadApps();

        AppItem *getAppItemByHash(const QString& hash);
        QString getAppHashById(const QString& appId);
        QList<QString> getAppsHashes();

        QWebEngineProfile *getWebEngineProfile();

        bool isDebug() const;

        signalk::Client *getSignalKClient();

        void loadConfig();

        ~FairWindSK();

    private:

        QWebEngineProfile *m_profile;

        QMap<QString, AppItem *> m_mapHash2AppItem;
        QMap<QString, QString> m_mapAppId2Hash;


        FairWindSK();
        inline static FairWindSK *m_instance = nullptr;



        signalk::Client m_signalkClient;


        bool m_debug;

        QString m_configFilename;
        Configuration m_configuration;


    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP