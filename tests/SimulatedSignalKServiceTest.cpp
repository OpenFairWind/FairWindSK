#include "tests/support/SimulatedSignalKService.hpp"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTimer>
#include <QWebSocket>
#include <QtTest>

class SimulatedSignalKServiceTest final : public QObject {
    Q_OBJECT

private:
    static QNetworkReply *request(QNetworkAccessManager &manager, const QUrl &url, const QByteArray &method = "GET");

private slots:
    void discoveryAndRestResources();
    void authenticationSuccessAndFailure();
    void delaysResponses();
    void streamsValidAndMalformedDeltas();
    void dropsConnections();
    void restartsAndExposesChangedResources();
};

QNetworkReply *SimulatedSignalKServiceTest::request(QNetworkAccessManager &manager, const QUrl &url, const QByteArray &method) {
    QNetworkRequest networkRequest(url);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    return method == "POST" ? manager.post(networkRequest, QByteArray("{}")) : manager.get(networkRequest);
}

void SimulatedSignalKServiceTest::discoveryAndRestResources() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    service.setResource(QStringLiteral("harbor"), QJsonObject{{QStringLiteral("name"), QStringLiteral("Harbor")}});
    QNetworkAccessManager manager;
    QNetworkReply *discovery = request(manager, service.discoveryUrl());
    QSignalSpy discoveryFinished(discovery, &QNetworkReply::finished);
    QVERIFY(discoveryFinished.wait());
    QCOMPARE(discovery->error(), QNetworkReply::NoError);
    QVERIFY(QJsonDocument::fromJson(discovery->readAll()).object().contains(QStringLiteral("endpoints")));
    discovery->deleteLater();
    const QUrl resources(QStringLiteral("http://127.0.0.1:%1/signalk/v2/api/resources/waypoints").arg(service.discoveryUrl().port()));
    QNetworkReply *reply = request(manager, resources);
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY(finished.wait());
    QVERIFY(QJsonDocument::fromJson(reply->readAll()).object().contains(QStringLiteral("harbor")));
    reply->deleteLater();
}

void SimulatedSignalKServiceTest::authenticationSuccessAndFailure() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    QNetworkAccessManager manager;
    const QUrl login(QStringLiteral("http://127.0.0.1:%1/signalk/v1/auth/login").arg(service.discoveryUrl().port()));
    QNetworkReply *accepted = request(manager, login, "POST");
    QSignalSpy acceptedFinished(accepted, &QNetworkReply::finished);
    QVERIFY(acceptedFinished.wait());
    QCOMPARE(accepted->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    accepted->deleteLater();
    service.setAuthenticationAccepted(false);
    QNetworkReply *rejected = request(manager, login, "POST");
    QSignalSpy rejectedFinished(rejected, &QNetworkReply::finished);
    QVERIFY(rejectedFinished.wait());
    QCOMPARE(rejected->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 401);
    rejected->deleteLater();
}

void SimulatedSignalKServiceTest::delaysResponses() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    service.setResponseDelay(80);
    QNetworkAccessManager manager;
    QElapsedTimer elapsed;
    elapsed.start();
    QNetworkReply *reply = request(manager, service.discoveryUrl());
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY(finished.wait());
    QVERIFY(elapsed.elapsed() >= 60);
    reply->deleteLater();
}

void SimulatedSignalKServiceTest::streamsValidAndMalformedDeltas() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    QWebSocket socket;
    QSignalSpy connected(&socket, &QWebSocket::connected);
    QSignalSpy messages(&socket, &QWebSocket::textMessageReceived);
    socket.open(service.webSocketUrl());
    QVERIFY(connected.wait());
    service.sendMalformedDelta();
    service.sendDelta(QStringLiteral("navigation.speedOverGround"), 4.2);
    QTRY_COMPARE(messages.count(), 2);
    QCOMPARE(messages.at(0).at(0).toString(), QStringLiteral("{malformed"));
    QVERIFY(messages.at(1).at(0).toString().contains(QStringLiteral("navigation.speedOverGround")));
}

void SimulatedSignalKServiceTest::dropsConnections() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    QWebSocket socket;
    QSignalSpy connected(&socket, &QWebSocket::connected);
    QSignalSpy disconnected(&socket, &QWebSocket::disconnected);
    socket.open(service.webSocketUrl());
    QVERIFY(connected.wait());
    service.dropStreams();
    QVERIFY(disconnected.wait());
}

void SimulatedSignalKServiceTest::restartsAndExposesChangedResources() {
    SimulatedSignalKService service;
    QVERIFY(service.start());
    service.setResource(QStringLiteral("before"), QJsonObject{{QStringLiteral("name"), QStringLiteral("Before")}});
    service.stop();
    service.setResource(QStringLiteral("during-restart"), QJsonObject{{QStringLiteral("name"), QStringLiteral("During restart")}});
    QVERIFY(service.start());
    QNetworkAccessManager manager;
    const QUrl resources(QStringLiteral("http://127.0.0.1:%1/signalk/v2/api/resources/waypoints").arg(service.discoveryUrl().port()));
    QNetworkReply *reply = request(manager, resources);
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY(finished.wait());
    const QJsonObject body = QJsonDocument::fromJson(reply->readAll()).object();
    QVERIFY(body.contains(QStringLiteral("before")));
    QVERIFY(body.contains(QStringLiteral("during-restart")));
    reply->deleteLater();
}

QTEST_MAIN(SimulatedSignalKServiceTest)
#include "SimulatedSignalKServiceTest.moc"
