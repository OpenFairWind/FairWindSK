#include "GeoCoordinateUtils.hpp"

#include <QObject>
#include <QtMath>
#include <QRegularExpression>

namespace fairwindsk::ui::geo {
    namespace {
        const QList<CoordinateFormatOption> kFormats = {
            {QStringLiteral("deg_min_prime_decimal"), QStringLiteral("DD°MM'.MMM")},
            {QStringLiteral("dms_colon"), QStringLiteral("DD:MM:SS")},
            {QStringLiteral("dms_colon_decimal"), QStringLiteral("DD:MM:SS.S")},
            {QStringLiteral("dm_colon_decimal"), QStringLiteral("DD:MM.MMM")},
            {QStringLiteral("dms_symbols"), QStringLiteral("DD°MM'SS")},
            {QStringLiteral("dm_symbols_decimal"), QStringLiteral("DD°MM.MMM'")},
            {QStringLiteral("decimal_degrees"), QStringLiteral("DD.DDDDDD")}
        };

        QString hemisphereFor(double value, const bool latitude) {
            if (latitude) {
                return value < 0.0 ? QStringLiteral("S") : QStringLiteral("N");
            }
            return value < 0.0 ? QStringLiteral("W") : QStringLiteral("E");
        }

        int degreeWidth(const bool latitude) {
            return latitude ? 2 : 3;
        }

        QString paddedDegrees(const int degrees, const bool latitude) {
            return QStringLiteral("%1").arg(degrees, degreeWidth(latitude), 10, QLatin1Char('0'));
        }

        QStringList extractedNumbers(const QString &text) {
            QStringList values;
            const QRegularExpression matcher(QStringLiteral("[-+]?\\d+(?:\\.\\d+)?"));
            auto iterator = matcher.globalMatch(text);
            while (iterator.hasNext()) {
                values.append(iterator.next().captured(0));
            }
            return values;
        }

        bool validateCoordinate(const double value, const bool latitude, QString *message) {
            const double limit = latitude ? 90.0 : 180.0;
            if (qAbs(value) > limit) {
                if (message) {
                    *message = latitude
                               ? QObject::tr("Latitude must be between -90 and 90 degrees.")
                               : QObject::tr("Longitude must be between -180 and 180 degrees.");
                }
                return false;
            }
            return true;
        }

        QString formatDegreesMinutes(const double value,
                                     const bool latitude,
                                     const QString &middleSeparator,
                                     const QString &tailSeparator,
                                     const QString &fractionSuffix = QString()) {
            const double absoluteValue = qAbs(value);
            int degrees = int(qFloor(absoluteValue));
            double minutesTotal = (absoluteValue - degrees) * 60.0;
            int minutes = int(qFloor(minutesTotal));
            constexpr int fractionScale = 1000;
            int fraction = int(qRound((minutesTotal - minutes) * fractionScale));
            if (fraction >= fractionScale) {
                fraction = 0;
                ++minutes;
            }
            if (minutes >= 60) {
                minutes = 0;
                ++degrees;
            }

            return QStringLiteral("%1%2%3%4%5%6 %7")
                    .arg(paddedDegrees(degrees, latitude),
                         middleSeparator,
                         QStringLiteral("%1").arg(minutes, 2, 10, QLatin1Char('0')),
                         tailSeparator,
                         QStringLiteral("%1").arg(fraction, 3, 10, QLatin1Char('0')),
                         fractionSuffix,
                         hemisphereFor(value, latitude));
        }

        QString formatDegreesMinutesSeconds(const double value,
                                            const bool latitude,
                                            const QString &pattern,
                                            const int decimals) {
            const double absoluteValue = qAbs(value);
            int degrees = int(qFloor(absoluteValue));
            const double minutesBase = (absoluteValue - degrees) * 60.0;
            int minutes = int(qFloor(minutesBase));
            double seconds = (minutesBase - minutes) * 60.0;
            const double scale = qPow(10.0, decimals);
            seconds = qRound(seconds * scale) / scale;
            if (seconds >= 60.0) {
                seconds = 0.0;
                ++minutes;
            }
            if (minutes >= 60) {
                minutes = 0;
                ++degrees;
            }

            QString secondText;
            if (decimals == 0) {
                secondText = QStringLiteral("%1").arg(int(qRound(seconds)), 2, 10, QLatin1Char('0'));
            } else {
                secondText = QStringLiteral("%1").arg(seconds, 2 + decimals + 1, 'f', decimals, QLatin1Char('0'));
            }

            return pattern.arg(paddedDegrees(degrees, latitude),
                               QStringLiteral("%1").arg(minutes, 2, 10, QLatin1Char('0')),
                               secondText,
                               hemisphereFor(value, latitude));
        }
    }

    QList<CoordinateFormatOption> coordinateFormatOptions() {
        return kFormats;
    }

    QString defaultCoordinateFormatId() {
        return QStringLiteral("dm_symbols_decimal");
    }

    QString normalizeCoordinateFormatId(const QString &formatId) {
        for (const auto &option : kFormats) {
            if (option.id == formatId) {
                return option.id;
            }
        }
        return defaultCoordinateFormatId();
    }

    QString coordinateFormatLabel(const QString &formatId) {
        const QString normalized = normalizeCoordinateFormatId(formatId);
        for (const auto &option : kFormats) {
            if (option.id == normalized) {
                return option.label;
            }
        }
        return normalized;
    }

