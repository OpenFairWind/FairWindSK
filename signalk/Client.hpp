//
// Created by Raffaele Montella on 03/06/21.
//

#ifndef FAIRWINDSK_SIGNALKCLIENT_HPP
#define FAIRWINDSK_SIGNALKCLIENT_HPP


#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>
#include <QVariantMap>

#include "Waypoint.hpp"
#include "Subscription.hpp"

namespace fairwindsk::signalk {

    //namespace signalk { class Waypoint; }
    class Waypoint;

    class Client : public QObject {
    Q_OBJECT

    public:
        explicit Client(QObject *parent = nullptr);
        ~Client() override;

        bool init(QMap<QString, QVariant> params);

        QJsonObject subscribe(const QString& path, QObject *receiver, const char *member, int period = 1000, const QString& policy = "ideal", int minPeriod = 200);
        QJsonObject subscribe(const QString& context, const QString& path, QObject *receiver, const char *member, int period = 1000, const QString& policy = "ideal", int minPeriod = 200);
        void subscribeStream(const QString& context, const QString& path, QObject *receiver, const char *member, int period = 1000, const QString& policy = "ideal", int minPeriod = 200);
        void removeSubscription(const QString& path, QObject *receiver);

        QString getSelf();

        QJsonObject getAll();

        bool login();
        QUrl url() const;
        QUrl server() const;
        QUrl http(const QString& version = "v1");
        QUrl ws(const QString& version = "v1");
        QUrl tcp(const QString& version = "v1");

        QJsonObject signalkGet(const QString& path);
        QJsonObject signalkGet(const QUrl& url);
        QJsonObject signalkGet(const QString& path,  QString& payload);
        QJsonObject signalkGet(const QString& path,  QJsonObject& payload);
        QJsonObject signalkGet(const QUrl& url,  QString& payload);
        QJsonObject signalkGet(const QUrl& url,  QJsonObject& payload);

        QJsonObject signalkPost(const QString& path);
        QJsonObject signalkPost(const QUrl& url);
        QJsonObject signalkPost(const QString& path,  QString& payload);
        QJsonObject signalkPost(const QString& path,  QJsonObject& payload);
        QJsonObject signalkPost(const QUrl& url,  QString& payload);
        QJsonObject signalkPost(const QUrl& url,  QJsonObject& payload);

        QJsonObject signalkPut(const QString& path);
        QJsonObject signalkPut(const QUrl& url);
        QJsonObject signalkPut(const QString& path,  QString& payload);
        QJsonObject signalkPut(const QString& path,  QJsonObject& payload);
        QJsonObject signalkPut(const QUrl& url,  QString& payload);
        QJsonObject signalkPut(const QUrl& url,  QJsonObject& payload);

        QJsonObject signalkDelete(const QString& path);
        QJsonObject signalkDelete(const QUrl& url);
        QJsonObject signalkDelete(const QString& path, const QString& payload);
        QJsonObject signalkDelete(const QString& path,  QJsonObject& payload);
        QJsonObject signalkDelete(const QUrl& url, const QString& payload);
        QJsonObject signalkDelete(const QUrl& url,  QJsonObject& payload);

        QString getToken();

        qint64 sendMessage(QJsonObject message);

        Waypoint getWaypointByHref(const QString &href);
        QMap<QString, Waypoint> getWaypoints();
        QMap<QString, QJsonObject> getResources(const QString &collection, const QVariantMap &query = {});
        QJsonObject getResource(const QString &collection, const QString &id, const QVariantMap &query = {});
        QJsonObject createResource(const QString &collection, const QJsonObject &payload, const QVariantMap &query = {});
        QJsonObject putResource(const QString &collection, const QString &id, const QJsonObject &payload);
        bool deleteResource(const QString &collection, const QString &id);
        bool navigateToWaypoint(const QString &href);
        QJsonArray getHistoryPaths(const QVariantMap &query = {});
        QJsonObject getHistoryValues(const QStringList &paths, const QVariantMap &query = {});
        QJsonObject getUnitPreferencesActive();
        QJsonObject getUnitPreferencesConfig();
        QJsonDocument getUnitPreferencesPresets();
        QJsonObject getUnitPreferencesPreset(const QString &name);
        QJsonObject getUnitPreferencesCategories();
        QJsonObject getUnitPreferencesDefinitions();
        QJsonObject getUnitPreferencesDefaultCategories();
        QJsonObject putUnitPreferencesCustomPreset(const QString &name, const QJsonObject &payload);
        bool deleteUnitPreferencesCustomPreset(const QString &name);
        QJsonObject getPathMeta(const QString &path, const QString &context = QStringLiteral("vessels/self"));
        bool isRestHealthy() const;
        bool isStreamHealthy() const;
        QString connectionStatusText() const;
        void beginPlannedServerRestart(int gracePeriodMs = 45000);

