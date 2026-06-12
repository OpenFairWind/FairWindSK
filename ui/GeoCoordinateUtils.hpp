#ifndef FAIRWINDSK_UI_GEOCOORDINATEUTILS_HPP
#define FAIRWINDSK_UI_GEOCOORDINATEUTILS_HPP

#include <QGeoCoordinate>
#include <QList>
#include <QString>

namespace fairwindsk::ui::geo {

    struct CoordinateFormatOption {
        QString id;
        QString label;
    };

    QList<CoordinateFormatOption> coordinateFormatOptions();
    QString defaultCoordinateFormatId();
    QString normalizeCoordinateFormatId(const QString &formatId);
    QString coordinateFormatLabel(const QString &formatId);
    QString formatSingleCoordinate(double value, bool latitude, const QString &formatId);
    QString formatCoordinate(double latitude, double longitude, const QString &formatId);
    QString formatCoordinate(const QGeoCoordinate &coordinate, const QString &formatId);
    bool parseSingleCoordinate(const QString &text, bool latitude, const QString &formatId, double *value, QString *message = nullptr);

}

#endif // FAIRWINDSK_UI_GEOCOORDINATEUTILS_HPP
