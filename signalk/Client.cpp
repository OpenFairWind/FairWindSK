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
#include <QEventLoop>
#include <QPointer>
#include <QTimer>
#include <QUrlQuery>

#include "Client.hpp"
#include "FairWindSK.hpp"
#include "Waypoint.hpp"

namespace fairwindsk::signalk {
    namespace {
        constexpr int kStreamTimeoutMs = 30000;
        constexpr int kStreamHealthCheckIntervalMs = 5000;
    }

    QNetworkRequest Client::createJsonRequest(const QUrl& url) const {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json");

        if (!m_Cookie.isEmpty()) {
            request.setRawHeader("Cookie", m_Cookie.toLatin1());
        }

        return request;
    }

    QByteArray Client::finishReply(QNetworkReply *reply, const bool updateCookie, bool *success, QString *message, int *httpStatus) const {
        const QScopedPointer<QNetworkReply> guard(reply);
        if (success) {
            *success = false;
        }
        if (message) {
            message->clear();
        }
        if (httpStatus) {
            *httpStatus = 0;
        }
        if (!guard) {
            return {};
        }

        qInfo() << "SignalK::Client::finishReply waiting for" << guard->request().url();

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        QObject::connect(guard.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeoutTimer.start(kRequestTimeoutMs);
        loop.exec();
        qInfo() << "SignalK::Client::finishReply loop exited for" << guard->request().url()
                << "finished =" << guard->isFinished();

        if (!guard->isFinished()) {
            if (m_Debug) {
                qDebug() << Q_FUNC_INFO << "Timeout waiting for reply from" << guard->request().url();
            }
            guard->abort();
            if (message) {
                *message = tr("Timeout while contacting %1").arg(guard->request().url().toString());
            }
        }

        if (guard->error() != QNetworkReply::NoError && m_Debug) {
            qDebug() << Q_FUNC_INFO << "Failure" << guard->errorString();
        }
        if (guard->error() != QNetworkReply::NoError && message && message->isEmpty()) {
            *message = guard->errorString();
        }

        if (httpStatus) {
            *httpStatus = guard->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }

        if (updateCookie) {
            const QByteArray cookie = guard->rawHeader("Set-Cookie");
            if (!cookie.isEmpty()) {
                const_cast<Client *>(this)->m_Cookie = cookie;

                if (m_Debug) {
                    qDebug() << "m_Cookie: " << m_Cookie;
                }
            }
        }

        if (updateCookie && m_Debug) {
            const QList<QByteArray> headerList = guard->rawHeaderList();
            for (const QByteArray &head : headerList) {
                qDebug() << head << ":" << guard->rawHeader(head);
            }
        }

        if (!guard->isOpen()) {
            qWarning() << "SignalK::Client::finishReply reply is not open for" << guard->request().url();
            return {};
        }

        if (success) {
            *success = guard->error() == QNetworkReply::NoError;
        }
        qInfo() << "SignalK::Client::finishReply returning data for" << guard->request().url()
                << "httpStatus =" << guard->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                << "error =" << guard->error();
        return guard->readAll();
    }

    void Client::beginRequest(const QString &method, const QUrl &url) {
        ++m_activeRequests;
        emit requestActivityChanged(m_activeRequests > 0);
        emit requestCountChanged(m_activeRequests);

        if (m_Debug) {
            qDebug() << "SignalK request" << method << url;
        }
    }

    void Client::endRequest(const bool success, const QUrl &url, const int httpStatus, const QString &message) {
        m_activeRequests = std::max(0, m_activeRequests - 1);
        emit requestActivityChanged(m_activeRequests > 0);
        emit requestCountChanged(m_activeRequests);

        const bool suppressMessage = shouldSuppressServerMessage(url, httpStatus);
        if (success) {
            setRestHealth(true, tr("REST online"));
        } else if (!suppressMessage && (httpStatus == 0 || httpStatus >= 500)) {
            setRestHealth(false, tr("REST error"));
        }

        if (suppressMessage) {
            if (m_Debug) {
                qDebug() << "Suppressing Signal K message for unsupported endpoint" << httpStatus << url;
            }
            return;
        }

        if (!message.trimmed().isEmpty()) {
            emit serverMessageChanged(message.trimmed());
        }

        if (!success && message.trimmed().isEmpty()) {
            emit serverMessageChanged(tr("Signal K request failed"));
        }
    }

    bool Client::shouldSuppressServerMessage(const QUrl &url, const int httpStatus) const {
        return httpStatus == 501 && url.path().startsWith(QStringLiteral("/signalk/v2/api/history"));
    }

    void Client::setRestHealth(const bool healthy, const QString &statusText) {
        m_restHealthy = healthy;
        emitConnectivityState(statusText);
    }

    void Client::setStreamHealth(const bool healthy, const QString &statusText) {
        m_streamHealthy = healthy;
        emitConnectivityState(statusText);
    }

    void Client::emitConnectivityState(const QString &statusText) {
        QString summary = statusText.trimmed();
        if (summary.isEmpty()) {
            if (m_restHealthy && m_streamHealthy) {
                summary = tr("REST + Stream online");
            } else if (m_restHealthy) {
                summary = tr("REST online, stream offline");
            } else if (m_streamHealthy) {
                summary = tr("Stream online, REST offline");
            } else {
                summary = tr("Signal K offline");
            }
        }

        const ConnectionHealthState state = currentConnectionHealthState();
        const bool serverHealthy = m_restHealthy && m_streamHealthy;
        if (m_connectivityStateEmitted &&
            m_lastRestHealthyEmitted == m_restHealthy &&
            m_lastStreamHealthyEmitted == m_streamHealthy &&
            m_lastServerHealthyEmitted == serverHealthy &&
            m_lastConnectionHealthStateEmitted == state &&
            m_lastStreamActivityEmitted == m_lastStreamActivity &&
            m_lastConnectivitySummaryEmitted == summary) {
            return;
        }

        m_connectivityStateEmitted = true;
        m_lastRestHealthyEmitted = m_restHealthy;
        m_lastStreamHealthyEmitted = m_streamHealthy;
        m_lastServerHealthyEmitted = serverHealthy;
        m_lastConnectionHealthStateEmitted = state;
        m_lastStreamActivityEmitted = m_lastStreamActivity;
        m_lastConnectivitySummaryEmitted = summary;

        emit serverHealthChanged(serverHealthy, summary);
        emit connectivityChanged(m_restHealthy, m_streamHealthy, summary);
        emit connectionHealthStateChanged(state, connectionHealthStateLabel(state), m_lastStreamActivity, summary);
    }

    void Client::markStreamActivity(const QString &statusText) {
        m_lastStreamActivity = QDateTime::currentDateTimeUtc();
        if (!m_streamHealthTimer.isActive()) {
            m_streamHealthTimer.start();
        }
        setStreamHealth(true, statusText);
    }

    QString Client::discoveryMessage() const {
        if (m_Server.contains("server") && m_Server["server"].isObject()) {
            const auto serverObject = m_Server["server"].toObject();
            const QString id = serverObject.value("id").toString();
            const QString version = serverObject.value("version").toString();
            if (!id.isEmpty() && !version.isEmpty()) {
                return tr("%1 %2").arg(id, version);
            }
            if (!id.isEmpty()) {
                return id;
            }
            if (!version.isEmpty()) {
                return tr("Version %1").arg(version);
            }
        }

        return tr("Signal K reachable");
    }

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

        m_streamHealthTimer.setInterval(kStreamHealthCheckIntervalMs);
        m_streamHealthTimer.setSingleShot(false);
        connect(&m_streamHealthTimer, &QTimer::timeout, this, &Client::onStreamHealthTimeout);

        m_reconnectTimer.setInterval(2000);
        m_reconnectTimer.setSingleShot(true);
        connect(&m_reconnectTimer, &QTimer::timeout, this, &Client::attemptReconnect);

        m_plannedRestartTimer.setSingleShot(true);
        connect(&m_plannedRestartTimer, &QTimer::timeout, this, &Client::finishPlannedRestartGrace);
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
        qInfo() << "SignalK::Client::init entered";
        m_connectionParams = params;
        m_reconnectTimer.stop();

        // Get the FairWindSK instance
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        Q_UNUSED(fairWindSK);

        // Reset websocket signal wiring before reconnecting.
        disconnect(&m_WebSocket, nullptr, this, nullptr);
        if (m_WebSocket.state() != QAbstractSocket::UnconnectedState) {
            m_WebSocket.abort();
            m_WebSocket.close();
        }

        // Connect the on connected event
        connect(&m_WebSocket, &QWebSocket::connected, this, &Client::onConnected, Qt::UniqueConnection);

        // Connect the on disconnected event
        connect(&m_WebSocket, &QWebSocket::disconnected, this, &Client::onDisconnected, Qt::UniqueConnection);
        connect(&m_WebSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this, [this](QAbstractSocket::SocketError error) {
            qWarning() << "SignalK::Client websocket error" << error << m_WebSocket.errorString();
        });

        // Check if the url is present in parameters
        if (params.contains("url")) {

            // Set the url
            m_Url = params["url"].toString();
        }
        qInfo() << "SignalK::Client::init url =" << m_Url;

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
        qInfo() << "SignalK::Client::init active/debug =" << m_Active << m_Debug;

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
            setRestHealth(false, tr("Connecting to Signal K"));
            setStreamHealth(false, tr("Connecting to Signal K"));
            emit serverMessageChanged(tr("Connecting to Signal K"));
            startAsyncReconnectDiscovery();
            result = true;
        }  else {
            qInfo() << "SignalK::Client::init skipped because client is inactive";
            setRestHealth(false, tr("Signal K disabled"));
            setStreamHealth(false, tr("Signal K disabled"));
            emit serverMessageChanged(tr("Signal K connection disabled"));
            if (m_Debug)
                    qDebug() << "Data connection not active!";

        }

        qInfo() << "SignalK::Client::init exiting with result" << result;
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

        // Strip JSON string quotes from a valid response like "vessels.urn:mrn:..."
        const QString self = QString::fromUtf8(result).replace(QStringLiteral("\""), QString());

        // A valid Signal K self URN always starts with "vessels."
        // Reject error bodies such as "bad auth token" returned on 401 responses
        if (!self.startsWith(QStringLiteral("vessels."))) {
            if (m_Debug)
                qDebug() << "SignalKClient::getSelf : rejected invalid self URN:" << self;
            return {};
        }

        return self;
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
        const auto url = QUrl(http().toString()+processedPath);

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

        // Invoke the http method. Plain GET is safer when no request body is needed.
        auto data = payload.isEmpty() ? httpGet(url) : httpGet(url, payload);

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
    QJsonObject Client::signalkDelete(const QString& path, const QString& payload) {

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
    QJsonObject Client::signalkDelete(const QUrl& url, const QString& payload) {

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
        const auto data = httpDelete(url, payload);

        // Return the result as JSON object
        return QJsonDocument::fromJson(data).object();
    }

    /****************************************************************************
     * HTTP methods
     ***************************************************************************/

    /*
 * httpGet
 * Executes an http get request without payload
 */
    QByteArray Client::httpGet(const QUrl& url) {
        beginRequest(QStringLiteral("GET"), url);
        auto *reply = m_NetworkAccessManager.get(createJsonRequest(url));
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(reply, false, &success, &message, &httpStatus);
        endRequest(success, url, httpStatus, message);
        return data;
    }

    /*
 * httpGet
 * Executes a http get request with payload
 */
    QByteArray Client::httpGet(const QUrl& url, const QJsonObject& payload) {
        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);
        beginRequest(QStringLiteral("GET"), url);
        auto *reply = m_NetworkAccessManager.sendCustomRequest(createJsonRequest(url), "GET", jsonDocument.toJson());
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(reply, false, &success, &message, &httpStatus);
        endRequest(success, url, httpStatus, message);
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

        if (!m_Cookie.isEmpty() && m_Debug) {
            qDebug() << "SignalKClient::httpPost cookie: " << m_Cookie.toLatin1();
        }

        beginRequest(QStringLiteral("POST"), url);
        auto *reply = m_NetworkAccessManager.post(createJsonRequest(url), jsonDocument.toJson());
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(reply, true, &success, &message, &httpStatus);
        endRequest(success, url, httpStatus, message);
        return data;
    }

