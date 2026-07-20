#include "signalk/Subscription.hpp"
#include "signalk/Waypoint.hpp"

#include <QJsonArray>
#include <QtTest>

class UpdateReceiver final : public QObject {
    Q_OBJECT

public:
    QJsonObject received;

public slots:
    void acceptUpdate(const QJsonObject &update) {
        received = update;
    }
};

class SignalKValueObjectsTest final : public QObject {
    Q_OBJECT

private slots:
    void waypointRoundTrip();
    void waypointReadsGeoJsonProperties();
    void subscriptionMatchesAndInvokesReceiver();
    void subscriptionNormalizesPolicyAndContext();
};

void SignalKValueObjectsTest::waypointRoundTrip() {
    const QGeoCoordinate coordinate(41.902782, 12.496366, 15.5);
    fairwindsk::signalk::Waypoint waypoint(QStringLiteral("rome"), QStringLiteral("Rome"),
                                          QStringLiteral("Test waypoint"), QStringLiteral("harbor"), coordinate);

    QCOMPARE(waypoint.getId(), QStringLiteral("rome"));
    QCOMPARE(waypoint.getName(), QStringLiteral("Rome"));
    QCOMPARE(waypoint.getDescription(), QStringLiteral("Test waypoint"));
    QCOMPARE(waypoint.getType(), QStringLiteral("harbor"));
    QVERIFY(waypoint.getTimestamp().isValid());
    QCOMPARE(waypoint.getCoordinates(), coordinate);

    waypoint.setName(QStringLiteral("Roma"));
    waypoint.setDescription(QStringLiteral("Updated"));
    QCOMPARE(waypoint.getName(), QStringLiteral("Roma"));
    QCOMPARE(waypoint.value(QStringLiteral("feature")).toObject()
                 .value(QStringLiteral("properties")).toObject()
                 .value(QStringLiteral("description")).toString(), QStringLiteral("Updated"));
}

void SignalKValueObjectsTest::waypointReadsGeoJsonProperties() {
    QJsonObject properties{{QStringLiteral("name"), QStringLiteral("Fallback")},
                           {QStringLiteral("description"), QStringLiteral("From feature")},
                           {QStringLiteral("type"), QStringLiteral("generic")}};
    QJsonObject geometry{{QStringLiteral("type"), QStringLiteral("Point")},
                         {QStringLiteral("coordinates"), QJsonArray{2.0, 48.0}}};
    QJsonObject feature{{QStringLiteral("id"), QStringLiteral("feature-id")},
                        {QStringLiteral("properties"), properties},
                        {QStringLiteral("geometry"), geometry}};
    fairwindsk::signalk::Waypoint waypoint(QJsonObject{{QStringLiteral("feature"), feature}});

    QCOMPARE(waypoint.getId(), QStringLiteral("feature-id"));
    QCOMPARE(waypoint.getName(), QStringLiteral("Fallback"));
    QCOMPARE(waypoint.getCoordinates(), QGeoCoordinate(48.0, 2.0));
}

void SignalKValueObjectsTest::subscriptionMatchesAndInvokesReceiver() {
    UpdateReceiver receiver;
    fairwindsk::signalk::Subscription subscription(
        QStringLiteral("vessels.self"), QStringLiteral("vessels.urn:mrn:imo:mmsi:123"),
        QStringLiteral("navigation.*"), &receiver, SLOT(acceptUpdate(QJsonObject)));
    const QJsonObject update{{QStringLiteral("value"), 12.5}};

    QVERIFY(subscription.match(QStringLiteral("vessels.urn:mrn:imo:mmsi:123.navigation.speedOverGround"), update));
    QCOMPARE(receiver.received, update);
    QVERIFY(!subscription.match(QStringLiteral("vessels.other.environment.wind.speedTrue"), update));
}

void SignalKValueObjectsTest::subscriptionNormalizesPolicyAndContext() {
    UpdateReceiver receiver;
    fairwindsk::signalk::Subscription subscription(
        QStringLiteral("vessels.self"), QStringLiteral("vessels.old"), QStringLiteral("navigation.position"),
        &receiver, SLOT(acceptUpdate(QJsonObject)), 500, QStringLiteral("unsupported"), 100);

    QCOMPARE(subscription.getPolicy(), QStringLiteral("instant"));
    QCOMPARE(subscription.getPeriod(), 500);
    QCOMPARE(subscription.getMinPeriod(), 100);
    QVERIFY(subscription.checkReceiver(&receiver));

    subscription.retargetContext(QStringLiteral("vessels.new"));
    QVERIFY(subscription.getRegex().match(QStringLiteral("vessels.new.navigation.position")).hasMatch());
    QVERIFY(!subscription.getRegex().match(QStringLiteral("vessels.old.navigation.position")).hasMatch());
}

QTEST_APPLESS_MAIN(SignalKValueObjectsTest)

#include "SignalKValueObjectsTest.moc"
