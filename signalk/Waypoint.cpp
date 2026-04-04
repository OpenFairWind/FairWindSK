//
// Created by Raffaele Montella on 01/03/22.
//

#include <cmath>

#include <QDateTime>
#include <QJsonObject>

#include "Waypoint.hpp"

#include <QJsonArray>

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
    namespace {
        QString featurePropertyString(const QJsonObject &waypoint, const QString &key) {
            if (!waypoint.contains("feature") || !waypoint.value("feature").isObject()) {
                return {};
            }

            const QJsonObject feature = waypoint.value("feature").toObject();
            if (!feature.contains("properties") || !feature.value("properties").isObject()) {
                return {};
            }

            return feature.value("properties").toObject().value(key).toString();
        }

        void setFeatureProperty(QJsonObject *waypoint, const QString &key, const QString &value) {
            if (!waypoint) {
                return;
            }

            QJsonObject feature = waypoint->value("feature").toObject();
            QJsonObject properties = feature.value("properties").toObject();
            properties.insert(key, value);
            feature.insert("properties", properties);
            waypoint->insert("feature", feature);
        }
    }

    Waypoint::Waypoint(const QString &id, const QString &name, const QString &description, const QString &type,
                       const QGeoCoordinate &coordinate) {
        QJsonObject pos;
        pos["latitude"] = coordinate.latitude();
        pos["longitude"] = coordinate.longitude();
        if (!std::isnan(coordinate.altitude())) {
            pos["altitude"] = coordinate.altitude();
        }

        QJsonObject properties;
        properties["name"] = name;
        properties["description"] = description;
        properties["type"] = type;

        QJsonArray coordinates;
        coordinates.append(coordinate.longitude());
        coordinates.append(coordinate.latitude());
        if (!std::isnan(coordinate.altitude())) {
            coordinates.append(coordinate.altitude());
        }

        QJsonObject geometry;
        geometry["type"] = QStringLiteral("Point");
        geometry["coordinates"] = coordinates;

        QJsonObject feature;
        feature["id"] = id;
        feature["type"] = QStringLiteral("Feature");
        feature["properties"] = properties;
        feature["geometry"] = geometry;

        this->operator[]("name") = name;
        this->operator[]("description") = description;
        this->operator[]("type") = type;
        this->operator[]("timestamp") = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        this->operator[]("feature") = feature;
        this->operator[]("position") = pos;
    }

    QString Waypoint::getId() {
        return this->operator[]("feature").toObject()["id"].toString();
    };

    QString Waypoint::getName()  {
        const QString name = this->operator[]("name").toString();
        return name.isEmpty() ? featurePropertyString(*this, QStringLiteral("name")) : name;
    };

    QString Waypoint::getDescription() {
        const QString description = this->operator[]("description").toString();
        return description.isEmpty() ? featurePropertyString(*this, QStringLiteral("description")) : description;
    };

    QString Waypoint::getType() {
        const QString type = this->operator[]("type").toString();
        return type.isEmpty() ? featurePropertyString(*this, QStringLiteral("type")) : type;
    };

    QDateTime Waypoint::getTimestamp() {
        return QDateTime::fromString(this->operator[]("timestamp").toString(), Qt::ISODate);
    }

    QGeoCoordinate Waypoint::getCoordinates() {

        QGeoCoordinate result;
        if (contains("feature") && this->operator[]("feature").isObject()) {
            const auto geoJson = this->operator[]("feature").toObject();
            if (geoJson.contains("geometry") && geoJson["geometry"].isObject()) {
                const auto geometryJson = geoJson["geometry"].toObject();
                if (geometryJson.contains("coordinates") && geometryJson["coordinates"].isArray()) {
                    const auto coordinatesJsonArray = geometryJson["coordinates"].toArray();
                    if (coordinatesJsonArray.size() > 0 && coordinatesJsonArray.at(0).isDouble()) {
                        result.setLongitude(coordinatesJsonArray.at(0).toDouble());
                    }
                    if (coordinatesJsonArray.size() > 1 && coordinatesJsonArray.at(1).isDouble()) {
                        result.setLatitude(coordinatesJsonArray.at(1).toDouble());
                    }
                    if (coordinatesJsonArray.size() > 2 && coordinatesJsonArray.at(2).isDouble()) {
                        result.setAltitude(coordinatesJsonArray.at(2).toDouble());
                    }
                }
            }
        }
        return result;
    }

    void Waypoint::setName(const QString& name) {
        this->operator[]("name") = name;
        setFeatureProperty(this, QStringLiteral("name"), name);

    }
    void Waypoint::setDescription(const QString& description) {
        this->operator[]("description") = description;
        setFeatureProperty(this, QStringLiteral("description"), description);
    }

    Waypoint::Waypoint(const QJsonObject &jsonObject) : QJsonObject(jsonObject) {
    };
}