    /*
 * httpPost
 * Executes a http put request
 */
    QByteArray Client::httpPut(const QUrl& url, const QJsonObject& payload) {

        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        beginRequest(QStringLiteral("PUT"), url);
        auto *reply = m_NetworkAccessManager.put(createJsonRequest(url), jsonDocument.toJson());
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(reply, false, &success, &message, &httpStatus);
        endRequest(success, url, httpStatus, message);
        return data;
    }

    /*
 * httpDelete
 * Executes a http delete request
 */
    QByteArray Client::httpDelete(const QUrl& url, const QJsonObject& payload) {
        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);

        beginRequest(QStringLiteral("DELETE"), url);
        auto *reply = m_NetworkAccessManager.sendCustomRequest(createJsonRequest(url), "DELETE", jsonDocument.toJson());
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(reply, false, &success, &message, &httpStatus);
        endRequest(success, url, httpStatus, message);
        return data;
    }

    bool Client::login() {
        bool result = false;
        qInfo() << "SignalK::Client::login entered";
        QJsonObject payload;
        payload["username"] = m_Username;
        payload["password"] = m_Password;
        QJsonObject data = signalkPost( QUrl(m_Url.toString() + "/v1/auth/login"), payload);
        qInfo() << "SignalK::Client::login reply keys =" << data.keys();

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

        qInfo() << "SignalK::Client::login exiting with result" << result;
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
        if (m_Server.contains("endpoints") && m_Server["endpoints"].isObject()) {
            auto jsonObjectEndponts = m_Server["endpoints"].toObject();
            if (jsonObjectEndponts.contains(version) && jsonObjectEndponts[version].isObject()) {
                auto jsonObjectVersion = jsonObjectEndponts[version].toObject();
                if (jsonObjectVersion.contains("signalk-" + protocol) &&
                    jsonObjectVersion["signalk-" + protocol].isString()) {
                    qInfo() << "SignalK::Client::getEndpointByProtocol" << protocol << version
                            << "->" << jsonObjectVersion["signalk-" + protocol].toString();
                    return jsonObjectVersion["signalk-" + protocol].toString();
                }
            }
        }
        qWarning() << "SignalK::Client::getEndpointByProtocol missing endpoint for" << protocol << version;
        return {};
    }

