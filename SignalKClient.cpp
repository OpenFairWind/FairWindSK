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

        mUrl = "";
        mVersion = "v1";
        mActive = false;
        mDebug = false;
        mRestore = true;
        mLabel = "Signal K Connection";
        mUsername = "";
        mPassword = "";
        mToken = "";
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

        // Get the FairWindSK instance
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get the Signal K document instance
        auto signalKDocument = fairWindSK->getSignalKDocument();

        // Connect the on created event
        connect(signalKDocument, &signalk::Document::created, this, &SignalKClient::onCreated);

        // Connect the on updated event
        connect(signalKDocument, &signalk::Document::updated, this, &SignalKClient::onUpdated);

        // Connect the on fetched event
        connect(signalKDocument, &signalk::Document::fetched, this, &SignalKClient::onFetched);

        // Connect the on connected event
        connect(&mWebSocket, &QWebSocket::connected, this, &SignalKClient::onConnected);

        // Connect the on disconnected event
        connect(&mWebSocket, &QWebSocket::disconnected, this, &SignalKClient::onDisconnected);

        // Check if the url is present in parameters
        if (params.contains("url")) {

            // Set the url
            mUrl = params["url"].toString();
        }

        // Check if the version is present in parameters
        if (params.contains("version")) {

            // Set the version
            mVersion = params["version"].toString();
        }

        // Check if the label is present in parameters
        if (params.contains("label")) {

            // Set the label
            mLabel = params["label"].toString();
        }

        // Check if the active is present in parameters
        if (params.contains("active")) {

            // Set the active
            mActive = params["active"].toBool();
        }

        // Check if the debug is present in parameters
        if (params.contains("debug")) {

            // Set the debug
            mDebug = params["debug"].toBool();
        }

        // Check if the restore is present in parameters
        if (params.contains("restore")) {

            // Set the restore
            mRestore = params["restore"].toBool();
        }

        // Check if the token is present in parameters
        if (params.contains("token")) {

            // Set the token
            mToken = params["token"].toString();
        }

        // Check if the username is present in parameters
        if (params.contains("username")) {

            // Set the username
            mUsername = params["username"].toString();
        }

        // Check if the password is present in parameters
        if (params.contains("password")) {

            // Set the password
            mPassword = params["password"].toString();
        }

        if (mDebug)
            qDebug() << "SignalKAPIClient::onInit(" << params << ")";

        if (mActive) {

            mServer = signalkGet(mUrl);

            if (mDebug) {
                qDebug() << "Server: " << mServer;
            }

            // Check if the token is empty
            if (mToken.isEmpty()) {

                // Perform the login
                login();
            }

            // Check if the token is not empty
            if (!mToken.isEmpty()) {

                mCookie="JAUTHENTICATION="+mToken+"; Path=/; HttpOnly";


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