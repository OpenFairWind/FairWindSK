//
// Created by Raffaele Montella on 03/06/21.
//

#ifndef FAIRWINDSK_SIGNALKCLIENT_HPP
#define FAIRWINDSK_SIGNALKCLIENT_HPP

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QGeoCoordinate>

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
        void removeSubscription(const QString& path, QObject *receiver);

        QString getSelf();

        QJsonObject getAll();

        bool login();
        QUrl url();
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
        QJsonObject signalkDelete(const QString& path,  QString& payload);
        QJsonObject signalkDelete(const QString& path,  QJsonObject& payload);
        QJsonObject signalkDelete(const QUrl& url,  QString& payload);
        QJsonObject signalkDelete(const QUrl& url,  QJsonObject& payload);

        QString getToken();

        qint64 sendMessage(QJsonObject message);

        Waypoint getWaypointByHref(const QString &href);

        static QString getStringFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static double getDoubleFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QJsonObject getObjectFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QDateTime  getDateTimeFromUpdateByPath(const QJsonObject &update, const QString& path = "");
        static QGeoCoordinate getGeoCoordinateFromUpdateByPath(const QJsonObject &update, const QString& path = "");

        static QString currentISO8601TimeUTC();

    public slots:

        void unsubscribe(QObject *receiver);

    private slots:
        void onConnected();
        void onDisconnected();
        void onTextMessageReceived(QString message);



    private:
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

        QByteArray httpGet(const QUrl& url);
        QByteArray httpGet(const QUrl& url, const QJsonObject& payload);
        QByteArray httpPost(const QUrl& url, const QJsonObject& payload);
        QByteArray httpPut(const QUrl& url, const QJsonObject& payload);
        QByteArray httpDelete(const QUrl& url, const QJsonObject& payload);

        QUrl getEndpointByProtocol(const QString &protocol, const QString& version = "v1");

        QJsonObject m_Server;

        QList<Subscription> m_subscriptions;
    };
}

#endif //FAIRWINDSK_SIGNALKCLIENT_HPP