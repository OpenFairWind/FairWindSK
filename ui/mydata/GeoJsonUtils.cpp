//
// Created by Codex on 21/03/26.
//

#include "GeoJsonUtils.hpp"

#include <cmath>

#include <QDateTime>
#include <QJsonArray>

#include "signalk/Client.hpp"

namespace {
    QString resourceId(const QString &fallbackId, const QJsonObject &resource) {
        const auto feature = resource["feature"].toObject();
        if (feature["id"].isString() && !feature["id"].toString().isEmpty()) {
            return feature["id"].toString();
        }
        if (resource["id"].isString() && !resource["id"].toString().isEmpty()) {
            return resource["id"].toString();
        }
        return fallbackId;
    }

    QJsonObject resourceProperties(const QJsonObject &resource) {
        return resource["feature"].toObject()["properties"].toObject();
    }

    QJsonObject resourceGeometry(const QJsonObject &resource) {
        return resource["feature"].toObject()["geometry"].toObject();
    }

    QJsonArray collectFeatures(const QJsonDocument &document) {
        QJsonArray features;
        if (document.isObject()) {
            const auto object = document.object();
            if (object["type"].toString() == "FeatureCollection" && object["features"].isArray()) {
                return object["features"].toArray();
            }
            if (object["type"].toString() == "Feature") {
                features.append(object);
                return features;
            }
        }
        if (document.isArray()) {
            for (const auto &item : document.array()) {
                if (item.isObject() && item.toObject()["type"].toString() == "Feature") {
                    features.append(item.toObject());
                }
            }
        }
        return features;
    }

    QJsonObject resourceToFeature(const fairwindsk::ui::mydata::ResourceKind kind,
                                  const QString &id,
                                  const QJsonObject &resource) {
        QJsonObject feature;
        feature["type"] = "Feature";
        feature["id"] = resourceId(id, resource);

        QJsonObject properties = resourceProperties(resource);
        if (resource["name"].isString()) {
            properties["name"] = resource["name"].toString();
        }
        if (resource["description"].isString()) {
            properties["description"] = resource["description"].toString();
        }
        if (resource["timestamp"].isString()) {
            properties["timestamp"] = resource["timestamp"].toString();
        }

        switch (kind) {
            case fairwindsk::ui::mydata::ResourceKind::Waypoint:
            case fairwindsk::ui::mydata::ResourceKind::Route:
            case fairwindsk::ui::mydata::ResourceKind::Region:
                if (!resource["type"].toString().isEmpty()) {
                    properties["resourceType"] = resource["type"].toString();
                }
                feature["geometry"] = resourceGeometry(resource);
                break;
            case fairwindsk::ui::mydata::ResourceKind::Note: {
                properties["title"] = resource["title"].toString();
                properties["href"] = resource["href"].toString();
                properties["mimeType"] = resource["mimeType"].toString();
                const auto extraProperties = resource["properties"].toObject();
                for (auto it = extraProperties.begin(); it != extraProperties.end(); ++it) {
                    properties[it.key()] = it.value();
                }
                const auto position = resource["position"].toObject();
                if (!position.isEmpty()) {
                    QJsonArray coordinates;
                    coordinates.append(position["longitude"].toDouble());
                    coordinates.append(position["latitude"].toDouble());
                    if (position.contains("altitude")) {
                        coordinates.append(position["altitude"].toDouble());
                    }
                    QJsonObject geometry;
                    geometry["type"] = "Point";
                    geometry["coordinates"] = coordinates;
                    feature["geometry"] = geometry;
                } else {
                    feature["geometry"] = QJsonValue::Null;
                }
                break;
            }
            case fairwindsk::ui::mydata::ResourceKind::Chart: {
                properties["identifier"] = resource["identifier"].toString();
                properties["chartFormat"] = resource["chartFormat"].toString();
                properties["chartUrl"] = resource["chartUrl"].toString();
                properties["tilemapUrl"] = resource["tilemapUrl"].toString();
                properties["region"] = resource["region"].toString();
                if (resource.contains("scale")) {
                    properties["scale"] = resource["scale"];
                }
                if (resource["chartLayers"].isArray()) {
                    properties["chartLayers"] = resource["chartLayers"].toArray();
                }
                if (resource["bounds"].isArray()) {
                    const auto bounds = resource["bounds"].toArray();
                    if (bounds.size() >= 2 && bounds.at(0).isArray() && bounds.at(1).isArray()) {
                        const auto southWest = bounds.at(0).toArray();
                        const auto northEast = bounds.at(1).toArray();
                        if (southWest.size() >= 2 && northEast.size() >= 2) {
                            QJsonArray ring;
                            ring.append(QJsonArray{southWest.at(0).toDouble(), southWest.at(1).toDouble()});
                            ring.append(QJsonArray{northEast.at(0).toDouble(), southWest.at(1).toDouble()});
                            ring.append(QJsonArray{northEast.at(0).toDouble(), northEast.at(1).toDouble()});
                            ring.append(QJsonArray{southWest.at(0).toDouble(), northEast.at(1).toDouble()});
                            ring.append(QJsonArray{southWest.at(0).toDouble(), southWest.at(1).toDouble()});
                            QJsonObject geometry;
                            geometry["type"] = "Polygon";
                            geometry["coordinates"] = QJsonArray{ring};
                            feature["geometry"] = geometry;
                        } else {
                            feature["geometry"] = QJsonValue::Null;
                        }
                    } else {
                        feature["geometry"] = QJsonValue::Null;
                    }
                } else {
                    feature["geometry"] = QJsonValue::Null;
                }
                break;
            }
        }

        feature["properties"] = properties;
        return feature;
    }

