//
// Created by Raffaele Montella on 03/06/21.
//

#include <QNetworkReply>
#include <QCoreApplication>
#include <QJsonDocument>

#include <SignalKClient.hpp>
#include "FairWindSK.hpp"

namespace fairwindsk {
/*
 * SignalKAPIClient - Public Constructor
 */
    SignalKClient::SignalKClient(QObject *parent) :
            QObject(parent) {

        mUrl = "http://172.24.1.1:3000/signalk";
        mVersion = "v1";
        mActive = false;
        mDebug = false;
        mRestore = true;
        mLabel = "Signal K Connection";
        mUsername = "admin";
        mPassword = "password";
    }

/*
 * ~SignalKAPIClient - Destructor
 */
    SignalKClient::~SignalKClient() = default;

/*
 * onInit
 * Initialization method
 */
    bool SignalKClient::init(QMap<QString, QVariant> params) {
        bool result = false;

        auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto signalKDocument = fairWindSK->getSignalKDocument();

        connect(signalKDocument, &signalk::Document::created, this, &SignalKClient::onCreated);
        connect(signalKDocument, &signalk::Document::updated, this, &SignalKClient::onUpdated);
        connect(signalKDocument, &signalk::Document::fetched, this, &SignalKClient::onFetched);

        connect(&mWebSocket, &QWebSocket::connected, this, &SignalKClient::onConnected);
        connect(&mWebSocket, &QWebSocket::disconnected, this, &SignalKClient::onDisconnected);

        if (params.contains("url")) {
            mUrl = params["url"].toString();
        }

        if (params.contains("version")) {
            mVersion = params["version"].toString();
        }

        if (params.contains("label")) {
            mLabel = params["label"].toString();
        }

        if (params.contains("active")) {
            mActive = params["active"].toBool();
        }

        if (params.contains("debug")) {
            mDebug = params["debug"].toBool();
        }

        if (params.contains("restore")) {
            mRestore = params["restore"].toBool();
        }


        if (params.contains("username")) {
            mUsername = params["username"].toString();
        }

        if (params.contains("password")) {
            mPassword = params["password"].toString();
        }

        // Check if the self is set manually
        QString self;
        if (params.contains("self")) {
            self = params["self"].toString();
        }

        if (mDebug)
            qDebug() << "SignalKAPIClient::onInit(" << params << ")";

        if (mActive) {

            mServer = signalkGet(mUrl);

            if (mDebug) {
                qDebug() << "Server: " << mServer;
            }

            if (login()) {

                if (mRestore) {
                    QJsonObject allSignalK = getAll();
                    fairWindSK->getSignalKDocument()->insert("", allSignalK);

                    if (mDebug)
                        qDebug() << allSignalK;
                }

                // Check if the self is defined
                if (!self.isEmpty()) {

                    // Override the self definition
                    signalKDocument->setSelf(self);
                }


                mWebSocket.open(QUrl(ws()));

                result = true;
            } else {
                if (mDebug)
                    qDebug() << "Login failed.";
            }
        }  else {
            if (mDebug)
                    qDebug() << "Data connection not active!";

        }

        return result;
    }


/*
 * getSelf
 * Returns the self key
 */
    QString SignalKClient::getSelf() {
        auto result = httpGet(http()+"self");

        if (mDebug)
            qDebug() << "SignalKClient::getSelf :" << result;

        return result.replace("\"","");
    }

    QJsonObject SignalKClient::getAll() {
        return signalkGet(http());
    }