        static QString getStringFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static double getDoubleFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QJsonObject getObjectFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QDateTime  getDateTimeFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QGeoCoordinate getGeoCoordinateFromUpdateByPath(const QJsonObject &update, const QString& path = "");

        static QString currentISO8601TimeUTC();

    public slots:

        void unsubscribe(QObject *receiver);

    signals:
        void requestActivityChanged(bool active);
        void requestCountChanged(int activeRequests);
        void serverHealthChanged(bool healthy, const QString &statusText);
        void connectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void serverMessageChanged(const QString &message);
        void serverStateResynchronized(bool recoveredFromDisconnect);

    private slots:
        void onConnected();
        void onDisconnected();
        void onTextMessageReceived(QString message);
        void onStreamHealthTimeout();
        void attemptReconnect();
        void finishPlannedRestartGrace();



    private:
        static constexpr int kRequestTimeoutMs = 10000;

        QString m_Username;
        QString m_Password;
        QString m_Token;
        QString m_Cookie;



        QWebSocket m_WebSocket;

        QNetworkAccessManager m_NetworkAccessManager;
        QUrl m_Url;

        bool m_Debug;
        bool m_Active;
        QString m_Label;

        QNetworkRequest createJsonRequest(const QUrl& url) const;
        QByteArray finishReply(QNetworkReply *reply, bool updateCookie = false, bool *success = nullptr, QString *message = nullptr, int *httpStatus = nullptr) const;
        QByteArray httpGet(const QUrl& url);
        QByteArray httpGet(const QUrl& url, const QJsonObject& payload);
        QByteArray httpPost(const QUrl& url, const QJsonObject& payload);
        QByteArray httpPut(const QUrl& url, const QJsonObject& payload);
        QByteArray httpDelete(const QUrl& url, const QJsonObject& payload);
        void beginRequest(const QString &method, const QUrl &url);
        void endRequest(bool success, const QUrl &url = {}, int httpStatus = 0, const QString &message = QString());
        QString discoveryMessage() const;
        bool shouldSuppressServerMessage(const QUrl &url, int httpStatus) const;
        void setRestHealth(bool healthy, const QString &statusText = QString());
        void setStreamHealth(bool healthy, const QString &statusText = QString());
        void emitConnectivityState(const QString &statusText = QString());
        void markStreamActivity(const QString &statusText = QString());
        QString normalizedSubscriptionContext(const QString &context) const;
        QString subscriptionMessage(const QString &context, const QString &path, int period, const QString &policy, int minPeriod) const;
        bool refreshServerDiscovery();
        bool applyDiscoveredServer(const QJsonObject &discoveredServer);
        void refreshAuthenticationState();
        void openWebSocket();
        void scheduleReconnect(int delayMs = 2000);
        void startAsyncReconnectDiscovery();
        bool hasSubscription(const QString &requestedContext, const QString &path, QObject *receiver) const;
        void resubscribeAll(bool hydrateSnapshots);
        QString serverFingerprint(const QJsonObject &server) const;
        QJsonObject buildDeltaUpdate(const QString &context, const QString &path, const QJsonValue &value) const;

        QUrl getEndpointByProtocol(const QString &protocol, const QString& version = "v1");
        QJsonDocument getJsonDocument(const QUrl &url, const QJsonObject &payload = {});
        QUrl withQuery(const QUrl &url, const QVariantMap &query) const;
        QUrl resourceUrl(const QString &collection, const QString &id = QString(), const QVariantMap &query = {}) const;
        QUrl historyUrl(const QString &suffix = QString(), const QVariantMap &query = {}) const;

        QJsonObject m_Server;

        QList<Subscription> m_subscriptions;
        int m_activeRequests = 0;
        bool m_restHealthy = false;
        bool m_streamHealthy = false;
        QDateTime m_lastStreamActivity;
        QTimer m_streamHealthTimer;
        QTimer m_reconnectTimer;
        QMap<QString, QVariant> m_connectionParams;
        QString m_serverFingerprintValue;
        bool m_hadStreamConnection = false;
        bool m_reconnectRecoveryPending = false;
        bool m_reconnectAttemptInFlight = false;
        bool m_plannedRestartInProgress = false;
        QTimer m_plannedRestartTimer;
    };
}

#endif //FAIRWINDSK_SIGNALKCLIENT_HPP