    QJsonObject featureToResource(const fairwindsk::ui::mydata::ResourceKind kind, const QJsonObject &feature) {
        const QString id = feature["id"].toString();
        const auto geometry = feature["geometry"].toObject();
        const auto properties = feature["properties"].toObject();

        QJsonObject resource;
        resource["timestamp"] = properties["timestamp"].toString(fairwindsk::signalk::Client::currentISO8601TimeUTC());

        switch (kind) {
            case fairwindsk::ui::mydata::ResourceKind::Waypoint: {
                QJsonObject cleanProperties = properties;
                cleanProperties.remove("timestamp");
                cleanProperties.remove("resourceType");

                QJsonObject featureObject;
                featureObject["id"] = id;
                featureObject["type"] = "Feature";
                featureObject["geometry"] = geometry;
                featureObject["properties"] = cleanProperties;

                resource["name"] = properties["name"].toString();
                resource["description"] = properties["description"].toString();
                resource["type"] = properties["resourceType"].toString("generic");
                resource["feature"] = featureObject;
                break;
            }
            case fairwindsk::ui::mydata::ResourceKind::Route: {
                QJsonObject cleanProperties = properties;
                cleanProperties.remove("timestamp");
                cleanProperties.remove("resourceType");

                QJsonObject featureObject;
                featureObject["id"] = id;
                featureObject["type"] = "Feature";
                featureObject["geometry"] = geometry;
                featureObject["properties"] = cleanProperties;

                resource["name"] = properties["name"].toString();
                resource["description"] = properties["description"].toString();
                resource["type"] = properties["resourceType"].toString("route");
                resource["feature"] = featureObject;
                break;
            }
            case fairwindsk::ui::mydata::ResourceKind::Region: {
                QJsonObject cleanProperties = properties;
                cleanProperties.remove("timestamp");

                QJsonObject featureObject;
                featureObject["id"] = id;
                featureObject["type"] = "Feature";
                featureObject["geometry"] = geometry;
                featureObject["properties"] = cleanProperties;

                resource["name"] = properties["name"].toString();
                resource["description"] = properties["description"].toString();
                resource["feature"] = featureObject;
                break;
            }
            case fairwindsk::ui::mydata::ResourceKind::Note: {
                resource["title"] = properties["title"].toString(properties["name"].toString());
                resource["description"] = properties["description"].toString();
                resource["href"] = properties["href"].toString();
                resource["mimeType"] = properties["mimeType"].toString("text/plain");

                QJsonObject extraProperties = properties;
                extraProperties.remove("timestamp");
                extraProperties.remove("title");
                extraProperties.remove("name");
                extraProperties.remove("description");
                extraProperties.remove("href");
                extraProperties.remove("mimeType");
                if (!extraProperties.isEmpty()) {
                    resource["properties"] = extraProperties;
                }

                if (geometry["type"].toString() == "Point") {
                    const auto coordinates = geometry["coordinates"].toArray();
                    QJsonObject position;
                    if (coordinates.size() > 1) {
                        position["latitude"] = coordinates.at(1).toDouble();
                        position["longitude"] = coordinates.at(0).toDouble();
                        if (coordinates.size() > 2) {
                            position["altitude"] = coordinates.at(2).toDouble();
                        }
                        resource["position"] = position;
                    }
                }
                break;
            }
            case fairwindsk::ui::mydata::ResourceKind::Chart: {
                resource["name"] = properties["name"].toString();
                resource["description"] = properties["description"].toString();
                resource["identifier"] = properties["identifier"].toString();
                resource["chartFormat"] = properties["chartFormat"].toString();
                resource["chartUrl"] = properties["chartUrl"].toString();
                resource["tilemapUrl"] = properties["tilemapUrl"].toString();
                resource["region"] = properties["region"].toString();
                if (properties.contains("scale")) {
                    resource["scale"] = properties["scale"];
                }
                if (properties["chartLayers"].isArray()) {
                    resource["chartLayers"] = properties["chartLayers"].toArray();
                }
                if (geometry["type"].toString() == "Polygon") {
                    const auto polygons = geometry["coordinates"].toArray();
                    if (!polygons.isEmpty()) {
                        const auto ring = polygons.at(0).toArray();
                        if (ring.size() >= 4) {
                            const auto southWest = ring.at(0).toArray();
                            const auto northEast = ring.at(2).toArray();
                            if (southWest.size() >= 2 && northEast.size() >= 2) {
                                QJsonArray bounds;
                                bounds.append(QJsonArray{southWest.at(0).toDouble(), southWest.at(1).toDouble()});
                                bounds.append(QJsonArray{northEast.at(0).toDouble(), northEast.at(1).toDouble()});
                                resource["bounds"] = bounds;
                            }
                        }
                    }
                }
                break;
            }
        }

        return resource;
    }
}

