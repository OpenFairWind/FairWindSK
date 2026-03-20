//
// Created by Codex on 21/03/26.
//

#include "ResourceModel.hpp"

#include <algorithm>

#include <QDateTime>
#include <QJsonArray>

#include "FairWindSK.hpp"

namespace {
    QString resourceName(const QJsonObject &resource) {
        if (resource.contains("name") && resource["name"].isString()) {
            return resource["name"].toString();
        }

        return resource["feature"].toObject()["properties"].toObject()["name"].toString();
    }

    QString resourceDescription(const QJsonObject &resource) {
        if (resource.contains("description") && resource["description"].isString()) {
            return resource["description"].toString();
        }

        return resource["feature"].toObject()["properties"].toObject()["description"].toString();
    }

    QString resourceType(const QJsonObject &resource) {
        if (resource.contains("type") && resource["type"].isString()) {
            return resource["type"].toString();
        }

        const QJsonObject properties = resource["feature"].toObject()["properties"].toObject();
        if (properties.contains("type") && properties["type"].isString()) {
            return properties["type"].toString();
        }

        return resource["feature"].toObject()["geometry"].toObject()["type"].toString();
    }

    QJsonArray resourceCoordinates(const QJsonObject &resource) {
        return resource["feature"].toObject()["geometry"].toObject()["coordinates"].toArray();
    }

    int pointCount(const fairwindsk::ui::mydata::ResourceKind kind, const QJsonObject &resource) {
        const QJsonArray coordinates = resourceCoordinates(resource);
        return kind == fairwindsk::ui::mydata::ResourceKind::Waypoint ? (coordinates.isEmpty() ? 0 : 1) : coordinates.size();
    }

    QString timestampText(const QJsonObject &resource) {
        const QString timestamp = resource["timestamp"].toString();
        const QDateTime dateTime = QDateTime::fromString(timestamp, Qt::ISODate);
        return dateTime.isValid() ? dateTime.toLocalTime().toString(Qt::DefaultLocaleShortDate) : timestamp;
    }
}

namespace fairwindsk::ui::mydata {
    QString resourceKindToCollection(const ResourceKind kind) {
        switch (kind) {
            case ResourceKind::Waypoint:
                return "waypoints";
            case ResourceKind::Route:
                return "routes";
            case ResourceKind::Track:
                return "tracks";
        }

        return {};
    }

    QString resourceKindToTitle(const ResourceKind kind) {
        switch (kind) {
            case ResourceKind::Waypoint:
                return QObject::tr("Waypoints");
            case ResourceKind::Route:
                return QObject::tr("Routes");
            case ResourceKind::Track:
                return QObject::tr("Tracks");
        }

        return {};
    }

    ResourceModel::ResourceModel(const ResourceKind kind, QObject *parent)
        : QAbstractTableModel(parent), m_kind(kind) {
        reload();
    }

    int ResourceModel::rowCount(const QModelIndex &parent) const {
        if (parent.isValid()) {
            return 0;
        }

        return m_ids.size();
    }

    int ResourceModel::columnCount(const QModelIndex &parent) const {
        if (parent.isValid()) {
            return 0;
        }

        return m_kind == ResourceKind::Waypoint ? 6 : 5;
    }

    QVariant ResourceModel::data(const QModelIndex &index, const int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size()) {
            return {};
        }

        const QJsonObject resource = m_resources.value(m_ids.at(index.row()));

        if (role == Qt::DisplayRole) {
            if (m_kind == ResourceKind::Waypoint) {
                const QJsonArray coordinates = resourceCoordinates(resource);
                switch (index.column()) {
                    case 0:
                        return resourceName(resource);
                    case 1:
                        return resourceDescription(resource);
                    case 2:
                        return resourceType(resource);
                    case 3:
                        return coordinates.size() > 1 ? QString::number(coordinates.at(1).toDouble(), 'f', 6) : QString();
                    case 4:
                        return coordinates.size() > 0 ? QString::number(coordinates.at(0).toDouble(), 'f', 6) : QString();
                    case 5:
                        return timestampText(resource);
                    default:
                        return {};
                }
            }

            switch (index.column()) {
                case 0:
                    return resourceName(resource);
                case 1:
                    return resourceDescription(resource);
                case 2:
                    return resourceType(resource);
                case 3:
                    return pointCount(m_kind, resource);
                case 4:
                    return timestampText(resource);
                default:
                    return {};
            }
        }

        if (role == Qt::TextAlignmentRole) {
            if (m_kind == ResourceKind::Waypoint && (index.column() == 3 || index.column() == 4)) {
                return Qt::AlignRight | Qt::AlignVCenter;
            }

            if (m_kind != ResourceKind::Waypoint && index.column() == 3) {
                return Qt::AlignCenter;
            }
        }

        return {};
    }

    QVariant ResourceModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }

        if (m_kind == ResourceKind::Waypoint) {
            switch (section) {
                case 0:
                    return tr("Name");
                case 1:
                    return tr("Description");
                case 2:
                    return tr("Type");
                case 3:
                    return tr("Latitude");
                case 4:
                    return tr("Longitude");
                case 5:
                    return tr("Timestamp");
                default:
                    return {};
            }
        }

        switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Description");
            case 2:
                return tr("Geometry");
            case 3:
                return tr("Points");
            case 4:
                return tr("Timestamp");
            default:
                return {};
        }
    }

    void ResourceModel::reload() {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        const auto resources = client->getResources(resourceKindToCollection(m_kind));
        QList<QString> ids = resources.keys();

        std::sort(ids.begin(), ids.end(), [&resources](const QString &left, const QString &right) {
            return resourceName(resources.value(left)).toLower() < resourceName(resources.value(right)).toLower();
        });

        beginResetModel();
        m_resources = resources;
        m_ids = ids;
        endResetModel();
    }

    bool ResourceModel::saveResource(const QString &id, const QJsonObject &resource) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        client->putResource(resourceKindToCollection(m_kind), id, resource);
        reload();
        return m_resources.contains(id);
    }

    bool ResourceModel::deleteResource(const QString &id) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        client->deleteResource(resourceKindToCollection(m_kind), id);
        reload();
        return !m_resources.contains(id);
    }

    QString ResourceModel::resourceIdAtRow(const int row) const {
        if (row < 0 || row >= m_ids.size()) {
            return {};
        }

        return m_ids.at(row);
    }

    QJsonObject ResourceModel::resourceAtRow(const int row) const {
        const QString id = resourceIdAtRow(row);
        return id.isEmpty() ? QJsonObject{} : m_resources.value(id);
    }

    ResourceKind ResourceModel::kind() const {
        return m_kind;
    }
}
