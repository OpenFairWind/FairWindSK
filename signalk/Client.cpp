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
#include "ProtocolUtils.hpp"
#include "FairWindSK.hpp"
#include "Waypoint.hpp"
#include <QTimer>

namespace fairwindsk::signalk {
    namespace {
        constexpr int kStreamTimeoutMs = 30000;
        constexpr int kStreamHealthCheckIntervalMs = 5000;

        bool canHydrateSubscriptionPath(const QString &path) {
            return !path.trimmed().isEmpty() && !path.contains('*');
        }

        QJsonValue deltaValueFromSnapshot(const QJsonValue &snapshot) {
            if (!snapshot.isObject()) {
                return snapshot;
            }

            const auto snapshotObject = snapshot.toObject();
            if (snapshotObject.contains(QStringLiteral("value"))) {
                return snapshotObject.value(QStringLiteral("value"));
            }

            return snapshot;
        }

        QString normalizedSubscriptionPolicy(QString policy) {
            policy = policy.trimmed().toLower();
            if (policy == QStringLiteral("fixed")) {
                return QStringLiteral("fixed");
            }
            return QStringLiteral("instant");
        }
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

        if (success) {
            *success = (guard->error() == QNetworkReply::NoError);
        }
        if (httpStatus) {
            *httpStatus = guard->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        if (updateCookie) {
            const QVariant cookieHeader = guard->header(QNetworkRequest::SetCookieHeader);
            if (cookieHeader.isValid()) {
                const QList<QNetworkCookie> cookies = cookieHeader.value<QList<QNetworkCookie>>();
                if (!cookies.isEmpty()) {
                    const QString cookieStr = QString::fromLatin1(cookies.first().toRawForm(QNetworkCookie::NameAndValueOnly));
                    const_cast<Client *>(this)->m_Cookie = cookieStr;
                    qInfo() << "SignalK::Client::finishReply new cookie:" << cookieStr;
                }
            }
        }
        return guard->readAll();
    }

    void Client::beginRequest(const QString &verb, const QUrl &url) {
        if (m_Debug) {
            qDebug() << "Client::beginRequest" << verb << url;
        }
        m_pendingRequestCount++;
        emit requestStateChanged(m_pendingRequestCount > 0, verb, url.toString());
    }

    void Client::endRequest(const bool success, const QUrl &url, int httpStatus, const QString &errorMessage) {
        if (m_pendingRequestCount > 0) {
            m_pendingRequestCount--;
        }
        if (m_Debug) {
            qDebug() << "Client::endRequest success=" << success
                     << "status=" << httpStatus
                     << "url=" << url
                     << "error=" << errorMessage;
        }
        emit requestStateChanged(m_pendingRequestCount > 0, {}, {});
    }

    Client::Client(QObject *parent) : QObject(parent) {
    }

    Client::~Client() {
        if (m_WebSocket.isValid()) {
            m_WebSocket.close();
        }
    }

    bool Client::init(QMap<QString, QVariant> params) {
        m_Active = true;

        if (params.contains("debug")) {
            m_Debug = params["debug"].toBool();
        }

        if (params.contains("url")) {
            m_Url = QUrl(params["url"].toString());
        }

        if (params.contains("ssl")) {
            m_Ssl = params["ssl"].toBool();
        }

        if (params.contains("token")) {
            m_Token = params["token"].toString();
        }

        if (params.contains("cookie")) {
            m_Cookie = params["cookie"].toString();
        }

        if (m_Debug) qDebug() << "SignalK::Client::init url=" << m_Url;

        m_reconnectTimer.setSingleShot(true);
        m_reconnectTimer.setInterval(kReconnectIntervalMs);
        connect(&m_reconnectTimer, &QTimer::timeout, this, &Client::attemptReconnect);

        m_streamHealthTimer.setInterval(kStreamHealthCheckIntervalMs);
        connect(&m_streamHealthTimer, &QTimer::timeout, this, &Client::onStreamHealthTimeout);

        m_plannedRestartTimer.setSingleShot(true);
        connect(&m_plannedRestartTimer, &QTimer::timeout, this, [this]() {
            m_plannedRestartInProgress = false;
        });

        startAsyncReconnectDiscovery();

        return true;
    }

    void Client::startAsyncReconnectDiscovery() {
        if (!m_Active || m_Url.isEmpty()) {
            return;
        }

        if (m_asyncDiscoveryWatcher) {
            return;
        }

        const auto future = QtConcurrent::run([this]() -> bool {
            return refreshServerDiscovery();
        });
        m_asyncDiscoveryWatcher = new QFutureWatcher<bool>(this);
        connect(m_asyncDiscoveryWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
            const bool ok = m_asyncDiscoveryWatcher->result();
            m_asyncDiscoveryWatcher->deleteLater();
            m_asyncDiscoveryWatcher = nullptr;
            if (ok) {
                connectWebSocket();
            } else {
                qWarning() << "SignalK::Client::startAsyncReconnectDiscovery: discovery failed; retrying in"
                           << kReconnectIntervalMs << "ms";
                m_reconnectTimer.start();
                emitConnectivityState(tr("Signal K server not found"));
            }
        });
        m_asyncDiscoveryWatcher->setFuture(future);
        emitConnectivityState(m_hadStreamConnection ? tr("Reconnecting to Signal K") : tr("Connecting to Signal K"));
    }

    void Client::connectWebSocket() {
        if (!m_Active || m_Url.isEmpty()) {
            return;
        }

        const QUrl wsUrl = ws();
        if (!wsUrl.isValid() || wsUrl.isEmpty()) {
            qWarning() << "SignalK::Client::connectWebSocket: ws() returned empty URL, retrying...";
            m_reconnectTimer.start();
            emitConnectivityState(tr("Signal K WebSocket endpoint not found"));
            return;
        }

        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        if (m_Ssl) {
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        }
        m_WebSocket.setSslConfiguration(sslConfig);
        m_WebSocket.setParent(this);

        connect(&m_WebSocket, &QWebSocket::connected, this, &Client::onConnected, Qt::UniqueConnection);
        connect(&m_WebSocket, &QWebSocket::disconnected, this, &Client::onDisconnected, Qt::UniqueConnection);
        connect(&m_WebSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            qWarning() << "SignalK::Client::connectWebSocket WebSocket error:" << m_WebSocket.errorString();
            if (m_WebSocket.state() != QAbstractSocket::ConnectedState) {
                setStreamHealth(false, tr("WebSocket error: %1").arg(m_WebSocket.errorString()));
                m_reconnectTimer.start();
            }
        }, Qt::UniqueConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        connect(&m_WebSocket, &QWebSocket::handshakeInterruptedOnError, this, [this](const QSslError &error) {
            qWarning() << "SignalK::Client::connectWebSocket TLS handshake interrupted:" << error.errorString();
        }, Qt::UniqueConnection);
#endif

        auto webSocketUrl = QUrl(ws().toString() + "?subscribe=none&heartbeat=5000");
        if (!m_Token.isEmpty()) {
            QUrlQuery query(webSocketUrl);
            query.addQueryItem("token", m_Token);
            webSocketUrl.setQuery(query);
        }

        qInfo() << "SignalK::Client::connectWebSocket url=" << webSocketUrl;
        m_WebSocket.open(webSocketUrl);
    }

    void Client::stop() {
        m_Active = false;
        m_reconnectTimer.stop();
        m_streamHealthTimer.stop();
        m_plannedRestartTimer.stop();
        if (m_asyncDiscoveryWatcher) {
            m_asyncDiscoveryWatcher->cancel();
            m_asyncDiscoveryWatcher->deleteLater();
            m_asyncDiscoveryWatcher = nullptr;
        }
        if (m_WebSocket.state() != QAbstractSocket::UnconnectedState) {
            m_WebSocket.close();
        }
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

        // Pre-fetch and cache the self URN once so resubscribeAll() resolves all
        // "vessels.self" contexts consistently without N blocking HTTP calls
        if (m_selfUrn.isEmpty()) {
            const QString resolvedSelf = getSelf();
            if (!resolvedSelf.isEmpty()) {
                m_selfUrn = resolvedSelf;
                qInfo() << "SignalK::Client::onConnected cached selfUrn =" << m_selfUrn;
            } else {
                qWarning() << "SignalK::Client::onConnected getSelf() returned empty; "
                              "live-stream matching will rely on vessels.self alias";
            }
        }

        const bool recoveredFromDisconnect = m_reconnectRecoveryPending;
        resubscribeAll(true);
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

        m_reconnectRecoveryPending = m_hadStreamConnection || m_reconnectRecoveryPending;
        setStreamHealth(false, tr("Signal K stream offline"));
        m_reconnectTimer.start();
    }
//! [onDisconnected]

