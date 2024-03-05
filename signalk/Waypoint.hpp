//
// Created by Raffaele Montella on 01/03/22.
//

#ifndef FAIRWINDSK_SIGNALK_WAYPOINT_HPP
#define FAIRWINDSK_SIGNALK_WAYPOINT_HPP

#include <QObject>
#include <QGeoCoordinate>
#include "../FairWindSK.hpp"

namespace fairwindsk::signalk {

    class Waypoint : public QJsonObject {
    public:
        Waypoint(const QString &id, const QString &name, const QString &description, const QString &type,
                 const QGeoCoordinate &coordinate);

        QString id();

        QString name();

        QString description();

        QString type();

        QGeoCoordinate coordinate();

    private:

    };
}

#endif //FAIRWIND_SDK_SIGNALK_WAYPOINT_HPP