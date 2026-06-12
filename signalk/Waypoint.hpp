//
// Created by Raffaele Montella on 01/03/22.
//

#ifndef FAIRWINDSK_SIGNALK_WAYPOINT_HPP
#define FAIRWINDSK_SIGNALK_WAYPOINT_HPP

#include <QJsonObject>
#include <QGeoCoordinate>


namespace fairwindsk::signalk {

    class Waypoint : public QJsonObject {
    public:
        Waypoint() = default;

        Waypoint(const QString &id, const QString &name, const QString &description, const QString &type,
                 const QGeoCoordinate &coordinate);

        explicit Waypoint(const QJsonObject &jsonObject);

        QString getId();

        QString getName();

        QString getDescription();

        QString getType();

        QDateTime getTimestamp();

        QGeoCoordinate getCoordinates();

        void setName(const QString &name);
        void setDescription(const QString &description);

    };
}

#endif //FAIRWIND_SDK_SIGNALK_WAYPOINT_HPP