namespace fairwindsk::ui::mydata {
    QJsonDocument exportResourcesAsGeoJson(const ResourceKind kind, const QList<QPair<QString, QJsonObject>> &resources) {
        QJsonArray features;
        for (const auto &entry : resources) {
            features.append(resourceToFeature(kind, entry.first, entry.second));
        }

        QJsonObject collection;
        collection["type"] = "FeatureCollection";
        collection["features"] = features;
        return QJsonDocument(collection);
    }

    bool importResourcesFromGeoJson(const ResourceKind kind,
                                    const QJsonDocument &document,
                                    QList<QPair<QString, QJsonObject>> *resources,
                                    QString *message) {
        const auto features = collectFeatures(document);
        if (features.isEmpty()) {
            if (message) {
                *message = QObject::tr("The selected file does not contain GeoJSON features.");
            }
            return false;
        }

        resources->clear();
        for (const auto &value : features) {
            const auto feature = value.toObject();
            if (feature["type"].toString() != "Feature") {
                continue;
            }
            const QString id = feature["id"].toString();
            resources->append({id, featureToResource(kind, feature)});
        }

        if (resources->isEmpty()) {
            if (message) {
                *message = QObject::tr("No compatible GeoJSON features were found.");
            }
            return false;
        }

        if (message) {
            *message = QObject::tr("Imported %1 GeoJSON feature(s).").arg(resources->size());
        }
        return true;
    }

