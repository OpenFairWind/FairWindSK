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
#include <QPointer>
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
        enum class RuntimeHealthState {
            Disconnected,
            Connecting,
            ConnectedLive,
            ConnectedStale,
            Reconnecting,
            RestDegraded,
            StreamDegraded,
            AppsLoading,
            AppsStale,
            ForegroundAppDegraded
        };
        Q_ENUM(RuntimeHealthState)

        enum class AppsState {
            Idle,
            Loading,
            Loaded,
            Failed,
            Stale
        };
        Q_ENUM(AppsState)

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
        bool stopSignalK();

        // Load the application
        bool loadApps();
        void reloadAppsAsync();

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
        AppsState appsState() const;
        QString appsStateText() const;
        RuntimeHealthState runtimeHealthState() const;
        QString runtimeHealthSummary() const;
        QString runtimeHealthBadgeText() const;
        void setForegroundAppHealth(const QString &summary, bool degraded);
        void clearForegroundAppHealth();

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

    signals:
        void appsReloadStarted();
        void appsReloadFinished(bool success);
        void appsStateChanged(AppsState state, const QString &stateText);
        void runtimeHealthChanged(RuntimeHealthState state, const QString &summary, const QString &badgeText);

    private:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void updateWebProfileCookie();
        void applyWebProfileLocalization();
        void refreshAutomaticComfortView();
        void refreshAutomaticComfortViewAvailability(const Configuration *configuration = nullptr);
        void refreshRuntimeHealth();
        bool rebuildAppRegistry(const nlohmann::json *appsPayload = nullptr);
        void setAppsState(AppsState state, const QString &stateText = QString());
        void startAppsRequest(const QUrl &url, quint64 generation, bool fallbackRequest);
        void finalizeAppsReload(bool success, const QString &statusText, const nlohmann::json *appsPayload = nullptr);
        void handleAutomaticComfortEnvironmentUpdate(const QJsonObject &update);

    private slots:
        void onAutomaticComfortEnvironmentUpdate(const QJsonObject &update);

    private:
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
        QString m_lastUiMetricsSignature;
        QString m_lastUiThemeSignature;
        class QNetworkAccessManager *m_runtimeNetworkAccessManager = nullptr;
        QPointer<class QNetworkReply> m_appsReply;
        quint64 m_appsReloadGeneration = 0;
        AppsState m_appsState = AppsState::Idle;
        QString m_appsStateText = QStringLiteral("Apps idle");
        RuntimeHealthState m_runtimeHealthState = RuntimeHealthState::Disconnected;
        QString m_runtimeHealthSummary = QStringLiteral("Signal K disconnected");
        QString m_runtimeHealthBadgeText = QStringLiteral("DISC");
        QString m_foregroundAppHealthSummary;
        bool m_foregroundAppDegraded = false;
        QString m_automaticComfortViewPath;
        QJsonObject m_automaticComfortEnvironmentUpdate;

    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP
