//
// Created by Raffaele Montella on 03/06/21.
//

#include <limits>
#include <iomanip>
#include <sstream>

#include <QApplication>
#include <QMutableListIterator>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUUid>

#include "Client.hpp"
#include "FairWindSK.hpp"
#include "Waypoint.hpp"

namespace fairwindsk::signalk {
/*
 * SignalKAPIClient - Public Constructor
 */
    Client::Client(QObject *parent) :
            QObject(parent) {

        mUrl = "";
        //mVersion = "v1";
        mActive = false;
        mDebug = false;

        mLabel = "Signal K Connection";
        mUsername = "";
        mPassword = "";
        mToken = "";
    }

/*
 * ~SignalKAPIClient - Destructor
 */
    Client::~Client() = default;

/*
 * onInit
 * Initialization method
 */
    bool Client::init(QMap<QString, QVariant> params) {
        bool result = false;

        // Get the FairWindSK instance
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();



        // Connect the on connected event
        connect(&mWebSocket, &QWebSocket::connected, this, &Client::onConnected);

        // Connect the on disconnected event
        connect(&mWebSocket, &QWebSocket::disconnected, this, &Client::onDisconnected);

        // Check if the url is present in parameters
        if (params.contains("url")) {

            // Set the url
            mUrl = params["url"].toString();
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

            if (!mServer.isEmpty()) {

                // Check if the token is empty
                if (mToken.isEmpty()) {

                    // Perform the login
                    login();
                }

                // Check if the token is not empty
                if (!mToken.isEmpty()) {

                    mCookie = "JAUTHENTICATION=" + mToken + "; Path=/; HttpOnly";

                    mWebSocket.open(QUrl(ws().toString() + "?subscribe=none"));

                    result = true;
                } else {
                    if (mDebug)
                        qDebug() << "Login failed.";
                }
            } else {
                if (mDebug)
                    qDebug() << "Server: " << mUrl << " not available!";
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
    QString Client::getSelf() {
        auto result = httpGet(QUrl(http().toString()+"self"));

        if (mDebug)
            qDebug() << "SignalKClient::getSelf :" << result;

        return result.replace("\"","");
    }

    QJsonObject Client::getAll() {
        if (mDebug) {
            qDebug() << http();
        }

        return signalkGet(http());
    }




    QJsonObject Client::signalkGet(const QString& path) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        auto url = QUrl(http().toString()+processedPath);
        // qDebug() << "signalkGet: " << path << " --> " << url;
        return signalkGet(url);
    }

    QJsonObject Client::signalkPost(const QString& path, const QJsonObject& payload) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        auto url = QUrl(http().toString()+processedPath);
        // qDebug() << "signalkPost: " << path << " --> " << url;
        return signalkPost(url,payload);
    }

    QJsonObject Client::signalkPut(const QString& path, const QJsonObject& payload) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        auto url = QUrl(http().toString()+processedPath);
        //qDebug() << "signalkPut " << path << " --> " << url;
        return signalkPut(url,payload);
    }

    QJsonObject Client::signalkGet(const QUrl& url) {
        auto data = httpGet(url);
        return QJsonDocument::fromJson(data).object();
    }

    QJsonObject Client::signalkPut(const QUrl& url, const QJsonObject& payload) {
        if (!mToken.isEmpty()) {
            payload["token"] = mToken;
        }

        if (mDebug)
            qDebug() << "SignalKClient::signalkPut payload: " << payload;

        auto data = httpPut(url,payload);
        return QJsonDocument::fromJson(data).object();
    }