    QString formatSingleCoordinate(const double value, const bool latitude, const QString &formatId) {
        const QString normalized = normalizeCoordinateFormatId(formatId);
        if (normalized == QStringLiteral("decimal_degrees")) {
            return QStringLiteral("%1 %2")
                .arg(QString::number(qAbs(value), 'f', 6),
                     hemisphereFor(value, latitude));
        }
        if (normalized == QStringLiteral("dm_colon_decimal")) {
            return formatDegreesMinutes(value, latitude, QStringLiteral(":"), QStringLiteral("."));
        }
        if (normalized == QStringLiteral("deg_min_prime_decimal")) {
            return formatDegreesMinutes(value, latitude, QStringLiteral("°"), QStringLiteral("'."), QString());
        }
        if (normalized == QStringLiteral("dm_symbols_decimal")) {
            return formatDegreesMinutes(value, latitude, QStringLiteral("°"), QStringLiteral("."), QStringLiteral("'"));
        }
        if (normalized == QStringLiteral("dms_colon")) {
            return formatDegreesMinutesSeconds(value, latitude, QStringLiteral("%1:%2:%3 %4"), 0);
        }
        if (normalized == QStringLiteral("dms_colon_decimal")) {
            return formatDegreesMinutesSeconds(value, latitude, QStringLiteral("%1:%2:%3 %4"), 1);
        }
        return formatDegreesMinutesSeconds(value, latitude, QStringLiteral("%1°%2'%3\" %4"), 0);
    }

    QString formatCoordinate(const double latitude, const double longitude, const QString &formatId) {
        return formatSingleCoordinate(latitude, true, formatId) + QStringLiteral("  ") +
               formatSingleCoordinate(longitude, false, formatId);
    }

    QString formatCoordinate(const QGeoCoordinate &coordinate, const QString &formatId) {
        if (!coordinate.isValid()) {
            return {};
        }
        return formatCoordinate(coordinate.latitude(), coordinate.longitude(), formatId);
    }

    bool parseSingleCoordinate(const QString &text,
                               const bool latitude,
                               const QString &formatId,
                               double *value,
                               QString *message) {
        if (!value) {
            return false;
        }

        const QString trimmed = text.trimmed().toUpper();
        if (trimmed.isEmpty()) {
            if (message) {
                *message = latitude ? QObject::tr("Latitude is required.") : QObject::tr("Longitude is required.");
            }
            return false;
        }

        int sign = trimmed.startsWith(QLatin1Char('-')) ? -1 : 1;
        if (trimmed.contains(QLatin1Char('S')) || trimmed.contains(QLatin1Char('W'))) {
            sign = -1;
        } else if (trimmed.contains(QLatin1Char('N')) || trimmed.contains(QLatin1Char('E'))) {
            sign = 1;
        }

        const QString normalized = normalizeCoordinateFormatId(formatId);
        const QStringList numbers = extractedNumbers(trimmed);
        if (numbers.isEmpty()) {
            if (message) {
                *message = latitude ? QObject::tr("Latitude format is invalid.") : QObject::tr("Longitude format is invalid.");
            }
            return false;
        }

        double parsedValue = 0.0;
        bool ok = false;
        if (normalized == QStringLiteral("decimal_degrees")) {
            parsedValue = numbers.first().toDouble(&ok);
        } else if (normalized == QStringLiteral("dm_colon_decimal") ||
                   normalized == QStringLiteral("deg_min_prime_decimal") ||
                   normalized == QStringLiteral("dm_symbols_decimal")) {
            if (numbers.size() < 2) {
                if (message) {
                    *message = latitude ? QObject::tr("Latitude requires degrees and minutes.")
                                        : QObject::tr("Longitude requires degrees and minutes.");
                }
                return false;
            }

            const double degrees = numbers.at(0).toDouble(&ok);
            if (!ok) {
                return false;
            }
            double minutes = 0.0;
            if (normalized == QStringLiteral("deg_min_prime_decimal") && numbers.size() >= 3) {
                const int wholeMinutes = numbers.at(1).toInt(&ok);
                if (!ok) {
                    return false;
                }
                const QString fractionText = numbers.at(2);
                minutes = wholeMinutes + (QStringLiteral("0.%1").arg(fractionText).toDouble(&ok));
            } else {
                minutes = numbers.at(1).toDouble(&ok);
            }
            if (!ok || minutes >= 60.0) {
                if (message) {
                    *message = QObject::tr("Minutes must be less than 60.");
                }
                return false;
            }
            parsedValue = qAbs(degrees) + (minutes / 60.0);
        } else {
            if (numbers.size() < 3) {
                if (message) {
                    *message = latitude ? QObject::tr("Latitude requires degrees, minutes, and seconds.")
                                        : QObject::tr("Longitude requires degrees, minutes, and seconds.");
                }
                return false;
            }

            const double degrees = numbers.at(0).toDouble(&ok);
            if (!ok) {
                return false;
            }
            const double minutes = numbers.at(1).toDouble(&ok);
            const double seconds = numbers.at(2).toDouble(&ok);
            if (!ok || minutes >= 60.0 || seconds >= 60.0) {
                if (message) {
                    *message = QObject::tr("Minutes and seconds must be less than 60.");
                }
                return false;
            }
            parsedValue = qAbs(degrees) + (minutes / 60.0) + (seconds / 3600.0);
        }

        parsedValue *= sign;
        if (!validateCoordinate(parsedValue, latitude, message)) {
            return false;
        }

        *value = parsedValue;
        return true;
    }
}
