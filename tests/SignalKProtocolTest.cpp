#include "signalk/ProtocolUtils.hpp"

#include <QJsonObject>
#include <QTimeZone>
#include <QtTest>

using fairwindsk::signalk::ReconnectEvent;
using fairwindsk::signalk::ReconnectState;

Q_DECLARE_METATYPE(ReconnectEvent)
Q_DECLARE_METATYPE(ReconnectState)

class SignalKProtocolTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesValues_data();
    void parsesValues();
    void rejectsMalformedPayloads_data();
    void rejectsMalformedPayloads();
    void calculatesStaleness_data();
    void calculatesStaleness();
    void reducesReconnectTransitions_data();
    void reducesReconnectTransitions();
};

void SignalKProtocolTest::parsesValues_data() {
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("firstPath");
    QTest::newRow("number") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"navigation.speedOverGround","value":4.2}]}]})") << 1 << QStringLiteral("navigation.speedOverGround");
    QTest::newRow("position") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"navigation.position","value":{"latitude":1,"longitude":2}}]}]})") << 1 << QStringLiteral("navigation.position");
    QTest::newRow("null") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"navigation.log","value":null}]}]})") << 1 << QStringLiteral("navigation.log");
    QTest::newRow("boolean") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"electrical.switches.test","value":true}]}]})") << 1 << QStringLiteral("electrical.switches.test");
    QTest::newRow("two-values") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"a","value":1},{"path":"b","value":2}]}]})") << 2 << QStringLiteral("a");
    QTest::newRow("two-updates") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"a","value":1}]},{"values":[{"path":"b","value":2}]}]})") << 2 << QStringLiteral("a");
    QTest::newRow("skip-empty-path") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"","value":1},{"path":"b","value":2}]}]})") << 1 << QStringLiteral("b");
    QTest::newRow("skip-missing-value") << QByteArray(R"({"context":"vessels.self","updates":[{"values":[{"path":"a"},{"path":"b","value":2}]}]})") << 1 << QStringLiteral("b");
}

void SignalKProtocolTest::parsesValues() {
    QFETCH(QByteArray, payload);
    QFETCH(int, count);
    QFETCH(QString, firstPath);
    const auto result = fairwindsk::signalk::parseDelta(payload);
    QVERIFY(result.validEnvelope);
    QCOMPARE(result.values.size(), count);
    QCOMPARE(result.values.first().path, firstPath);
}

void SignalKProtocolTest::rejectsMalformedPayloads_data() {
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<bool>("validEnvelope");
    QTest::newRow("empty") << QByteArray() << false;
    QTest::newRow("broken-json") << QByteArray("{") << false;
    QTest::newRow("array-root") << QByteArray("[]") << false;
    QTest::newRow("null-root") << QByteArray("null") << false;
    QTest::newRow("missing-updates") << QByteArray("{}") << false;
    QTest::newRow("updates-object") << QByteArray(R"({"updates":{}})") << false;
    QTest::newRow("empty-updates") << QByteArray(R"({"updates":[]})") << true;
    QTest::newRow("invalid-update-item") << QByteArray(R"({"updates":[42]})") << true;
}

void SignalKProtocolTest::rejectsMalformedPayloads() {
    QFETCH(QByteArray, payload);
    QFETCH(bool, validEnvelope);
    const auto result = fairwindsk::signalk::parseDelta(payload);
    QCOMPARE(result.validEnvelope, validEnvelope);
    QVERIFY(result.values.isEmpty());
}

void SignalKProtocolTest::calculatesStaleness_data() {
    QTest::addColumn<qint64>("ageMs");
    QTest::addColumn<qint64>("timeoutMs");
    QTest::addColumn<bool>("stale");
    QTest::newRow("fresh") << qint64(100) << qint64(1000) << false;
    QTest::newRow("boundary") << qint64(1000) << qint64(1000) << false;
    QTest::newRow("past-boundary") << qint64(1001) << qint64(1000) << true;
    QTest::newRow("long-gap") << qint64(60000) << qint64(1000) << true;
    QTest::newRow("clock-backwards") << qint64(-1) << qint64(1000) << false;
    QTest::newRow("zero-timeout-fresh") << qint64(0) << qint64(0) << false;
    QTest::newRow("zero-timeout-old") << qint64(1) << qint64(0) << true;
}

void SignalKProtocolTest::calculatesStaleness() {
    QFETCH(qint64, ageMs);
    QFETCH(qint64, timeoutMs);
    QFETCH(bool, stale);
    const QDateTime last = QDateTime::fromMSecsSinceEpoch(100000, QTimeZone::UTC);
    QCOMPARE(fairwindsk::signalk::isDataStale(last, last.addMSecs(ageMs), timeoutMs), stale);
}

void SignalKProtocolTest::reducesReconnectTransitions_data() {
    QTest::addColumn<ReconnectState>("state");
    QTest::addColumn<ReconnectEvent>("event");
    QTest::addColumn<ReconnectState>("expected");
#define ROW(name, stateValue, eventValue, expectedValue) QTest::newRow(name) << ReconnectState::stateValue << ReconnectEvent::eventValue << ReconnectState::expectedValue
    ROW("start", Disconnected, Start, Connecting);
    ROW("initial-connected", Connecting, Connected, Live);
    ROW("initial-failure", Connecting, DiscoveryFailed, Waiting);
    ROW("initial-loss", Connecting, ConnectionLost, Waiting);
    ROW("live-loss", Live, ConnectionLost, Waiting);
    ROW("wait-retry", Waiting, RetryTimer, Recovering);
    ROW("recovered", Recovering, Connected, Live);
    ROW("recovery-discovery-failure", Recovering, DiscoveryFailed, Waiting);
    ROW("recovery-loss", Recovering, ConnectionLost, Waiting);
    ROW("ignore-duplicate-start", Connecting, Start, Connecting);
    ROW("ignore-retry-while-live", Live, RetryTimer, Live);
    ROW("ignore-connect-while-waiting", Waiting, Connected, Waiting);
    ROW("stop-live", Live, Stop, Disconnected);
    ROW("stop-waiting", Waiting, Stop, Disconnected);
    ROW("stop-recovering", Recovering, Stop, Disconnected);
#undef ROW
}

void SignalKProtocolTest::reducesReconnectTransitions() {
    QFETCH(ReconnectState, state);
    QFETCH(ReconnectEvent, event);
    QFETCH(ReconnectState, expected);
    QCOMPARE(fairwindsk::signalk::nextReconnectState(state, event), expected);
}

QTEST_APPLESS_MAIN(SignalKProtocolTest)
#include "SignalKProtocolTest.moc"