    QJsonValue SignalKClient::onCreated(const QString &path, const QJsonValue &newValue) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        return signalkPost(http()+processedPath,newValue.toObject());
    }

    QJsonValue SignalKClient::onUpdated(const QString &path, const QJsonValue &newValue) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        return signalkPut(http()+processedPath,newValue.toObject());
    }

    QJsonValue SignalKClient::onFetched(const QString &path) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        return signalkGet(http()+processedPath);
    }

    QJsonObject SignalKClient::signalkGet(QString url) {
        auto data = httpGet(url);
        return QJsonDocument::fromJson(data).object();
    }

    QJsonObject SignalKClient::signalkPut(QString url, QJsonObject payload) {
        if (!mToken.isEmpty()) {
            payload["token"] = mToken;
        }

        if (mDebug)
            qDebug() << "SignalKClient::signalkPut payload: " << payload;

        auto data = httpPut(url,payload);
        return QJsonDocument::fromJson(data).object();
    }

    QJsonObject SignalKClient::signalkPost(QString url, QJsonObject payload) {
        if (!mToken.isEmpty()) {
            payload["token"] = mToken;
        }

        if (mDebug)
            qDebug() << "SignalKClient::signalkPost payload: " << payload;

        auto data = httpPost(url, payload);
        return QJsonDocument::fromJson(data).object();
    }

    /*
 * httpGet
 * Executes a http get request
 */
    QByteArray SignalKClient::httpGet(QString url) {
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!mCookie.isEmpty()) {
            req.setRawHeader("Cookie", mCookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(manager.get(req));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {

            if (mDebug)
                qDebug() << "Failure" << reply->errorString();
        }
        QByteArray data = reply->readAll();
        return data;
    }

    /*
 * httpPost
 * Executes a http post request
 */
    QByteArray SignalKClient::httpPost(QString url, QJsonObject payload) {

        if (mDebug)
            qDebug() << "SignalKClient::httpPost url: " << url << " payload: " << payload;

        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!mCookie.isEmpty()) {
            req.setRawHeader("Cookie", mCookie.toLatin1());

            if (mDebug)
                qDebug() << "SignalKClient::httpPost cookie: " << mCookie.toLatin1();
        }

        QScopedPointer<QNetworkReply> reply(manager.post(req, jsonDocument.toJson()));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (mDebug)
                qDebug() << "Failure" << reply->errorString();
        }
        QByteArray data = reply->readAll();

        auto cookie = reply->rawHeader("Set-Cookie");
        if (cookie!= nullptr && cookie.size()>0) {
            mCookie = cookie;

            if (mDebug)
                qDebug() << "mCookie: " <<mCookie;
        }

        if (mDebug) {
            QList<QByteArray> headerList = reply->rawHeaderList();
                    foreach(QByteArray head, headerList) {
                    qDebug() << head << ":" << reply->rawHeader(head);
                }
        }

        return data;
    }

    /*
 * httpPost
 * Executes a http post request
 */
    QByteArray SignalKClient::httpPut(QString url, QJsonObject payload) {

        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!mCookie.isEmpty()) {
            req.setRawHeader("Cookie", mCookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(manager.put(req, jsonDocument.toJson()));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Failure" << reply->errorString();
        }
        QByteArray data = reply->readAll();
        return data;
    }

    bool SignalKClient::login() {
        bool result = false;
        QJsonObject payload;
        payload["username"] = mUsername;
        payload["password"] = mPassword;
        QJsonObject data = signalkPost(mUrl + "/" + mVersion + "/auth/login", payload);

        if (mDebug)
            qDebug() << "SignalKClient::login : " << data;

        if (data.contains("token") && data["token"].isString()) {
            mToken = data["token"].toString();

            if (mDebug)
                qDebug() << "SignalKClient::login token: " << mToken;

            result = true;
        } else if (data.contains("message") && data["message"].isString() ){

            if (mDebug)
                qDebug() << data["message"].toString();
        }

        return result;
    }

/*
        {
 "endpoints": {
   "v1": {
     "version": "1.41.3",
     "signalk-http": "http://localhost:3000/signalk/v1/api/",
     "signalk-ws": "ws://localhost:3000/signalk/v1/stream",
     "signalk-tcp": "tcp://localhost:8375"
   }
 },
 "server": {
   "id": "signalk-server-node",
   "version": "1.41.3"
 }
}

        */

    QString SignalKClient::http() {
        return getEndpointByProtocol("http");
    }

    QString SignalKClient::ws() {
        return getEndpointByProtocol("ws");
    }

    QString SignalKClient::tcp() {
        return getEndpointByProtocol("tcp");
    }

    QString SignalKClient::getEndpointByProtocol(const QString &protocol) {
        if (mServer.contains("endpoints") & mServer["endpoints"].isObject()) {
            auto jsonObjectEndponts = mServer["endpoints"].toObject();
            if (jsonObjectEndponts.contains(mVersion) && jsonObjectEndponts[mVersion].isObject()) {
                auto jsonObjectVersion = jsonObjectEndponts[mVersion].toObject();
                if (jsonObjectVersion.contains("signalk-" + protocol) &&
                    jsonObjectVersion["signalk-" + protocol].isString()) {
                    return jsonObjectVersion["signalk-" + protocol].toString();
                }
            }
        }
        return {};
    }

//! [onConnected]
    void SignalKClient::onConnected() {
        if (mDebug)
            qDebug() << "WebSocket connected";
        connect(&mWebSocket, &QWebSocket::textMessageReceived,
                this, &SignalKClient::onTextMessageReceived);

    }
//! [onConnected]

//! [onDisconnected]
    void SignalKClient::onDisconnected() {
        if (mDebug)
            qDebug() << "WebSocket disconnected";
    }
//! [onDisconnected]

//! [onTextMessageReceived]
    void SignalKClient::onTextMessageReceived(QString message) {
        QJsonObject jsonObjectUpdate;

        QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());

        // check validity of the document
        if (!jsonDocument.isNull()) {
            if (jsonDocument.isObject()) {
                jsonObjectUpdate = jsonDocument.object();
            }
        }

        if (mDebug)
            qDebug() << "Update received:" << message;

        auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        fairWindSK->getSignalKDocument()->update(jsonObjectUpdate);

    }

    QString SignalKClient::getToken() {
        return mToken;
    }

    QString SignalKClient::getCookie() {
        return mCookie;
    }
//! [onTextMessageReceived]
}