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

        QString getSelf();

        QJsonObject getAll();

        bool login();

        QUrl http();
        QUrl ws();
        QUrl tcp();

        QJsonObject signalkGet(const QString& path);
        QJsonObject signalkPost(const QString& path, const QJsonObject& payload);
        QJsonObject signalkPut(const QString& path, const QJsonObject& payload);

        QJsonObject signalkGet(const QUrl& url);
        QJsonObject signalkPost(const QUrl& url, const QJsonObject& payload);
        QJsonObject signalkPut(const QUrl& url, const QJsonObject& payload);

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
        QString mUsername;
        QString mPassword;
        QString mToken;
        QString mCookie;



        QWebSocket mWebSocket;

        QNetworkAccessManager manager;
        QUrl mUrl;

        bool mDebug;
        bool mActive;
        QString mLabel;

        QByteArray httpGet(const QUrl& url);
        QByteArray httpPost(const QUrl& url, const QJsonObject& payload);
        QByteArray httpPut(const QUrl& url, const QJsonObject& payload);

        QUrl getEndpointByProtocol(const QString &protocol);

        QJsonObject mServer;

        QList<Subscription> subscriptions;
    };
}

#endif //FAIRWINDSK_SIGNALKCLIENT_HPP