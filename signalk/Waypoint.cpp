//
// Created by Raffaele Montella on 01/03/22.
//

#include <QJsonDocument>
#include <QJsonObject>

#include "Waypoint.hpp"



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

    QString Waypoint::id() {
        return this->operator[]("feature").toObject()["id"].toString();
    };

    QString Waypoint::name() {
        return this->operator[]("feature").toObject()["name"].toString();
    };

    QString Waypoint::description() {
        return this->operator[]("feature").toObject()["description"].toString();
    };

    QString Waypoint::type() {
        return this->operator[]("feature").toObject()["type"].toString();
    };

    QGeoCoordinate Waypoint::coordinate() {
        auto pos = this->operator[]("position").toObject();
        return QGeoCoordinate(pos["latitude"].toDouble(), pos["longitude"].toDouble(), pos["altitude"].toDouble());
    };
}