    QJsonObject Client::signalkPost(const QUrl& url, const QJsonObject& payload) {
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
    QByteArray Client::httpGet(const QUrl& url) {
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
    QByteArray Client::httpPost(const QUrl& url, const QJsonObject& payload) {

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
    QByteArray Client::httpPut(const QUrl& url, const QJsonObject& payload) {

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

    bool Client::login() {
        bool result = false;
        QJsonObject payload;
        payload["username"] = mUsername;
        payload["password"] = mPassword;
        QJsonObject data = signalkPost( QUrl(mUrl.toString() + "/v1/auth/login"), payload);

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

    QUrl Client::http() {
        return getEndpointByProtocol("http");
    }

    QUrl Client::ws() {
        return getEndpointByProtocol("ws");
    }

    QUrl Client::tcp() {
        return getEndpointByProtocol("tcp");
    }

    QUrl Client::getEndpointByProtocol(const QString &protocol) {
        if (mServer.contains("endpoints") & mServer["endpoints"].isObject()) {
            auto jsonObjectEndponts = mServer["endpoints"].toObject();
            if (jsonObjectEndponts.contains("v1") && jsonObjectEndponts["v1"].isObject()) {
                auto jsonObjectVersion = jsonObjectEndponts["v1"].toObject();
                if (jsonObjectVersion.contains("signalk-" + protocol) &&
                    jsonObjectVersion["signalk-" + protocol].isString()) {
                    return jsonObjectVersion["signalk-" + protocol].toString();
                }
            }
        }
        return {};
    }

//! [onConnected]
    void Client::onConnected() {
        if (mDebug)
            qDebug() << "WebSocket connected";
        connect(&mWebSocket, &QWebSocket::textMessageReceived,
                this, &Client::onTextMessageReceived);

    }
//! [onConnected]

//! [onDisconnected]
    void Client::onDisconnected() {
        if (mDebug)
            qDebug() << "WebSocket disconnected";
	QApplication::exit(1);
    }
//! [onDisconnected]

//! [onTextMessageReceived]
    void Client::onTextMessageReceived(QString message) {


        QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());

        // check validity of the document
        if (!jsonDocument.isNull()) {
            if (jsonDocument.isObject()) {
                auto updateObject = jsonDocument.object();

                if (mDebug)
                    qDebug() << "Update received:" << message;

                auto context = updateObject["context"].toString();
                auto updates = updateObject["updates"].toArray();
                for (auto updateItem: updates) {
                    auto values = updateItem.toObject()["values"].toArray();
                    for (auto valueItem: values) {
                        QString fullPath = context + "." + valueItem.toObject()["path"].toString();


                        for (auto subscription: subscriptions) {
                            subscription.match(fullPath, updateObject);
                        }
                    }
                }
            }
        }
    }
    //! [onTextMessageReceived]

    QString Client::getToken() {
        return mToken;
    }

    signalk::Waypoint Client::getWaypointByHref(const QString &href) {

        auto data = httpGet(QUrl(mUrl.toString() + "/v2/api" + href));
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto result = signalk::Waypoint(jsonDocument.object());
        return result;
    }

    QJsonObject Client::subscribe(const QString& path, QObject *receiver, const char *member, int period, const QString& policy, int minPeriod) {
        return(subscribe("vessels.self", path, receiver, member, period, policy, minPeriod));
    }
    QJsonObject Client::subscribe(const QString& context, const QString& path, QObject *receiver, const char *member, int period, const QString& policy, int minPeriod) {

        auto contextEx = context;
        if (contextEx == "vessels.self") {
            contextEx = getSelf();
        }

        auto message = QString("{\n"
                          "  \"context\": \"%1\",\n"
                          "  \"subscribe\": [\n"
                          "    {\n"
                          "      \"path\": \"%2\",\n"
                          "      \"period\": %3,\n"
                          "      \"format\": \"delta\",\n"
                          "      \"policy\": \"%4\",\n"
                          "      \"minPeriod\": %5\n"
                          "    }\n"
                          "  ]\n"
                          "}").arg(contextEx).arg(path).arg(period).arg(policy).arg(minPeriod);

        mWebSocket.sendTextMessage(message);


        Subscription subscription(contextEx, path, receiver, member);
        subscriptions.append(subscription);
        connect(receiver, &QObject::destroyed, this, &Client::unsubscribe);

        auto result = signalkGet(contextEx + "." + path);

        //qDebug() << "subscribe: " << message << " result: " << result;

        return result;
    }

    void Client::unsubscribe(QObject *receiver) {
        QMutableListIterator<Subscription> i(subscriptions);
        while (i.hasNext()) {
            auto subscription = i.next();
            if (subscription.checkReceiver(receiver)) {
                i.remove();
            }
        }
    }

    QString Client::getStringFromUpdateByPath(const QJsonObject &update, const QString& path) {

        QString result = "";

        if (update.contains("updates") and update["updates"].isArray()) {
            auto updatesJsonArray = update["updates"].toArray();
            for (auto updatesItem: updatesJsonArray) {
                if (updatesItem.isObject()) {
                    auto updateJsonObject = updatesItem.toObject();
                    if (updateJsonObject.contains("values") && updateJsonObject["values"].isArray()) {
                        auto valuesJsonArray = updateJsonObject["values"].toArray();
                        for (auto valuesItem: valuesJsonArray) {
                            if (valuesItem.isObject()) {
                                auto valueJsonObject = valuesItem.toObject();
                                if (valueJsonObject.contains("path") && valueJsonObject["path"].isString()) {
                                    auto valuePath = valueJsonObject["path"].toString();
                                    if (path.isEmpty() || path == valuePath) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isString()) {
                                            result = valueJsonObject["value"].toString();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    double Client::getDoubleFromUpdateByPath(const QJsonObject &update, const QString& path) {

        double result = std::numeric_limits<double>::quiet_NaN();
        int count = 0;
        if (update.contains("updates") and update["updates"].isArray()) {
            auto updatesJsonArray = update["updates"].toArray();
            for (auto updatesItem: updatesJsonArray) {
                if (updatesItem.isObject()) {
                    auto updateJsonObject = updatesItem.toObject();
                    if (updateJsonObject.contains("values") && updateJsonObject["values"].isArray()) {
                        auto valuesJsonArray = updateJsonObject["values"].toArray();
                        for (auto valuesItem: valuesJsonArray) {
                            if (valuesItem.isObject()) {
                                auto valueJsonObject = valuesItem.toObject();
                                if (valueJsonObject.contains("path") && valueJsonObject["path"].isString()) {
                                    auto valuePath = valueJsonObject["path"].toString();
                                    if (path.isEmpty() || path == valuePath) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isDouble()) {
                                            if (count == 0) {
                                                result = valueJsonObject["value"].toDouble();
                                            } else {
                                                result = result + valueJsonObject["value"].toDouble();
                                            }
                                            count++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (count>1) {
            result = result / count;
        }

        return result;
    }

    QJsonObject Client::getObjectFromUpdateByPath(const QJsonObject &update, const QString& path) {

        QJsonObject result;

        if (update.contains("updates") and update["updates"].isArray()) {
            auto updatesJsonArray = update["updates"].toArray();
            for (auto updatesItem: updatesJsonArray) {
                if (updatesItem.isObject()) {
                    auto updateJsonObject = updatesItem.toObject();
                    if (updateJsonObject.contains("values") && updateJsonObject["values"].isArray()) {
                        auto valuesJsonArray = updateJsonObject["values"].toArray();
                        for (auto valuesItem: valuesJsonArray) {
                            if (valuesItem.isObject()) {
                                auto valueJsonObject = valuesItem.toObject();
                                if (valueJsonObject.contains("path") && valueJsonObject["path"].isString()) {
                                    auto valuePath = valueJsonObject["path"].toString();
                                    if (path.isEmpty() || path == valuePath) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isObject()) {
                                            result = valueJsonObject["value"].toObject();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    QDateTime Client::getDateTimeFromUpdateByPath(const QJsonObject &update, const QString &path) {
        QDateTime result;

        //qDebug() << update;

        if (update.contains("updates") and update["updates"].isArray()) {
            auto updatesJsonArray = update["updates"].toArray();
            for (auto updatesItem: updatesJsonArray) {
                if (updatesItem.isObject()) {
                    auto updateJsonObject = updatesItem.toObject();
                    if (updateJsonObject.contains("values") && updateJsonObject["values"].isArray()) {
                        auto valuesJsonArray = updateJsonObject["values"].toArray();
                        for (auto valuesItem: valuesJsonArray) {
                            if (valuesItem.isObject()) {
                                auto valueJsonObject = valuesItem.toObject();
                                if (valueJsonObject.contains("path") && valueJsonObject["path"].isString()) {
                                    auto valuePath = valueJsonObject["path"].toString();
                                    if (path.isEmpty() || path == valuePath) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isObject()) {
                                            auto valueValueJsonObject = valueJsonObject["value"].toObject();
                                            if (valueValueJsonObject.contains("value") && valueValueJsonObject["value"].isObject()) {
                                                //result = valueValueJsonObject["value"].toObject();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        //qDebug() << result;
        return result;
    }

    QGeoCoordinate Client::getGeoCoordinateFromUpdateByPath(const QJsonObject &update, const QString &path) {
        QGeoCoordinate result;

        if (update.contains("updates") and update["updates"].isArray()) {
            auto updatesJsonArray = update["updates"].toArray();
            for (auto updatesItem: updatesJsonArray) {
                if (updatesItem.isObject()) {
                    auto updateJsonObject = updatesItem.toObject();
                    if (updateJsonObject.contains("values") && updateJsonObject["values"].isArray()) {
                        auto valuesJsonArray = updateJsonObject["values"].toArray();
                        for (auto valuesItem: valuesJsonArray) {
                            if (valuesItem.isObject()) {
                                auto valueJsonObject = valuesItem.toObject();
                                if (valueJsonObject.contains("path") && valueJsonObject["path"].isString()) {
                                    auto valuePath = valueJsonObject["path"].toString();
                                    if (path.isEmpty() || path == valuePath) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isObject()) {
                                            auto position = valueJsonObject["value"].toObject();
                                            if (position.contains("latitude")) {
                                                result.setLatitude(position["latitude"].toDouble());
                                            }
                                            if (position.contains("longitude")) {
                                                result.setLongitude(position["longitude"].toDouble());
                                            }
                                            if (position.contains("altitude")) {
                                                result.setAltitude(position["altitude"].toDouble());
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    /**
 * Generate a UTC ISO8601-formatted timestamp
 * and return as std::string
 */
    QString Client::currentISO8601TimeUTC() {
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(gmtime(&itt), "%FT%TZ");
        return {ss.str().c_str()};
    }

    qint64 Client::sendMessage(QJsonObject message) {
        if (!message.contains("context")) {
            message["context"] = getSelf();
        }
        if (!message.contains("requestId")) {
            message["requestId"] =  QUuid::createUuid().toString().replace("{","").replace("}","");
        }
        if (!message.contains("token")) {
            message["token"] = getToken();
        }
        const QJsonDocument doc(message);
        QString text = doc.toJson(QJsonDocument::Compact);

        return mWebSocket.sendTextMessage(text);
    }
}
