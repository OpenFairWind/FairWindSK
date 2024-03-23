//
// Created by Raffaele Montella on 01/03/22.
//

#include <QJsonDocument>
#include <QJsonObject>

#include "Waypoint.hpp"

/*
 * {
 *  "name":"Point1",
 *  "description":"",
 *  "feature":{
 *      "type":"Feature",
 *      "geometry":{
 *          "type":"Point",
 *          "coordinates":[23.328516077302734,59.91763457421976]
 *       },
 *       "properties":{},
 *       "id":""
 *  },
 *  "type":"",
 *  "timestamp":"2024-03-19T17:03:57.973Z",
 *  "$source":"resources-provider"
 * }
 */

namespace fairwindsk::signalk {
    Waypoint::Waypoint(const QString &id, const QString &name, const QString &description, const QString &type,
                       const QGeoCoordinate &coordinate) {
        QJsonObject pos;
        pos["latitude"] = coordinate.latitude();
        pos["longitude"] = coordinate.longitude();
        pos["altitude"] = coordinate.altitude();

        QString featureString;
        featureString = QString(
                R"({"id": "%1","type": "Feature","properties": { "name": "%2", "description": "%3", "type":"%4"}, "geometry": { "type": "Point", "coordinates": [ %5, %6, %7 ] }})")
                .arg(id)
                .arg(name)
                .arg(description)
                .arg(type)
                .arg(coordinate.longitude()).arg(coordinate.latitude()).arg(coordinate.altitude());


        qDebug() << "Waypoint::Waypoint :" << featureString;
        this->operator[]("feature") = QJsonDocument::fromJson(featureString.toLatin1()).object();
        this->operator[]("position") = pos;
    }

    QString Waypoint::getId() {
        return this->operator[]("feature").toObject()["id"].toString();
    };

    QString Waypoint::getName()  {
        return this->operator[]("name").toString();
    };

    QString Waypoint::getDescription() {
        return this->operator[]("description").toString();
    };

    QString Waypoint::getType() {
        return this->operator[]("type").toString();
    };

    QGeoCoordinate Waypoint::getCoordinates() {
        auto pos = this->operator[]("position").toObject();
        return QGeoCoordinate(pos["latitude"].toDouble(), pos["longitude"].toDouble(), pos["altitude"].toDouble());
    }

    Waypoint::Waypoint(const QJsonObject &jsonObject) : QJsonObject(jsonObject) {
    };
}