//
// Created by Raffaele Montella on 03/04/21.
//

#ifndef FAIRWINDSK_FAIRWINDSK_HPP
#define FAIRWINDSK_FAIRWINDSK_HPP

#include <QString>
#include <QList>
#include <QMap>
#include <QString>
#include <QColor>
#include <QEvent>
#include <QTimer>
#include <nlohmann/json.hpp>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QWebEngineProfile>
#else
class QObject;
#endif

#include "AppItem.hpp"
#include "signalk/Client.hpp"
#include "Configuration.hpp"

namespace fairwindsk {

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    using WebProfileHandle = QWebEngineProfile;
#else
    using WebProfileHandle = QObject;
#endif

    struct UiScrollPalette {
        QColor track;
        QColor handleTop;
        QColor handleMid;
        QColor handleBottom;
    };

    class AppItem;

    /*
     * FairWind
     * Singleton used to handle the entire FairWind ecosystem in a centralized way
     */
    class FairWindSK: public QObject {
        Q_OBJECT

    public:
        static constexpr quint32 RuntimeUi = 1u << 0;
        static constexpr quint32 RuntimeUnits = 1u << 1;
        static constexpr quint32 RuntimeSignalKConnection = 1u << 2;
        static constexpr quint32 RuntimeApps = 1u << 3;
        static constexpr quint32 RuntimeSignalKPaths = 1u << 4;
        static constexpr quint32 RuntimeAll = RuntimeUi | RuntimeUnits | RuntimeSignalKConnection | RuntimeApps | RuntimeSignalKPaths;

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
        WebProfileHandle *getWebEngineProfile();

        // Return true if the debug is set in the fairwindsk.ini file
        [[nodiscard]] bool isDebug() const;
        bool isAutomaticComfortViewConfigured(const Configuration *configuration = nullptr) const;
        bool isAutomaticComfortViewAvailable(const Configuration *configuration = nullptr) const;
        QString getActiveComfortViewPreset(const Configuration *configuration = nullptr) const;
        UiScrollPalette getActiveComfortScrollPalette(const Configuration *configuration = nullptr) const;

        // Get the Signal K client
        signalk::Client *getSignalKClient();

        // Load the configuration from the json file
        void loadConfig();

        void applyUiPreferences(const Configuration *configuration = nullptr);
        void reconfigureRuntime(quint32 runtimeChanges = RuntimeAll);

        // Check if the Autopilot app is installed
        bool checkAutopilotApp();

        // Check if the anchor app is installed
        bool checkAnchorApp();

        // Destructor
        ~FairWindSK() override;

    private:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void updateWebProfileCookie();
        void refreshAutomaticComfortView();
        void refreshAutomaticComfortViewAvailability(const Configuration *configuration = nullptr);

        // The private constructor
        FairWindSK();

        // Pointer to the WebEngine profile
        WebProfileHandle *m_profile = nullptr;

        // The hash/item map
        QMap<QString, AppItem *> m_mapHash2AppItem;

        // The id/hash map
        QMap<QString, QString> m_mapAppId2Hash;

        // The singleton instance
        inline static FairWindSK *m_instance = nullptr;

        // The Signal K client
        signalk::Client m_signalkClient;

        // The configuration file name
        QString m_configFilename;

        // The configuration object
        Configuration m_configuration;

        // Number of trys in Web Socket connection
        int m_nRetry = 5;

        // Time between trys
        int m_mSleep = 1000;

        // The debug flag
        bool m_debug = false;

        QTimer *m_autoComfortTimer = nullptr;
        QString m_activeComfortViewPreset;
        bool m_automaticComfortViewAvailable = false;

    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP
