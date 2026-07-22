#include "ui/GeoCoordinateUtils.hpp"

#include <QtTest>

namespace fairwindsk::tests {

    class GeoCoordinateUtilsTest final : public QObject {
        Q_OBJECT

    private slots:
        // Every advertised format must survive a format-and-parse round trip.
        void formatsRoundTrip();

        // Invalid values must be rejected without modifying the caller's value.
        void rejectsInvalidCoordinates();

        // Unknown stored format identifiers must retain the documented fallback.
        void normalizesUnknownFormat();
    };

    void GeoCoordinateUtilsTest::formatsRoundTrip() {
        constexpr double latitude = 41.902782;
        constexpr double longitude = 12.496366;

        const auto formats = ui::geo::coordinateFormatOptions();
        QCOMPARE(formats.size(), 7);

        for (const auto &format : formats) {
            const QString latitudeText = ui::geo::formatSingleCoordinate(latitude, true, format.id);
            const QString longitudeText = ui::geo::formatSingleCoordinate(longitude, false, format.id);
            double parsedLatitude = 0.0;
            double parsedLongitude = 0.0;

            QVERIFY2(ui::geo::parseSingleCoordinate(latitudeText, true, format.id, &parsedLatitude),
                     qPrintable(QStringLiteral("Could not parse latitude format %1: %2").arg(format.id, latitudeText)));
            QVERIFY2(ui::geo::parseSingleCoordinate(longitudeText, false, format.id, &parsedLongitude),
                     qPrintable(QStringLiteral("Could not parse longitude format %1: %2").arg(format.id, longitudeText)));

            // Whole-second formats intentionally have lower precision than the others.
            const double tolerance = format.id == QStringLiteral("dms_colon") ||
                                     format.id == QStringLiteral("dms_symbols")
                                     ? 0.00015
                                     : 0.00001;
            QVERIFY(qAbs(parsedLatitude - latitude) <= tolerance);
            QVERIFY(qAbs(parsedLongitude - longitude) <= tolerance);
        }
    }

    void GeoCoordinateUtilsTest::rejectsInvalidCoordinates() {
        double value = 123.0;
        QString message;

        QVERIFY(!ui::geo::parseSingleCoordinate(QStringLiteral("91 N"), true,
                                                 QStringLiteral("decimal_degrees"), &value, &message));
        QCOMPARE(value, 123.0);
        QVERIFY(!message.isEmpty());

        message.clear();
        QVERIFY(!ui::geo::parseSingleCoordinate(QStringLiteral("12:60:00 E"), false,
                                                 QStringLiteral("dms_colon"), &value, &message));
        QCOMPARE(value, 123.0);
        QVERIFY(!message.isEmpty());
    }

    void GeoCoordinateUtilsTest::normalizesUnknownFormat() {
        QCOMPARE(ui::geo::normalizeCoordinateFormatId(QStringLiteral("not-a-format")),
                 ui::geo::defaultCoordinateFormatId());
        QCOMPARE(ui::geo::coordinateFormatLabel(QStringLiteral("not-a-format")),
                 QStringLiteral("DD°MM.MMM'"));
    }

}

QTEST_APPLESS_MAIN(fairwindsk::tests::GeoCoordinateUtilsTest)

#include "GeoCoordinateUtilsTest.moc"
