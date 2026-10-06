#ifndef FAIRWINDSK_TESTS_SIMULATEDSIGNALKSERVICE_HPP
#define FAIRWINDSK_TESTS_SIMULATEDSIGNALKSERVICE_HPP

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>
#include <QWebSocketServer>

class SimulatedSignalKService final : public QObject {
    Q_OBJECT

public:
    explicit SimulatedSignalKService(QObject *parent = nullptr);
    bool start();
    void stop();
    void setAuthenticationAccepted(bool accepted);
    void setResponseDelay(int delayMs);
    void setResource(const QString &id, const QJsonObject &resource);
    QUrl discoveryUrl() const;
    QUrl webSocketUrl() const;
    void sendDelta(const QString &path, const QJsonValue &value);
    void sendMalformedDelta();
    void dropStreams();

private:
    void acceptHttpConnection();
    void processHttpRequest(QTcpSocket *socket, const QByteArray &request);
    QByteArray discoveryDocument() const;

    QTcpServer m_httpServer;
    QWebSocketServer m_webSocketServer;
    QList<QWebSocket *> m_streams;
    QHash<QString, QJsonObject> m_resources;
    bool m_authenticationAccepted = true;
    int m_responseDelayMs = 0;
};

#endif