    QJsonDocument exportTrackPointsAsGeoJson(const QList<HistoryTrackPoint> &points) {
        QJsonArray coordinates;
        QJsonArray samples;
        for (const auto &point : points) {
            QJsonArray coordinate;
            coordinate.append(point.coordinate.longitude());
            coordinate.append(point.coordinate.latitude());
            if (!std::isnan(point.coordinate.altitude())) {
                coordinate.append(point.coordinate.altitude());
            }
            coordinates.append(coordinate);

            QJsonObject sample;
            sample["timestamp"] = point.timestamp.toUTC().toString(Qt::ISODateWithMs);
            sample["coordinate"] = coordinate;
            samples.append(sample);
        }

        QJsonObject geometry;
        geometry["type"] = "LineString";
        geometry["coordinates"] = coordinates;

        QJsonObject properties;
        properties["samples"] = samples;

        QJsonObject feature;
        feature["type"] = "Feature";
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        QJsonObject collection;
        collection["type"] = "FeatureCollection";
        collection["features"] = QJsonArray{feature};
        return QJsonDocument(collection);
    }

    bool importTrackPointsFromGeoJson(const QJsonDocument &document,
                                      QList<HistoryTrackPoint> *points,
                                      QString *message) {
        const auto features = collectFeatures(document);
        if (features.isEmpty()) {
            if (message) {
                *message = QObject::tr("The selected file does not contain GeoJSON features.");
            }
            return false;
        }

        points->clear();
        for (const auto &value : features) {
            const auto feature = value.toObject();
            const auto geometry = feature["geometry"].toObject();
            if (geometry["type"].toString() == "LineString") {
                const auto coordinates = geometry["coordinates"].toArray();
                const auto samples = feature["properties"].toObject()["samples"].toArray();
                for (int i = 0; i < coordinates.size(); ++i) {
                    const auto coordinateArray = coordinates.at(i).toArray();
                    if (coordinateArray.size() < 2) {
                        continue;
                    }

                    HistoryTrackPoint point;
                    point.coordinate.setLongitude(coordinateArray.at(0).toDouble());
                    point.coordinate.setLatitude(coordinateArray.at(1).toDouble());
                    if (coordinateArray.size() > 2) {
                        point.coordinate.setAltitude(coordinateArray.at(2).toDouble());
                    }

                    if (i < samples.size() && samples.at(i).isObject()) {
                        point.timestamp = QDateTime::fromString(samples.at(i).toObject()["timestamp"].toString(), Qt::ISODateWithMs);
                    }
                    if (!point.timestamp.isValid()) {
                        point.timestamp = QDateTime::currentDateTimeUtc();
                    }

                    if (point.coordinate.isValid()) {
                        points->append(point);
                    }
                }
            } else if (geometry["type"].toString() == "Point") {
                const auto coordinateArray = geometry["coordinates"].toArray();
                if (coordinateArray.size() < 2) {
                    continue;
                }

                HistoryTrackPoint point;
                point.coordinate.setLongitude(coordinateArray.at(0).toDouble());
                point.coordinate.setLatitude(coordinateArray.at(1).toDouble());
                if (coordinateArray.size() > 2) {
                    point.coordinate.setAltitude(coordinateArray.at(2).toDouble());
                }
                point.timestamp = QDateTime::currentDateTimeUtc();
                points->append(point);
            }
        }

        if (points->isEmpty()) {
            if (message) {
                *message = QObject::tr("No compatible GeoJSON track coordinates were found.");
            }
            return false;
        }

        if (message) {
            *message = QObject::tr("Imported %1 track sample(s) from GeoJSON.").arg(points->size());
        }
        return true;
    }
}
