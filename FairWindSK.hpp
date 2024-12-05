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

        // Get the singleton instance
        static FairWindSK *getInstance();

        // Get the configuration
        Configuration *getConfiguration();

        // Set the configuration
        void setConfiguration(Configuration *configuration);

        // Starts the Signal K websocket client
        bool startSignalK();

        // Load the application
        bool loadApps();

        // Get the application item using the application hash
        AppItem *getAppItemByHash(const QString& hash);

        // Get the application hash by the application id
        QString getAppHashById(const QString& appId);

        // Get a list of application hashes
        QList<QString> getAppsHashes();

        // Get the WebEngine profile
        QWebEngineProfile *getWebEngineProfile();

        // Return true if the debug is set in the fairwindsk.ini file
        bool isDebug() const;

        // Get the Signal K client
        signalk::Client *getSignalKClient();

        // Load the configuration from the json file
        void loadConfig();

        // De destructor
        ~FairWindSK() override;

    private:

        // The private constructor
        FairWindSK();


        // Pointer to the WebEngine profile
        QWebEngineProfile *m_profile;

        // The hash/item map
        QMap<QString, AppItem *> m_mapHash2AppItem;

        // The id/hash map
        QMap<QString, QString> m_mapAppId2Hash;

        // The singleton instance
        inline static FairWindSK *m_instance = nullptr;

        // The Signal K client
        signalk::Client m_signalkClient;

        // The debug flag
        bool m_debug;

        // The configuration file name
        QString m_configFilename;

        // The configuration object
        Configuration m_configuration;

    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP