//
// Created by Raffaele Montella on 03/06/21.
//

#include <limits>
#include <iomanip>
#include <sstream>

#include <QApplication>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>

#include "Client.hpp"
#include "FairWindSK.hpp"
#include "Waypoint.hpp"

namespace fairwindsk::signalk {
/*
 * SignalKAPIClient - Public Constructor
 */
    Client::Client(QObject *parent) :
            QObject(parent) {

        m_Url = "";

        m_Active = false;
        m_Debug = false;

        m_Label = "Signal K Connection";
        m_Username = "";
        m_Password = "";
        m_Token = "";
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
        connect(&m_WebSocket, &QWebSocket::connected, this, &Client::onConnected);

        // Connect the on disconnected event
        connect(&m_WebSocket, &QWebSocket::disconnected, this, &Client::onDisconnected);

        // Check if the url is present in parameters
        if (params.contains("url")) {

            // Set the url
            m_Url = params["url"].toString();
        }

        // Check if the label is present in parameters
        if (params.contains("label")) {

            // Set the label
            m_Label = params["label"].toString();
        }

        // Check if the active is present in parameters
        if (params.contains("active")) {

            // Set the active
            m_Active = params["active"].toBool();
        }

        // Check if the debug is present in parameters
        if (params.contains("debug")) {

            // Set the debug
            m_Debug = params["debug"].toBool();
        }

        // Check if the token is present in parameters
        if (params.contains("token")) {

            // Set the token
            m_Token = params["token"].toString();
        }

        // Check if the username is present in parameters
        if (params.contains("username")) {

            // Set the username
            m_Username = params["username"].toString();
        }

        // Check if the password is present in parameters
        if (params.contains("password")) {

            // Set the password
            m_Password = params["password"].toString();
        }

        if (m_Debug)
            qDebug() << "SignalKAPIClient::onInit(" << params << ")";

        if (m_Active) {

            m_Server = signalkGet(m_Url);

            if (m_Debug) {
                qDebug() << "Server: " << m_Server;
            }

            if (!m_Server.isEmpty()) {

                // Check if the token is empty
                if (m_Token.isEmpty()) {

                    // Perform the login
                    login();
                }

                // Check if the token is not empty
                if (!m_Token.isEmpty()) {

                    m_Cookie = "JAUTHENTICATION=" + m_Token + "; Path=/; HttpOnly";

                    m_WebSocket.open(QUrl(ws().toString() + "?subscribe=none"));

                    result = true;
                } else {
                    if (m_Debug)
                        qDebug() << "Login failed.";
                }
            } else {
                if (m_Debug)
                    qDebug() << "Server: " << m_Url << " not available!";
            }
        }  else {
            if (m_Debug)
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

        if (m_Debug)
            qDebug() << "SignalKClient::getSelf :" << result;

        return result.replace("\"","");
    }

    QJsonObject Client::getAll() {
        if (m_Debug) {
            qDebug() << http();
        }

        return signalkGet(http());
    }

    /****************************************************************************
     * GET methods
     ***************************************************************************/

    /*
     * signalkGet
     * Give path as string
     */
    QJsonObject Client::signalkGet(const QString& path) {

        // Create a dummy payload
        QJsonObject jsonObject;

        // Invoke the Signal K method
        return signalkGet(path, jsonObject);
    }

    /*
     * signalkGet
     * Give path as JSON object
     */
    QJsonObject Client::signalkGet(const QUrl& url) {

        // Invoke a dummy payload
        QJsonObject jsonObject;

        // Invoke the Signal K method
        return signalkGet(url, jsonObject);
    }

    /*
     * signalkGet
     * Give path as string, payload as string
     */
    QJsonObject Client::signalkGet(const QString& path,  QString& payload) {

        // Crete the payload from the string
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the Signal K method
        return signalkGet(path, jsonObject);
    }

    /*
     * signalkGet
     * Give path as string, payload as JSON object
     */
    QJsonObject Client::signalkGet(const QString& path,  QJsonObject& payload) {

        // Assign the path to a mutable string
        QString processedPath = path;

        // Process the path
        processedPath = processedPath.replace(".","/");

        // Create the url
        auto url = QUrl(http().toString()+processedPath);

        // Check if the debug is active
        if (m_Debug) {

            // Print a message
            qDebug() << "signalkGet: " << path << " --> " << url;
        }

        // Invoke the Signal K method
        return signalkGet(url, payload);
    }

    /*
     * signalkGet
     * Give path as URL, payload as string
     */
    QJsonObject Client::signalkGet(const QUrl& url,  QString& payload) {
        // Create a payload form a string
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the Signal K method
        return signalkGet(url, jsonObject);
    }

    /*
     * signalkGet
     * Give path as URL, payload as JSON object
     */
    QJsonObject Client::signalkGet(const QUrl& url,  QJsonObject& payload) {

        // Check if the token is available
        if (!m_Token.isEmpty()) {

            // Add the token to the payload
            payload["token"] = m_Token;
        }

        // Check if the debug is active
        if (m_Debug) {

            // Show a message
            qDebug() << "SignalKClient::signalkGet payload: " << payload;
        }

        // Invoke the http method
        auto data = httpGet(url, payload);

        // Get the results as JSON object
        return QJsonDocument::fromJson(data).object();
    }


    /****************************************************************************
     * POST methods
     ***************************************************************************/

    /*
     * signalkPost
     * Give path as string
     */
    QJsonObject Client::signalkPost(const QString& path) {

        // Create a dummy payload
        QJsonObject jsonObject;

        // Invoke the Signal K method
        return signalkPost(path, jsonObject);
    }

    /*
     * signalkPost
     * Give path as JSON object
     */
    QJsonObject Client::signalkPost(const QUrl& url) {

        // Invoke a dummy payload
        QJsonObject jsonObject;

        // Invoke the Signal K method
        return signalkPost(url, jsonObject);
    }

    /*
     * signalkPost
     * Give path as string, payload as string
     */
    QJsonObject Client::signalkPost(const QString& path,  QString& payload) {

        // Crete the payload from the string
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the Signal K method
        return signalkPost(path, jsonObject);
    }

    /*
     * signalkPost
     * Give path as string, payload as JSON object
     */
    QJsonObject Client::signalkPost(const QString& path,  QJsonObject& payload) {

        // Assign the path to a mutable string
        QString processedPath = path;

        // Process the path
        processedPath = processedPath.replace(".","/");

        // Create the url
        auto url = QUrl(http().toString()+processedPath);

        // Check if the debug is active
        if (m_Debug) {

            // Print a message
            qDebug() << "signalkPost: " << path << " --> " << url;
        }

        // Invoke the Signal K method
        return signalkPost(url,payload);
    }

    /*
     * signalkPost
     * Give path as URL, payload as string
     */
    QJsonObject Client::signalkPost(const QUrl& url,  QString& payload) {
        // Create a payload form a string
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the Signal K method
        return signalkPost(url, jsonObject);
    }

    /*
     * signalkPost
     * Give path as URL, payload as JSON object
     */
    QJsonObject Client::signalkPost(const QUrl& url,  QJsonObject& payload) {

        // Check if the token is available
        if (!m_Token.isEmpty()) {

            // Add the token to the payload
            payload["token"] = m_Token;
        }

        // Check if the debug is active
       if (m_Debug) {

            // Show a message
            qDebug() << "SignalKClient::signalkPost payload: " << payload;
        }

        // Invoke the http method
        auto data = httpPost(url, payload);

        // Get the results as JSON object
        return QJsonDocument::fromJson(data).object();
    }

    /****************************************************************************
     * PUT methods
     ***************************************************************************/

    /*
     * signalkPut
     * Give path as string
     */
    QJsonObject Client::signalkPut(const QString& path) {
        QJsonObject jsonObject;
        return signalkPut(path, jsonObject);
    }

    /*
     * signalkPut
     * Give path as URL, payload as JSON object
     */
    QJsonObject Client::signalkPut(const QUrl& url) {
        QJsonObject jsonObject;
        return signalkPut(url, jsonObject);
    }

    /*
     * signalkPut
     * Give path as string, payload as string
     */
    QJsonObject Client::signalkPut(const QString& path,  QString& payload) {
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();
        return signalkPut(path, jsonObject);
    }

    /*
     * signalkPut
     * Give path as string, payload as JSON objcte
     */
    QJsonObject Client::signalkPut(const QString& path,  QJsonObject& payload) {
        QString processedPath = path;
        processedPath = processedPath.replace(".","/");
        auto url = QUrl(http().toString()+processedPath);
        if (m_Debug)
        {
            qDebug() << "signalkPut " << path << " --> " << url;
        }
        return signalkPut(url, payload);
    }

    /*
     * signalkPut
     * Give path as URL, payload as string
     */
    QJsonObject Client::signalkPut(const QUrl& url,  QString& payload) {
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();
        return signalkPut(url, jsonObject);
    }

    /*
     * signalkPut
     * Give path as URL, payload as JSON object
     */
    QJsonObject Client::signalkPut(const QUrl& url,  QJsonObject& payload) {
        if (!m_Token.isEmpty()) {
            payload["token"] = m_Token;
        }

       if (m_Debug) {
           qDebug() << "SignalKClient::signalkPut payload: " << m_Token << " " << payload;
       }
        auto data = httpPut(url,payload);
        return QJsonDocument::fromJson(data).object();
    }

    /****************************************************************************
     * DELETE methods
     ***************************************************************************/

    /*
     * signalkDelete
     * Given Path
     */
    QJsonObject Client::signalkDelete(const QString& path) {

        // Define a dummy payload
        QJsonObject jsonObject;

        // Invoke the delete method
        return signalkDelete(path, jsonObject);
    }

    /*
     * signalkDelete
     * Given URL
     */
    QJsonObject Client::signalkDelete(const QUrl& url) {

        // Define a dummy payload
        QJsonObject jsonObject;

        // Invoke the delete method
        return signalkDelete(url, jsonObject);
    }

    /*
     * signalkDelete
     * Given Path and payload as string
     */
    QJsonObject Client::signalkDelete(const QString& path,  QString& payload) {

        // Create  payload from a string
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the delete method
        return signalkDelete(path, jsonObject);
    }

    /*
     * signalkDelete
     * Given Path and payload as JSON object
     */
    QJsonObject Client::signalkDelete(const QString& path,  QJsonObject& payload) {

        // Define the processed path string
        QString processedPath = path;

        // Create the path (it works only with v1 APIs)
        processedPath = processedPath.replace(".","/");

        // Create the URL object
        const auto url = QUrl(http().toString()+processedPath);

        // Check if debug is active
        if (m_Debug) {

            // Write a message
            qDebug() << "signalkDelete " << path << " --> " << url;
        }

        // Invoke the delete method
        return signalkDelete(url,payload);
    }

    /*
     * signalkDelete
     * Given URL and payload as string
     */
    QJsonObject Client::signalkDelete(const QUrl& url,  QString& payload) {

        // Create a payload json object
        QJsonObject jsonObject = QJsonDocument::fromJson(payload.toUtf8()).object();

        // Invoke the delete method
        return signalkDelete(url, jsonObject);
    }

    /*
     * signalkDelete
     * Given URL and payload as JSON object
     */
    QJsonObject Client::signalkDelete(const QUrl& url,  QJsonObject& payload) {

        // Check if the token is available
        if (!m_Token.isEmpty()) {

            // Add the token to the payload
            payload["token"] = m_Token;
        }

        // Check if debug is active
        if (m_Debug) {

            // Show a message
            qDebug() << "SignalKClient::signalkDelete payload: " << payload;
        }

        // Invoke the http method
        auto data = httpDelete(url, payload);

        // Return the result as JSON object
        return QJsonDocument::fromJson(data).object();
    }

    /****************************************************************************
     * HTTP methods
     ***************************************************************************/

    /*
 * httpGet
 * Executes a http get request without payload
 */
    QByteArray Client::httpGet(const QUrl& url) {
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            req.setRawHeader("Cookie", m_Cookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(m_NetworkAccessManager.get(req));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {

            if (m_Debug)
                qDebug() << "Failure" << reply->errorString();
        }
        QByteArray data = reply->readAll();
        return data;
    }

    /*
 * httpGet
 * Executes a http get request with payload
 */
    QByteArray Client::httpGet(const QUrl& url, const QJsonObject& payload) {
        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            req.setRawHeader("Cookie", m_Cookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(m_NetworkAccessManager.sendCustomRequest(req, "GET", jsonDocument.toJson()));

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

    /*
 * httpPost
 * Executes a http post request
 */
    QByteArray Client::httpPost(const QUrl& url, const QJsonObject& payload) {

        if (m_Debug)
            qDebug() << "SignalKClient::httpPost url: " << url << " payload: " << payload;

        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            req.setRawHeader("Cookie", m_Cookie.toLatin1());

            if (m_Debug)
                qDebug() << "SignalKClient::httpPost cookie: " << m_Cookie.toLatin1();
        }

        QScopedPointer<QNetworkReply> reply(m_NetworkAccessManager.post(req, jsonDocument.toJson()));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (m_Debug)
                qDebug() << "Failure" << reply->errorString();
        }
        QByteArray data = reply->readAll();

        auto cookie = reply->rawHeader("Set-Cookie");
        if (cookie!= nullptr && cookie.size()>0) {
            m_Cookie = cookie;

            if (m_Debug)
                qDebug() << "m_Cookie: " <<m_Cookie;
        }

        if (m_Debug) {
            QList<QByteArray> headerList = reply->rawHeaderList();
                    foreach(QByteArray head, headerList) {
                    qDebug() << head << ":" << reply->rawHeader(head);
                }
        }

        return data;
    }

    /*
 * httpPost
 * Executes a http put request
 */
    QByteArray Client::httpPut(const QUrl& url, const QJsonObject& payload) {

        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            req.setRawHeader("Cookie", m_Cookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(m_NetworkAccessManager.put(req, jsonDocument.toJson()));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (m_Debug) {
                qDebug() << "Failure" << reply->errorString();
            }
        }
        QByteArray data = reply->readAll();
        return data;
    }

    /*
 * httpDelete
 * Executes a http delete request
 */
    QByteArray Client::httpDelete(const QUrl& url, const QJsonObject& payload) {
        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            req.setRawHeader("Cookie", m_Cookie.toLatin1());
        }

        QScopedPointer<QNetworkReply> reply(m_NetworkAccessManager.sendCustomRequest(req, "DELETE", jsonDocument.toJson()));

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (m_Debug)
            {
                qDebug() << "Failure" << reply->errorString();
            }
        }
        QByteArray data = reply->readAll();
        return data;
    }

    bool Client::login() {
        bool result = false;
        QJsonObject payload;
        payload["username"] = m_Username;
        payload["password"] = m_Password;
        QJsonObject data = signalkPost( QUrl(m_Url.toString() + "/v1/auth/login"), payload);

        if (m_Debug)
            qDebug() << "SignalKClient::login : " << data;

        if (data.contains("token") && data["token"].isString()) {
            m_Token = data["token"].toString();

            if (m_Debug)
                qDebug() << "SignalKClient::login token: " << m_Token;

            result = true;
        } else if (data.contains("message") && data["message"].isString() ){

            if (m_Debug)
                qDebug() << data["message"].toString();
        }

        return result;
    }

    QUrl Client::http(const QString& version ) {
        return getEndpointByProtocol("http", version);
    }

    QUrl Client::ws(const QString& version) {
        return getEndpointByProtocol("ws", version);
    }

    QUrl Client::tcp(const QString& version) {
        return getEndpointByProtocol("tcp", version);
    }

    QUrl Client::getEndpointByProtocol(const QString &protocol, const QString& version) {
        if (m_Server.contains("endpoints") & m_Server["endpoints"].isObject()) {
            auto jsonObjectEndponts = m_Server["endpoints"].toObject();
            if (jsonObjectEndponts.contains(version) && jsonObjectEndponts[version].isObject()) {
                auto jsonObjectVersion = jsonObjectEndponts[version].toObject();
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
        if (m_Debug)
            qDebug() << "WebSocket connected";
        connect(&m_WebSocket, &QWebSocket::textMessageReceived,
                this, &Client::onTextMessageReceived);

    }
//! [onConnected]

//! [onDisconnected]
    void Client::onDisconnected() {
        if (m_Debug)
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

                if (m_Debug)
                    qDebug() << "Update received:" << message;

                auto context = updateObject["context"].toString();
                auto updates = updateObject["updates"].toArray();
                for (auto updateItem: updates) {
                    auto values = updateItem.toObject()["values"].toArray();
                    for (auto valueItem: values) {
                        QString fullPath = context + "." + valueItem.toObject()["path"].toString();


                        for (auto subscription: m_subscriptions) {
                            subscription.match(fullPath, updateObject);
                        }
                    }
                }
            }
        }
    }
    //! [onTextMessageReceived]

    QString Client::getToken() {
        return m_Token;
    }

    signalk::Waypoint Client::getWaypointByHref(const QString &href) {

        auto data = httpGet(QUrl(m_Url.toString() + "/v2/api" + href));
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto result = signalk::Waypoint(jsonDocument.object());
        return result;
    }

    QMap<QString, Waypoint> Client::getWaypoints() {
        QMap<QString, Waypoint> result;

        const auto data = httpGet(QUrl(m_Url.toString() + "/v2/api/resources/waypoints"));
        const auto jsonDocument = QJsonDocument::fromJson(data);
        const auto jsonObject = jsonDocument.object();


        for ( const auto& key : jsonObject.keys() ) {
            if (jsonObject.contains(key) && jsonObject[key].isObject()) {
                auto value = jsonObject[key].toObject();

                qDebug() << "Waypoint " << key;
                qDebug() << value;

                auto waypoint = signalk::Waypoint(value);
                result.insert(key, waypoint);
            }

        }

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

        m_WebSocket.sendTextMessage(message);


        Subscription subscription(contextEx, path, receiver, member);
        m_subscriptions.append(subscription);
        connect(receiver, &QObject::destroyed, this, &Client::unsubscribe);

        auto result = signalkGet(contextEx + "." + path);

        if (m_Debug) {
            qDebug() << "subscribe: " << message << " result: " << result;
        }
        return result;
    }

    void Client::removeSubscription(const QString& path, QObject *receiver) {
        QMutableListIterator<Subscription> i(m_subscriptions);
        while (i.hasNext()) {
            auto subscription = i.next();
            if (subscription.checkReceiver(receiver) && path == subscription.getPath()) {
                i.remove();
            }
        }
    }

    void Client::unsubscribe(QObject *receiver) {
        QMutableListIterator<Subscription> i(m_subscriptions);
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

        return m_WebSocket.sendTextMessage(text);
    }

    QUrl Client::url() {
        return m_Url;
    }

    QUrl Client::server() const
    {
        return QUrl(m_Url.scheme() + "://" + m_Url.host() + ":" + QString::number(m_Url.port()));
    }
}
