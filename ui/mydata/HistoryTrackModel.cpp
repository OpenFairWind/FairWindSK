//
// Created by Codex on 21/03/26.
//

#include "HistoryTrackModel.hpp"

#include <cmath>

#include <QJsonArray>
#include <QLocale>

#include "FairWindSK.hpp"

namespace {
    QDateTime parseTimestamp(const QJsonValue &value) {
        if (!value.isString()) {
            return {};
        }

        QDateTime dateTime = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(value.toString(), Qt::ISODate);
        }
        return dateTime;
    }

    QGeoCoordinate parseCoordinateValue(const QJsonValue &value) {
        QGeoCoordinate coordinate;
        if (!value.isObject()) {
            return coordinate;
        }

        const auto object = value.toObject();
        if (object.contains("latitude")) {
            coordinate.setLatitude(object["latitude"].toDouble());
        }
        if (object.contains("longitude")) {
            coordinate.setLongitude(object["longitude"].toDouble());
        }
        if (object.contains("altitude")) {
            coordinate.setAltitude(object["altitude"].toDouble());
        }
        return coordinate;
    }

    void appendPointsFromDataArray(const QJsonArray &data, QList<fairwindsk::ui::mydata::HistoryTrackPoint> *points) {
        for (const auto &sample : data) {
            if (!sample.isArray()) {
                continue;
            }

            const auto row = sample.toArray();
            if (row.size() < 2) {
                continue;
            }

            const auto timestamp = parseTimestamp(row.at(0));
            const auto coordinate = parseCoordinateValue(row.at(1));
            if (timestamp.isValid() && coordinate.isValid()) {
                points->append({timestamp, coordinate});
            }
        }
    }
}

namespace fairwindsk::ui::mydata {
    HistoryTrackModel::HistoryTrackModel(QObject *parent)
        : QAbstractTableModel(parent) {
    }

    int HistoryTrackModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_points.size();
    }

    int HistoryTrackModel::columnCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : 4;
    }

    QVariant HistoryTrackModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_points.size()) {
            return {};
        }

        const auto &point = m_points.at(index.row());
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
                case 0:
                    return QLocale().toString(point.timestamp.toLocalTime(), QLocale::ShortFormat);
                case 1:
                    return QString::number(point.coordinate.latitude(), 'f', 6);
                case 2:
                    return QString::number(point.coordinate.longitude(), 'f', 6);
                case 3:
                    return std::isnan(point.coordinate.altitude()) ? QString() : QString::number(point.coordinate.altitude(), 'f', 1);
                default:
                    return {};
            }
        }

        if (role == Qt::TextAlignmentRole && index.column() > 0) {
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        }

        return {};
    }

    QVariant HistoryTrackModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }

        switch (section) {
            case 0: return tr("Time");
            case 1: return tr("Latitude");
            case 2: return tr("Longitude");
            case 3: return tr("Altitude");
            default: return {};
        }
    }

    bool HistoryTrackModel::reload(const QString &duration, const QString &resolution, QString *message) {
        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        const auto path = configuration->getRoot()["signalk"]["pos"].is_string()
                ? QString::fromStdString(configuration->getRoot()["signalk"]["pos"].get<std::string>())
                : QStringLiteral("navigation.position");

        QVariantMap query;
        query["duration"] = duration;
        query["resolution"] = resolution;

        const auto payload = fairwindsk::FairWindSK::getInstance()->getSignalKClient()->getHistoryValues({path}, query);
        return applyHistoryPayload(QJsonDocument(payload), message);
    }

    bool HistoryTrackModel::importDocument(const QJsonDocument &document, QString *message) {
        return applyHistoryPayload(document, message);
    }

    QJsonDocument HistoryTrackModel::exportDocument() const {
        if (!m_sourceDocument.isNull()) {
            return m_sourceDocument;
        }

        QJsonArray array;
        for (const auto &point : m_points) {
            QJsonObject item;
            item["timestamp"] = point.timestamp.toUTC().toString(Qt::ISODateWithMs);
            QJsonObject coordinate;
            coordinate["latitude"] = point.coordinate.latitude();
            coordinate["longitude"] = point.coordinate.longitude();
            if (!std::isnan(point.coordinate.altitude())) {
                coordinate["altitude"] = point.coordinate.altitude();
            }
            item["position"] = coordinate;
            array.append(item);
        }
        return QJsonDocument(array);
    }

    bool HistoryTrackModel::hasPoints() const {
        return !m_points.isEmpty();
    }

    bool HistoryTrackModel::applyHistoryPayload(const QJsonDocument &document, QString *message) {
        QList<HistoryTrackPoint> points;

        if (document.isObject()) {
            const auto object = document.object();
            if (object.contains("data") && object["data"].isArray()) {
                appendPointsFromDataArray(object["data"].toArray(), &points);
            }
            if (object.contains("result") && object["result"].isObject()) {
                appendPointsFromDataArray(object["result"].toObject()["data"].toArray(), &points);
            }
            if (object.contains("results") && object["results"].isArray()) {
                for (const auto &result : object["results"].toArray()) {
                    if (result.isObject()) {
                        appendPointsFromDataArray(result.toObject()["data"].toArray(), &points);
                    }
                }
            }
        } else if (document.isArray()) {
            for (const auto &item : document.array()) {
                if (item.isObject()) {
                    const auto object = item.toObject();
                    const auto timestamp = parseTimestamp(object["timestamp"]);
                    const auto coordinate = parseCoordinateValue(object["position"]);
                    if (timestamp.isValid() && coordinate.isValid()) {
                        points.append({timestamp, coordinate});
                    }
                }
            }
        }

        beginResetModel();
        m_points = points;
        m_sourceDocument = document;
        endResetModel();

        if (message) {
            *message = points.isEmpty() ? tr("No track samples are available from the History API.") :
                                          tr("Loaded %1 track samples.").arg(points.size());
        }

        return !points.isEmpty();
    }
}