    void Client::onTextMessageReceived(const QString& textMessage) {
        const auto parsedDelta = parseSignalKDelta(textMessage);

        if (!parsedDelta.isHeartbeat && parsedDelta.values.isEmpty()) {
            if (m_Debug) {
                qDebug() << "SignalK::Client::onTextMessageReceived non-data message ignored";
            }
            return;
        }

        // Only a structurally valid delta proves that the data stream is healthy.
        markStreamActivity(tr("Stream online"));

        const QString context = parsedDelta.values.isEmpty() ? QString() : parsedDelta.values.first().context;

        if (m_Debug)
            qDebug() << "SignalK::Client::onTextMessageReceived context=" << context
                     << "values=" << parsedDelta.values.size();

        // Determine once per message whether context is the self vessel.
        // If m_selfUrn is not yet cached, fall back to the discovery-document self field
        // so that subscriptions using "vessels.self" alias can still dispatch correctly.
        const QString discoveryUrn = m_Server.value(QStringLiteral("self")).toString();
        const bool isSelfContext = (!m_selfUrn.isEmpty() && context == m_selfUrn)
                                || (!discoveryUrn.isEmpty() && context == discoveryUrn);
        if (isSelfContext && m_selfUrn.isEmpty() && !discoveryUrn.isEmpty()) {
            m_selfUrn = discoveryUrn;
            qInfo() << "SignalK::Client::onTextMessageReceived cached selfUrn from live message =" << m_selfUrn;
        }

        for (const DeltaValue &deltaValue : parsedDelta.values) {
            const QString path = deltaValue.path;
            const QString fullPath = context + "." + path;

                // Build a single-path delta so slots receive exactly the one value they
                // subscribed for; passing the full batched update would cause
                // getDoubleFromUpdateByPath("") to average all numeric values in the message
            const QJsonObject perPathUpdate = buildDeltaUpdate(context, path, deltaValue.value);

                // If the server sends updates with the actual vessel URN but some subscriptions
                // still hold the "vessels.self" context (getSelf failed at startup), build an
                // alias path so those subscriptions can still match.
            const QString selfAliasPath = isSelfContext
                ? QStringLiteral("vessels.self.") + path
                : QString();

            if (m_Debug)
                qDebug() << "SignalK::Client::onTextMessageReceived dispatching"
                         << fullPath
                         << (isSelfContext ? "(+ vessels.self alias)" : "");

            for (auto subscription: m_subscriptions) {
                subscription.match(fullPath, perPathUpdate);
                if (!selfAliasPath.isEmpty()) {
                    subscription.match(selfAliasPath, perPathUpdate);
                }
            }
        }
    }
    //! [onTextMessageReceived]

    void Client::onStreamHealthTimeout() {
        if (m_lastStreamActivity.isNull()) {
            return;
        }

        const qint64 elapsed = m_lastStreamActivity.msecsTo(QDateTime::currentDateTimeUtc());
        if (elapsed >= kStreamTimeoutMs) {
            qWarning() << "SignalK::Client::onStreamHealthTimeout: stream silent for" << elapsed << "ms";
            setStreamHealth(false, tr("Signal K stream timed out"));
            if (m_WebSocket.state() == QAbstractSocket::ConnectedState) {
                m_WebSocket.close();
            }
        }
    }

    void Client::markStreamActivity(const QString &statusText) {
        m_lastStreamActivity = QDateTime::currentDateTimeUtc();
        setStreamHealth(true, statusText);
        if (!m_streamHealthTimer.isActive()) {
            m_streamHealthTimer.start();
        }
    }

    void Client::setStreamHealth(const bool healthy, const QString &statusText) {
        const bool restHealthy = !m_Server.isEmpty();
        const ConnectionHealthState newState = [&]() {
            if (restHealthy && healthy) {
                return ConnectionHealthState::Online;
            }
            if (restHealthy && !healthy) {
                return m_hadStreamConnection ? ConnectionHealthState::Reconnecting : ConnectionHealthState::Connecting;
            }
            if (!restHealthy && !healthy) {
                return m_hadStreamConnection ? ConnectionHealthState::Stale : ConnectionHealthState::Connecting;
            }
            return ConnectionHealthState::Connecting;
        }();

        if (newState == m_connectionHealthState && statusText == m_connectionStatusText) {
            return;
        }
        m_connectionHealthState = newState;
        m_connectionStatusText = statusText;
        emit connectionHealthStateChanged(newState, statusText, QDateTime::currentDateTimeUtc(), m_selfUrn);
        emitConnectivityState(statusText);
    }

    void Client::emitConnectivityState(const QString &statusText) {
        const bool restHealthy = !m_Server.isEmpty();
        const bool streamHealthy = m_connectionHealthState == ConnectionHealthState::Online;
        emit connectivityChanged(restHealthy, streamHealthy, statusText);
    }

    QString Client::normalizedSubscriptionContext(const QString &context) const {
        if (context == "vessels.self") {
            // Use cached URN to avoid repeated blocking HTTP calls for every subscription
            if (!m_selfUrn.isEmpty()) {
                return m_selfUrn;
            }
            const QString resolved = const_cast<Client *>(this)->getSelf();
            if (!resolved.isEmpty()) {
                const_cast<Client *>(this)->m_selfUrn = resolved;
                return resolved;
            }
            return context;
        }

        return context;
    }

    QString Client::subscriptionMessage(const QString &context, const QString &path, const int period, const QString &policy, const int minPeriod) const {
        const QString effectivePolicy = normalizedSubscriptionPolicy(policy);
        return QString("{\n"
                       "  \"context\": \"%1\",\n"
                       "  \"subscribe\": [\n"
                       "    {\n"
                       "      \"path\": \"%2\",\n"
                       "      \"period\": %3,\n"
                       "      \"policy\": \"%4\",\n"
                       "      \"minPeriod\": %5\n"
                       "    }\n"
                       "  ]\n"
                       "}").arg(context, path, QString::number(period), effectivePolicy, QString::number(minPeriod));
    }

    bool Client::refreshServerDiscovery() {
        if (m_Url.isEmpty()) {
            return false;
        }

        QJsonObject server;
        bool success = false;
        QString message;
        int httpStatus = 0;
        const QByteArray data = finishReply(
            m_NetworkAccessManager.get(createJsonRequest(m_Url)),
            false, &success, &message, &httpStatus);

        if (!success || data.isEmpty()) {
            qWarning() << "SignalK::Client::refreshServerDiscovery: request failed status=" << httpStatus
                       << "error=" << message;
            return false;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "SignalK::Client::refreshServerDiscovery: invalid JSON";
            return false;
        }

        server = doc.object();
        if (server.isEmpty()) {
            qWarning() << "SignalK::Client::refreshServerDiscovery: empty discovery document";
            return false;
        }

        m_Server = server;
        qInfo() << "SignalK::Client::refreshServerDiscovery: server=" << m_Server;
        emit serverMessageChanged(discoveryMessage());
        return true;
    }

    QString Client::serverFingerprint(const QJsonObject &server) const {
        if (server.isEmpty()) {
            return {};
        }
        const QString version = server.value(QStringLiteral("version")).toString();
        const QString self = server.value(QStringLiteral("self")).toString();
        return version + "|" + self;
    }

    bool Client::hasSubscription(const QString &context, const QString &path, QObject *receiver) const {
        for (const auto &sub : m_subscriptions) {
            if (sub.getRequestedContext() == context && sub.getPath() == path && sub.getReceiver() == receiver) {
                return true;
            }
        }
        return false;
    }

    QString Client::getSelf() {
        if (!m_Server.isEmpty() && m_Server.contains("self")) {
            const QString selfFromDiscovery = m_Server.value("self").toString();
            if (!selfFromDiscovery.isEmpty()) {
                if (m_Debug) qDebug() << "SignalK::Client::getSelf from discovery:" << selfFromDiscovery;
                return selfFromDiscovery;
            }
        }

        const QString selfUrl = http().toString() + "/self";
        if (m_Debug) qDebug() << "SignalK::Client::getSelf url=" << selfUrl;

        auto result = httpGet(QUrl(selfUrl));
        if (result.isEmpty()) {
            qWarning() << "SignalK::Client::getSelf: empty response";
            return {};
        }

        QString selfStr = QString::fromUtf8(result).trimmed();
        selfStr.remove('"');
        if (m_Debug) qDebug() << "SignalK::Client::getSelf result=" << selfStr;
        return selfStr;
    }

    QString Client::discoveryMessage() const {
        if (m_Server.isEmpty()) {
            return tr("Signal K server not found");
        }
        const QString version = m_Server.value(QStringLiteral("version")).toString();
        const QString self    = m_Server.value(QStringLiteral("self")).toString();
        return tr("Signal K %1 [%2]").arg(version, self);
    }

    QUrl Client::http(const QString& version) {
        QUrl url = getEndpointByProtocol("https", version);
        if (!url.isValid() || url.isEmpty()) {
            url = getEndpointByProtocol("http", version);
        }
        if (m_Url.scheme() == "https" && url.scheme() == "http") {
            url.setScheme("https");
        }
        return url;
    }

    QUrl Client::ws(const QString& version) {
        QUrl url = getEndpointByProtocol("wss", version);
        if (!url.isValid() || url.isEmpty()) {
            url = getEndpointByProtocol("ws", version);
        }
        if (m_Url.scheme() == "https" && url.scheme() == "ws") {
            url.setScheme("wss");
        }
        return url;
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

        // Pre-fetch and cache the self URN once so resubscribeAll() resolves all
        // "vessels.self" contexts consistently without N blocking HTTP calls
        if (m_selfUrn.isEmpty()) {
            const QString resolvedSelf = getSelf();
            if (!resolvedSelf.isEmpty()) {
                m_selfUrn = resolvedSelf;
                qInfo() << "SignalK::Client::onConnected cached selfUrn =" << m_selfUrn;
            } else {
                qWarning() << "SignalK::Client::onConnected getSelf() returned empty; "
                              "live-stream matching will rely on vessels.self alias";
            }
        }

        const bool recoveredFromDisconnect = m_reconnectRecoveryPending;
        resubscribeAll(true);
        m_hadStreamConnection = true;
        m_reconnectRecoveryPending = false;
        emit serverStateResynchronized(recoveredFromDisconnect);
    }
//! [onConnected]