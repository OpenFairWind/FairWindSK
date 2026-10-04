#include "PList.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QtTest>

class PListTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesAllSupportedTypes();
    void serializesAndParsesNestedValues();
    void handlesMalformedInput();
};

void PListTest::parsesAllSupportedTypes() {
    QByteArray xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0"><dict>
<key>text</key><string>helm</string>
<key>integer</key><integer>42</integer>
<key>real</key><real>3.5</real>
<key>enabled</key><true/>
<key>disabled</key><false/>
<key>payload</key><data>RmFpcldpbmRTSw==</data>
<key>when</key><date>2026-07-21T10:00:00Z</date>
</dict></plist>)";
    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    const PList plist(&buffer);
    const QVariantMap values = plist.toMap();
    QCOMPARE(values.value(QStringLiteral("text")).toString(), QStringLiteral("helm"));
    QCOMPARE(values.value(QStringLiteral("integer")).toInt(), 42);
    QCOMPARE(values.value(QStringLiteral("real")).toDouble(), 3.5);
    QCOMPARE(values.value(QStringLiteral("enabled")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("disabled")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("payload")).toByteArray(), QByteArray("FairWindSK"));
    QCOMPARE(values.value(QStringLiteral("when")).toDateTime().toUTC(),
             QDateTime(QDate(2026, 7, 21), QTime(10, 0), QTimeZone::UTC));
}

void PListTest::serializesAndParsesNestedValues() {
    QVariantMap source;
    source.insert(QStringLiteral("name"), QStringLiteral("FairWindSK"));
    source.insert(QStringLiteral("items"), QVariantList{1, QStringLiteral("two"), true});
    source.insert(QStringLiteral("nested"), QVariantMap{{QStringLiteral("depth"), 7.25}});

    QByteArray emptyXml = QByteArrayLiteral("<plist version=\"1.0\"><dict/></plist>");
    QBuffer emptyBuffer(&emptyXml);
    QVERIFY(emptyBuffer.open(QIODevice::ReadOnly));
    PList serializer(&emptyBuffer);

    QByteArray serialized = serializer.toPList(source).toUtf8();
    QBuffer serializedBuffer(&serialized);
    QVERIFY(serializedBuffer.open(QIODevice::ReadOnly));
    const PList parsed(&serializedBuffer);
    QCOMPARE(parsed.toMap(), source);
}

void PListTest::handlesMalformedInput() {
    QByteArray malformed = QByteArrayLiteral("<plist><dict><key>broken</key>");
    QBuffer buffer(&malformed);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    const PList plist(&buffer);
    QVERIFY(plist.toMap().isEmpty());
}

QTEST_APPLESS_MAIN(PListTest)

#include "PListTest.moc"