//! [onConnected]
    void Client::onConnected() {
        if (m_Debug)
            qDebug() << "WebSocket connected";

        m_reconnectTimer.stop();
        m_plannedRestartTimer.stop();
        m_plannedRestartInProgress = false;
        markStreamActivity(tr("Stream online"));
        emit serverMessageChanged(discoveryMessage());

        connect(&m_WebSocket, &QWebSocket::textMessageReceived,
                this, &Client::onTextMessageReceived, Qt::UniqueConnection);

        const bool recoveredFromDisconnect = m_reconnectRecoveryPending;
        resubscribeAll(recoveredFromDisconnect);
        m_hadStreamConnection = true;
        m_reconnectRecoveryPending = false;
        emit serverStateResynchronized(recoveredFromDisconnect);

    }
//! [onConnected]

//! [onDisconnected]
    void Client::onDisconnected() {
        if (m_Debug)
            qDebug() << "WebSocket disconnected";

        m_streamHealthTimer.stop();
        if (m_plannedRestartInProgress) {
            setStreamHealth(false, tr("Signal K restarting"));
            emit serverMessageChanged(tr("Waiting for Signal K restart"));
            return;
        }

        setStreamHealth(false, tr("Stream disconnected"));
        emit serverMessageChanged(tr("Waiting for Signal K"));
        if (m_Active && !m_Url.isEmpty()) {
            m_reconnectRecoveryPending = m_hadStreamConnection || m_reconnectRecoveryPending;
            scheduleReconnect();
        }
    }
//! [onDisconnected]

