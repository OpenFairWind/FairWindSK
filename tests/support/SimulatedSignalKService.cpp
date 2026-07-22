#include "SimulatedSignalKService.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QTcpSocket>
#include <QTimer>
#include <QWebSocket>

SimulatedSignalKService::SimulatedSignalKService(QObject *parent)
    : QObject(parent),
      m_webSocketServer(QStringLiteral("FairWindSK simulated Signal K"), QWebSocketServer::NonSecureMode) {
    connect(&m_httpServer, &QTcpServer::newConnection, this, &SimulatedSignalKService::acceptHttpConnection);
    connect(&m_webSocketServer, &QWebSocketServer::newConnection, this, [this]() {
        QWebSocket *stream = m_webSocketServer.nextPendingConnection();
        m_streams.append(stream);
        connect(stream, &QWebSocket::disconnected, this, [this, stream]() {
            m_streams.removeAll(stream);
            stream->deleteLater();
        });
    });
}

bool SimulatedSignalKService::start() {
    if (m_httpServer.isListening() || m_webSocketServer.isListening()) {
        return true;
    }
    if (!m_httpServer.listen(QHostAddress::LocalHost, 0)) {
        return false;
    }
    if (!m_webSocketServer.listen(QHostAddress::LocalHost, 0)) {
        m_httpServer.close();
        return false;
    }
    return true;
}

void SimulatedSignalKService::stop() {
    dropStreams();
    m_webSocketServer.close();
    m_httpServer.close();
}

void SimulatedSignalKService::setAuthenticationAccepted(const bool accepted) {
    m_authenticationAccepted = accepted;
}

void SimulatedSignalKService::setResponseDelay(const int delayMs) {
    m_responseDelayMs = qMax(0, delayMs);
}

void SimulatedSignalKService::setResource(const QString &id, const QJsonObject &resource) {
    m_resources.insert(id, resource);
}

QUrl SimulatedSignalKService::discoveryUrl() const {
    return QUrl(QStringLiteral("http://127.0.0.1:%1/signalk").arg(m_httpServer.serverPort()));
}

QUrl SimulatedSignalKService::webSocketUrl() const {
    return QUrl(QStringLiteral("ws://127.0.0.1:%1/signalk/v1/stream").arg(m_webSocketServer.serverPort()));
}

void SimulatedSignalKService::sendDelta(const QString &path, const QJsonValue &value) {
    const QJsonObject item{{QStringLiteral("path"), path}, {QStringLiteral("value"), value}};
    const QJsonObject update{{QStringLiteral("values"), QJsonArray{item}}};
    const QJsonObject envelope{{QStringLiteral("context"), QStringLiteral("vessels.self")},
                               {QStringLiteral("updates"), QJsonArray{update}}};
    const QString payload = QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
    for (QWebSocket *stream : m_streams) {
        stream->sendTextMessage(payload);
    }
}

void SimulatedSignalKService::sendMalformedDelta() {
    for (QWebSocket *stream : m_streams) {
        stream->sendTextMessage(QStringLiteral("{malformed"));
    }
}

void SimulatedSignalKService::dropStreams() {
    const QList<QWebSocket *> streams = m_streams;
    for (QWebSocket *stream : streams) {
        stream->close(QWebSocketProtocol::CloseCodeGoingAway, QStringLiteral("simulated outage"));
    }
}

void SimulatedSignalKService::acceptHttpConnection() {
    while (QTcpSocket *socket = m_httpServer.nextPendingConnection()) {
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QByteArray request = socket->readAll();
            processHttpRequest(socket, request);
        });
    }
}

void SimulatedSignalKService::processHttpRequest(QTcpSocket *socket, const QByteArray &request) {
    const QList<QByteArray> requestLine = request.left(request.indexOf("\r\n")).split(' ');
    const QByteArray method = requestLine.value(0);
    const QByteArray path = requestLine.value(1);
    int status = 200;
    QJsonDocument body;

    if (path == "/signalk") {
        body = QJsonDocument::fromJson(discoveryDocument());
    } else if (path == "/signalk/v1/auth/login" && method == "POST") {
        status = m_authenticationAccepted ? 200 : 401;
        body = QJsonDocument(QJsonObject{{QStringLiteral("token"), m_authenticationAccepted ? QStringLiteral("test-token") : QString()}});
    } else if (path.startsWith("/signalk/v2/api/resources/waypoints")) {
        QJsonObject resources;
        for (auto iterator = m_resources.cbegin(); iterator != m_resources.cend(); ++iterator) {
            resources.insert(iterator.key(), iterator.value());
        }
        body = QJsonDocument(resources);
    } else {
        status = 404;
        body = QJsonDocument(QJsonObject{{QStringLiteral("message"), QStringLiteral("not found")}});
    }

    const QByteArray payload = body.toJson(QJsonDocument::Compact);
    const QByteArray reason = status == 200 ? QByteArray("OK") : (status == 401 ? QByteArray("Unauthorized") : QByteArray("Not Found"));
    const QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reason
                                + "\r\nContent-Type: application/json\r\nContent-Length: "
                                + QByteArray::number(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload;
    const QPointer<QTcpSocket> guardedSocket(socket);
    QTimer::singleShot(m_responseDelayMs, this, [guardedSocket, response]() {
        if (!guardedSocket) return;
        guardedSocket->write(response);
        guardedSocket->disconnectFromHost();
    });
}

QByteArray SimulatedSignalKService::discoveryDocument() const {
    const QJsonObject endpoints{{QStringLiteral("v1"), QJsonObject{
        {QStringLiteral("signalk-http"), QStringLiteral("http://127.0.0.1:%1/signalk/v1/api/").arg(m_httpServer.serverPort())},
        {QStringLiteral("signalk-ws"), webSocketUrl().toString()}}}};
    return QJsonDocument(QJsonObject{{QStringLiteral("self"), QStringLiteral("vessels.self")},
                                     {QStringLiteral("endpoints"), endpoints}}).toJson(QJsonDocument::Compact);
}