//! [onTextMessageReceived]
    void Client::onTextMessageReceived(QString message) {
        markStreamActivity(tr("Stream online"));

        QJsonDocument jsonDocument = QJsonDocument::fromJson(message.toUtf8());

        // check validity of the document
        if (!jsonDocument.isNull()) {
            if (jsonDocument.isObject()) {
                auto updateObject = jsonDocument.object();

                if (m_Debug)
                    qDebug() << "Update received:" << message;

                const auto context = updateObject["context"].toString();
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

    void Client::onStreamHealthTimeout() {
        if (m_lastStreamActivity.isNull()) {
            return;
        }

        if (m_lastStreamActivity.msecsTo(QDateTime::currentDateTimeUtc()) > kStreamTimeoutMs) {
            setStreamHealth(false, tr("Data stale"));
            emit serverMessageChanged(tr("Using cached Signal K data"));
        }
    }

    QString Client::getToken() {
        return m_Token;
    }

    QString Client::normalizedSubscriptionContext(const QString &context) const {
        if (context == "vessels.self") {
            // getSelf() returns empty when the server rejects auth or returns a non-URN body;
            // fall back to the literal "vessels.self" so WebSocket subscriptions remain valid
            const QString resolved = const_cast<Client *>(this)->getSelf();
            return resolved.isEmpty() ? context : resolved;
        }

        return context;
    }

    QString Client::subscriptionMessage(const QString &context, const QString &path, const int period, const QString &policy, const int minPeriod) const {
        return QString("{\n"
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
                       "}").arg(context, path, QString::number(period), policy, QString::number(minPeriod));
    }

    bool Client::refreshServerDiscovery() {
        qInfo() << "SignalK::Client requesting server discovery from" << m_Url;
        const QJsonObject discoveredServer = signalkGet(m_Url);
        return applyDiscoveredServer(discoveredServer);
    }

    bool Client::applyDiscoveredServer(const QJsonObject &discoveredServer) {
        qInfo() << "SignalK::Client discovery reply keys =" << discoveredServer.keys();

        if (m_Debug) {
            qDebug() << "Server:" << discoveredServer;
        }

        if (discoveredServer.isEmpty()) {
            return false;
        }

        const QString newFingerprint = serverFingerprint(discoveredServer);
        if (!m_serverFingerprintValue.isEmpty()
            && !newFingerprint.isEmpty()
            && m_serverFingerprintValue != newFingerprint) {
            m_reconnectRecoveryPending = true;
        }

        m_Server = discoveredServer;
        m_serverFingerprintValue = newFingerprint;
        setRestHealth(true, tr("REST online"));
        setStreamHealth(false, tr("Connecting stream"));
        emit serverMessageChanged(discoveryMessage());
        return true;
    }

    void Client::refreshAuthenticationState() {
        if (m_Token.isEmpty()) {
            qInfo() << "SignalK::Client token missing, attempting login";
            login();
            qInfo() << "SignalK::Client login finished; token present =" << !m_Token.isEmpty();
        }

        if (!m_Token.isEmpty()) {
            m_Cookie = "JAUTHENTICATION=" + m_Token + "; Path=/; HttpOnly";
        } else {
            m_Cookie.clear();
            emit serverMessageChanged(tr("Connected in public mode"));

            if (m_Debug) {
                qDebug() << "Proceeding without token in read-only/public mode.";
            }
        }
    }

    void Client::openWebSocket() {
        const auto webSocketUrl = QUrl(ws().toString() + "?subscribe=none");
        qInfo() << "SignalK::Client websocket URL =" << webSocketUrl;
        if (webSocketUrl.isValid() && !webSocketUrl.isEmpty()) {
            qInfo() << "SignalK::Client opening websocket";
            m_WebSocket.open(webSocketUrl);
            qInfo() << "SignalK::Client websocket open issued";
        } else if (m_Debug) {
            qDebug() << "No valid websocket endpoint available for" << m_Url;
        }
    }

    void Client::scheduleReconnect(const int delayMs) {
        if (m_plannedRestartInProgress || m_reconnectTimer.isActive() || m_reconnectAttemptInFlight) {
            return;
        }

        setStreamHealth(false, tr("Reconnecting stream"));
        emit serverMessageChanged(tr("Reconnecting to Signal K"));
        m_reconnectTimer.start(std::max(0, delayMs));
    }

    void Client::startAsyncReconnectDiscovery() {
        if (m_plannedRestartInProgress || m_reconnectAttemptInFlight) {
            return;
        }

        m_reconnectAttemptInFlight = true;
        emitConnectivityState(m_hadStreamConnection ? tr("Reconnecting to Signal K") : tr("Connecting to Signal K"));
        beginRequest(QStringLiteral("GET"), m_Url);
        auto *reply = m_NetworkAccessManager.get(createJsonRequest(m_Url));

        auto *timeoutTimer = new QTimer(reply);
        timeoutTimer->setSingleShot(true);
        connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
            if (!reply->isFinished()) {
                reply->abort();
            }
        });
        timeoutTimer->start(kRequestTimeoutMs);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
            m_reconnectAttemptInFlight = false;

            const bool success = guard->error() == QNetworkReply::NoError;
            const int httpStatus = guard->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString message = success
                                        ? QString()
                                        : (guard->errorString().trimmed().isEmpty()
                                               ? tr("Signal K server not available")
                                               : guard->errorString().trimmed());

            if (!success) {
                setRestHealth(false, tr("Signal K offline"));
                emit serverMessageChanged(tr("Signal K server not available"));
                endRequest(false, guard->request().url(), httpStatus, message);
                scheduleReconnect();
                return;
            }

            const QJsonDocument document = QJsonDocument::fromJson(guard->readAll());
            const QJsonObject discoveredServer = document.isObject() ? document.object() : QJsonObject{};
            const bool applied = applyDiscoveredServer(discoveredServer);
            endRequest(applied, guard->request().url(), httpStatus, applied ? QString() : tr("Invalid Signal K discovery payload"));
            if (!applied) {
                setRestHealth(false, tr("Signal K offline"));
                emit serverMessageChanged(tr("Signal K server not available"));
                scheduleReconnect();
                return;
            }

            refreshAuthenticationState();
            m_reconnectRecoveryPending = true;
            openWebSocket();
        });
    }

    bool Client::hasSubscription(const QString &requestedContext, const QString &path, QObject *receiver) const {
        for (const auto &subscription : m_subscriptions) {
            if (subscription.getReceiver() == receiver
                && subscription.getRequestedContext() == requestedContext
                && subscription.getPath() == path) {
                return true;
            }
        }

        return false;
    }

    void Client::resubscribeAll(const bool hydrateSnapshots) {
        QMutableListIterator<Subscription> iterator(m_subscriptions);
        while (iterator.hasNext()) {
            auto &subscription = iterator.next();
            if (!subscription.getReceiver()) {
                iterator.remove();
                continue;
            }

            const QString effectiveContext = normalizedSubscriptionContext(subscription.getRequestedContext());
            subscription.retargetContext(effectiveContext);
            m_WebSocket.sendTextMessage(subscriptionMessage(effectiveContext,
                                                           subscription.getPath(),
                                                           1000,
                                                           QStringLiteral("ideal"),
                                                           200));

            if (!hydrateSnapshots || subscription.getPath().contains('*')) {
                continue;
            }

            const QJsonObject snapshot = signalkGet(effectiveContext + "." + subscription.getPath());
            if (!snapshot.isEmpty()) {
                const QJsonObject update = buildDeltaUpdate(effectiveContext, subscription.getPath(), snapshot);
                subscription.match(effectiveContext + "." + subscription.getPath(), update);
            }
        }
    }

    QString Client::serverFingerprint(const QJsonObject &server) const {
        if (server.isEmpty()) {
            return {};
        }

        return QString::fromUtf8(QJsonDocument(server).toJson(QJsonDocument::Compact));
    }

    QJsonObject Client::buildDeltaUpdate(const QString &context, const QString &path, const QJsonValue &value) const {
        QJsonObject valueObject;
        valueObject.insert(QStringLiteral("path"), path);
        valueObject.insert(QStringLiteral("value"), value);

        QJsonObject updateEntry;
        updateEntry.insert(QStringLiteral("values"), QJsonArray{valueObject});

        QJsonObject update;
        update.insert(QStringLiteral("context"), context);
        update.insert(QStringLiteral("updates"), QJsonArray{updateEntry});
        return update;
    }

    void Client::attemptReconnect() {
        if (!m_Active || m_Url.isEmpty() || m_plannedRestartInProgress) {
            return;
        }

        if (m_WebSocket.state() == QAbstractSocket::ConnectedState
            || m_WebSocket.state() == QAbstractSocket::ConnectingState) {
            return;
        }

        qInfo() << "SignalK::Client::attemptReconnect entered";
        startAsyncReconnectDiscovery();
    }

    void Client::beginPlannedServerRestart(const int gracePeriodMs) {
        if (!m_Active || m_Url.isEmpty()) {
            return;
        }

        m_plannedRestartInProgress = true;
        m_reconnectRecoveryPending = true;
        m_reconnectTimer.stop();
        m_streamHealthTimer.stop();

        if (m_reconnectAttemptInFlight) {
            m_reconnectAttemptInFlight = false;
        }

        setRestHealth(false, tr("Signal K restarting"));
        setStreamHealth(false, tr("Signal K restarting"));
        emit serverMessageChanged(tr("Signal K restart requested"));

        if (m_WebSocket.state() != QAbstractSocket::UnconnectedState) {
            m_WebSocket.abort();
            m_WebSocket.close();
        }

        m_plannedRestartTimer.start(std::max(5000, gracePeriodMs));
    }

    void Client::finishPlannedRestartGrace() {
        if (!m_plannedRestartInProgress) {
            return;
        }

        m_plannedRestartInProgress = false;
        if (m_Active && !m_Url.isEmpty()) {
            scheduleReconnect(0);
        }
    }

    signalk::Waypoint Client::getWaypointByHref(const QString &href) {

        auto data = httpGet(QUrl(m_Url.toString() + "/v2/api" + href));
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto result = signalk::Waypoint(jsonDocument.object());
        return result;
    }

    QJsonDocument Client::getJsonDocument(const QUrl &url, const QJsonObject &payload) {
        const auto data = payload.isEmpty() ? httpGet(url) : httpGet(url, payload);
        return QJsonDocument::fromJson(data);
    }

    QUrl Client::withQuery(const QUrl &url, const QVariantMap &query) const {
        if (query.isEmpty()) {
            return url;
        }

        QUrl result(url);
        QUrlQuery urlQuery(result);
        for (auto it = query.constBegin(); it != query.constEnd(); ++it) {
            if (!it.value().isValid() || it.value().toString().trimmed().isEmpty()) {
                continue;
            }
            urlQuery.addQueryItem(it.key(), it.value().toString());
        }
        result.setQuery(urlQuery);
        return result;
    }

    QUrl Client::resourceUrl(const QString &collection, const QString &id, const QVariantMap &query) const {
        const QString basePath = m_Url.toString() + "/v2/api/resources/" + collection;
        QUrl url;
        if (id.isEmpty()) {
            url = QUrl(basePath);
        } else {
            url = QUrl(basePath + "/" + QString::fromLatin1(QUrl::toPercentEncoding(id)));
        }

        return withQuery(url, query);
    }

    QUrl Client::historyUrl(const QString &suffix, const QVariantMap &query) const {
        const QString path = suffix.isEmpty() ? "/v2/api/history" : "/v2/api/history/" + suffix;
        return withQuery(QUrl(m_Url.toString() + path), query);
    }

    QMap<QString, Waypoint> Client::getWaypoints() {
        QMap<QString, Waypoint> result;

        const auto data = httpGet(QUrl(m_Url.toString() + "/v2/api/resources/waypoints"));
        const auto jsonDocument = QJsonDocument::fromJson(data);
        const auto jsonObject = jsonDocument.object();


        for ( const auto& key : jsonObject.keys() ) {
            if (jsonObject.contains(key) && jsonObject[key].isObject()) {
                auto value = jsonObject[key].toObject();
                auto waypoint = signalk::Waypoint(value);
                result.insert(key, waypoint);
            }

        }

        return result;
    }

    QMap<QString, QJsonObject> Client::getResources(const QString &collection, const QVariantMap &query) {
        QMap<QString, QJsonObject> result;

        const auto jsonDocument = getJsonDocument(resourceUrl(collection, QString(), query));
        const auto jsonObject = jsonDocument.object();

        for (const auto &key : jsonObject.keys()) {
            if (jsonObject.contains(key) && jsonObject[key].isObject()) {
                result.insert(key, jsonObject[key].toObject());
            }
        }

        return result;
    }

    QJsonObject Client::getResource(const QString &collection, const QString &id, const QVariantMap &query) {
        return getJsonDocument(resourceUrl(collection, id, query)).object();
    }

    QJsonObject Client::createResource(const QString &collection, const QJsonObject &payload, const QVariantMap &query) {
        QJsonDocument jsonDocument;
        jsonDocument.setObject(payload);
        const auto response = finishReply(m_NetworkAccessManager.post(createJsonRequest(resourceUrl(collection, QString(), query)),
                                                                      jsonDocument.toJson()));
        return QJsonDocument::fromJson(response).object();
    }

    QJsonObject Client::putResource(const QString &collection, const QString &id, const QJsonObject &payload) {
        QJsonObject mutablePayload = payload;
        return signalkPut(resourceUrl(collection, id), mutablePayload);
    }

    bool Client::deleteResource(const QString &collection, const QString &id) {
        QJsonObject payload;
        signalkDelete(resourceUrl(collection, id), payload);
        return true;
    }

    bool Client::navigateToWaypoint(const QString &href) {
        if (href.trimmed().isEmpty()) {
            return false;
        }

        QJsonObject payload;
        payload["href"] = href;
        const auto response = signalkPut(QUrl(m_Url.toString() + "/v2/api/vessels/self/navigation/course/destination"), payload);
        return !response.isEmpty() || !m_Active;
    }

    QJsonArray Client::getHistoryPaths(const QVariantMap &query) {
        const auto jsonDocument = getJsonDocument(historyUrl("paths", query));
        return jsonDocument.isArray() ? jsonDocument.array() : QJsonArray{};
    }

    QJsonObject Client::getHistoryValues(const QStringList &paths, const QVariantMap &query) {
        QVariantMap effectiveQuery = query;
        if (!paths.isEmpty()) {
            effectiveQuery["paths"] = paths.join(",");
        }

        return getJsonDocument(historyUrl("values", effectiveQuery)).object();
    }

    QJsonObject Client::getUnitPreferencesActive() {
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/active"));
    }

    QJsonObject Client::getUnitPreferencesConfig() {
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/config"));
    }

    QJsonDocument Client::getUnitPreferencesPresets() {
        return getJsonDocument(QUrl(url().toString() + "/v1/unitpreferences/presets"));
    }

    QJsonObject Client::getUnitPreferencesPreset(const QString &name) {
        const QString encodedName = QString::fromUtf8(QUrl::toPercentEncoding(name));
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/presets/" + encodedName));
    }

    QJsonObject Client::getUnitPreferencesCategories() {
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/categories"));
    }

    QJsonObject Client::getUnitPreferencesDefinitions() {
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/definitions"));
    }

    QJsonObject Client::getUnitPreferencesDefaultCategories() {
        return signalkGet(QUrl(url().toString() + "/v1/unitpreferences/default-categories"));
    }

    QJsonObject Client::putUnitPreferencesCustomPreset(const QString &name, const QJsonObject &payload) {
        const QString encodedName = QString::fromUtf8(QUrl::toPercentEncoding(name));
        QJsonObject mutablePayload = payload;
        return signalkPut(QUrl(url().toString() + "/v1/unitpreferences/presets/custom/" + encodedName), mutablePayload);
    }

    bool Client::deleteUnitPreferencesCustomPreset(const QString &name) {
        const QString encodedName = QString::fromUtf8(QUrl::toPercentEncoding(name));
        QJsonObject payload;
        signalkDelete(QUrl(url().toString() + "/v1/unitpreferences/presets/custom/" + encodedName), payload);
        return true;
    }

    QJsonObject Client::getPathMeta(const QString &path, const QString &context) {
        QString normalizedContext = context;
        if (normalizedContext.endsWith('/')) {
            normalizedContext.chop(1);
        }

        QString pathComponent = path;
        pathComponent.replace('.', '/');

        return signalkGet(QUrl(url().toString() + "/v1/api/" + normalizedContext + "/" + pathComponent + "/meta"));
    }

    bool Client::isRestHealthy() const {
        return m_restHealthy;
    }

    bool Client::isStreamHealthy() const {
        return m_streamHealthy;
    }

    QString Client::connectionStatusText() const {
        if (m_restHealthy && m_streamHealthy) {
            return tr("REST + Stream online");
        }
        if (m_restHealthy) {
            return tr("REST online, stream offline");
        }
        if (m_streamHealthy) {
            return tr("Stream online, REST offline");
        }
        return tr("Signal K offline");
    }

    Client::ConnectionHealthState Client::currentConnectionHealthState() const {
        if (!m_Active || m_Url.isEmpty()) {
            return ConnectionHealthState::Disconnected;
        }

        if (m_plannedRestartInProgress || m_reconnectTimer.isActive() || m_reconnectAttemptInFlight) {
            return m_hadStreamConnection ? ConnectionHealthState::Reconnecting : ConnectionHealthState::Connecting;
        }

        if (m_restHealthy && m_streamHealthy) {
            return ConnectionHealthState::Live;
        }

        if (m_restHealthy && !m_streamHealthy) {
            return m_hadStreamConnection ? ConnectionHealthState::Stale : ConnectionHealthState::Connecting;
        }

        if (!m_restHealthy && m_streamHealthy) {
            return ConnectionHealthState::Degraded;
        }

        return m_hadStreamConnection ? ConnectionHealthState::Reconnecting : ConnectionHealthState::Disconnected;
    }

    Client::ConnectionHealthState Client::connectionHealthState() const {
        return currentConnectionHealthState();
    }

    QString Client::connectionHealthStateLabel(const ConnectionHealthState state) const {
        switch (state) {
        case ConnectionHealthState::Disconnected:
            return tr("Disconnected");
        case ConnectionHealthState::Connecting:
            return tr("Connecting");
        case ConnectionHealthState::Live:
            return tr("Live");
        case ConnectionHealthState::Stale:
            return tr("Stale");
        case ConnectionHealthState::Reconnecting:
            return tr("Reconnecting");
        case ConnectionHealthState::Degraded:
            return tr("Degraded");
        }

        return tr("Disconnected");
    }

    QString Client::connectionHealthStateText() const {
        return connectionHealthStateLabel(currentConnectionHealthState());
    }

    QDateTime Client::lastStreamUpdate() const {
        return m_lastStreamActivity;
    }

    QJsonObject Client::subscribe(const QString& path, QObject *receiver, const char *member, int period, const QString& policy, int minPeriod) {
        return(subscribe("vessels.self", path, receiver, member, period, policy, minPeriod));
    }
    QJsonObject Client::subscribe(const QString& context, const QString& path, QObject *receiver, const char *member, int period, const QString& policy, int minPeriod) {
        const auto contextEx = normalizedSubscriptionContext(context);
        const auto message = subscriptionMessage(contextEx, path, period, policy, minPeriod);

        m_WebSocket.sendTextMessage(message);

        if (!hasSubscription(context, path, receiver)) {
            Subscription subscription(context, contextEx, path, receiver, member);
            m_subscriptions.append(subscription);
            connect(receiver, &QObject::destroyed, this, &Client::unsubscribe, Qt::UniqueConnection);
        }

        auto result = signalkGet(contextEx + "." + path);

        if (m_Debug) {
            qDebug() << "subscribe: " << message << " result: " << result;
        }
        return result;
    }

    void Client::subscribeStream(const QString& context, const QString& path, QObject *receiver, const char *member, int period, const QString& policy, int minPeriod) {
        const auto contextEx = normalizedSubscriptionContext(context);
        const auto message = subscriptionMessage(contextEx, path, period, policy, minPeriod);

        m_WebSocket.sendTextMessage(message);

        if (!hasSubscription(context, path, receiver)) {
            Subscription subscription(context, contextEx, path, receiver, member);
            m_subscriptions.append(subscription);
            connect(receiver, &QObject::destroyed, this, &Client::unsubscribe, Qt::UniqueConnection);
        }

        if (m_Debug) {
            qDebug() << "subscribeStream:" << message;
        }
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
                                    if (path.isEmpty() || valuePath.startsWith(path)) {
                                        if (valueJsonObject.contains("value") && valueJsonObject["value"].isObject()) {
                                            result = valueJsonObject["value"].toObject();
                                            result["subPath"] = valuePath.replace(path+".","");
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
                                            result = QDateTime::fromString(valueJsonObject["value"].toString(), Qt::ISODateWithMs);
                                            if (!result.isValid()) {
                                                result = QDateTime::fromString(valueJsonObject["value"].toString(), Qt::ISODate);
                                            }
                                        } else if (valueJsonObject.contains("value") && valueJsonObject["value"].isObject()) {
                                            auto valueValueJsonObject = valueJsonObject["value"].toObject();
                                            if (valueValueJsonObject.contains("value") && valueValueJsonObject["value"].isString()) {
                                                result = QDateTime::fromString(valueValueJsonObject["value"].toString(), Qt::ISODateWithMs);
                                                if (!result.isValid()) {
                                                    result = QDateTime::fromString(valueValueJsonObject["value"].toString(), Qt::ISODate);
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
        }
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

    QUrl Client::url() const {
        return m_Url;
    }

    QUrl Client::server() const
    {
        return QUrl(m_Url.scheme() + "://" + m_Url.host() + ":" + QString::number(m_Url.port()));
    }
}
