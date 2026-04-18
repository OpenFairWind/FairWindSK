//
// Created by Codex on 21/03/26.
//

#include "ResourceModel.hpp"

#include <algorithm>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QLocale>
#include <QPointer>
#include <QTimer>
#include <QUuid>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui/GeoCoordinateUtils.hpp"

namespace {
    QString displayNameForResource(const QJsonObject &resource) {
        static const QStringList directKeys = {"name", "title", "identifier", "href"};
        for (const auto &key : directKeys) {
            if (resource.contains(key) && resource[key].isString()) {
                return resource[key].toString();
            }
        }

        const QJsonObject featureProperties = resource["feature"].toObject()["properties"].toObject();
        for (const auto &key : directKeys) {
            if (featureProperties.contains(key) && featureProperties[key].isString()) {
                return featureProperties[key].toString();
            }
        }

        return {};
    }

    QString descriptionForResource(const QJsonObject &resource) {
        if (resource.contains("description") && resource["description"].isString()) {
            return resource["description"].toString();
        }

        return resource["feature"].toObject()["properties"].toObject()["description"].toString();
    }

    QString idForResource(const QString &fallbackId, const QJsonObject &resource) {
        const QJsonObject feature = resource["feature"].toObject();
        if (feature.contains("id") && feature["id"].isString() && !feature["id"].toString().isEmpty()) {
            return feature["id"].toString();
        }

        if (resource.contains("id") && resource["id"].isString() && !resource["id"].toString().isEmpty()) {
            return resource["id"].toString();
        }

        return fallbackId;
    }

    QString geometryTypeForResource(const QJsonObject &resource) {
        const QJsonObject geometry = resource["feature"].toObject()["geometry"].toObject();
        if (geometry.contains("type") && geometry["type"].isString()) {
            return geometry["type"].toString();
        }

        return resource["type"].toString();
    }

    QJsonArray coordinatesForResource(const QJsonObject &resource) {
        return resource["feature"].toObject()["geometry"].toObject()["coordinates"].toArray();
    }

    int pointCountForGeometry(const QString &geometryType, const QJsonArray &coordinates) {
        if (geometryType == "Point") {
            return coordinates.isEmpty() ? 0 : 1;
        }

        if (geometryType == "LineString" || geometryType == "MultiPoint") {
            return coordinates.size();
        }

        if (geometryType == "Polygon" || geometryType == "MultiLineString") {
            int count = 0;
            for (const auto &ring : coordinates) {
                count += ring.toArray().size();
            }
            return count;
        }

        if (geometryType == "MultiPolygon") {
            int count = 0;
            for (const auto &polygon : coordinates) {
                for (const auto &ring : polygon.toArray()) {
                    count += ring.toArray().size();
                }
            }
            return count;
        }

        return coordinates.size();
    }

    QString timestampForDisplay(const QJsonObject &resource) {
        const QString timestamp = resource["timestamp"].toString();
        if (timestamp.isEmpty()) {
            return {};
        }

        QDateTime dateTime = QDateTime::fromString(timestamp, Qt::ISODateWithMs);
        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(timestamp, Qt::ISODate);
        }
        return dateTime.isValid() ? QLocale().toString(dateTime.toLocalTime(), QLocale::ShortFormat) : timestamp;
    }

    QString noteHref(const QJsonObject &resource) {
        return resource["href"].toString();
    }

    QString chartIdentifier(const QJsonObject &resource) {
        return resource["identifier"].toString();
    }

    QString chartFormat(const QJsonObject &resource) {
        return resource["chartFormat"].toString();
    }

    QString coordinateLatitudeText(const QJsonObject &resource) {
        const QJsonArray coordinates = coordinatesForResource(resource);
        if (coordinates.size() <= 1) {
            return {};
        }

        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        return fairwindsk::ui::geo::formatSingleCoordinate(
            coordinates.at(1).toDouble(),
            true,
            configuration->getCoordinateFormat());
    }

    QString coordinateLongitudeText(const QJsonObject &resource) {
        const QJsonArray coordinates = coordinatesForResource(resource);
        if (coordinates.isEmpty()) {
            return {};
        }

        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        return fairwindsk::ui::geo::formatSingleCoordinate(
            coordinates.at(0).toDouble(),
            false,
            configuration->getCoordinateFormat());
    }

    QString upsertedResourceId(const QJsonObject &response, const QJsonObject &resource, const QString &fallbackId = {}) {
        if (response.contains("id") && response["id"].isString()) {
            return response["id"].toString();
        }
        if (response.contains("identifier") && response["identifier"].isString()) {
            return response["identifier"].toString();
        }
        if (response.contains("feature") && response["feature"].isObject()) {
            const auto feature = response["feature"].toObject();
            if (feature.contains("id") && feature["id"].isString()) {
                return feature["id"].toString();
            }
        }
        return idForResource(fallbackId, resource);
    }

    QList<QPair<QString, QJsonObject>> importEntriesFromObject(const QJsonObject &object) {
        QList<QPair<QString, QJsonObject>> entries;

        if (object.contains("resources") && object["resources"].isArray()) {
            const auto resources = object["resources"].toArray();
            for (const auto &item : resources) {
                if (item.isObject()) {
                    entries.append({QString(), item.toObject()});
                }
            }
            return entries;
        }

        bool keyedResourceMap = !object.isEmpty();
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (!it.value().isObject()) {
                keyedResourceMap = false;
                break;
            }
        }
        if (keyedResourceMap && !object.isEmpty()) {
            for (auto it = object.begin(); it != object.end(); ++it) {
                entries.append({it.key(), it.value().toObject()});
            }
            return entries;
        }

        entries.append({QString(), object});
        return entries;
    }

    QStringList sortedResourceIds(const QMap<QString, QJsonObject> &resources) {
        QList<QString> ids = resources.keys();
        std::sort(ids.begin(), ids.end(), [&resources](const QString &left, const QString &right) {
            return displayNameForResource(resources.value(left)).toLower() < displayNameForResource(resources.value(right)).toLower();
        });
        return ids;
    }

    QJsonValue applyJsonValuePath(const QJsonValue &currentValue, const QStringList &segments, const int index, const QJsonValue &newValue) {
        if (index >= segments.size()) {
            return newValue;
        }

        QJsonObject object = currentValue.isObject() ? currentValue.toObject() : QJsonObject{};
        const QString key = segments.at(index);
        if (index == segments.size() - 1) {
            if (newValue.isNull() || newValue.isUndefined()) {
                object.remove(key);
            } else {
                object.insert(key, newValue);
            }
            return object;
        }

        object.insert(key, applyJsonValuePath(object.value(key), segments, index + 1, newValue));
        return object;
    }
}

namespace fairwindsk::ui::mydata {
    QString resourceKindToCollection(const ResourceKind kind) {
        switch (kind) {
            case ResourceKind::Waypoint:
                return "waypoints";
            case ResourceKind::Route:
                return "routes";
            case ResourceKind::Region:
                return "regions";
            case ResourceKind::Note:
                return "notes";
            case ResourceKind::Chart:
                return "charts";
        }

        return {};
    }

    QString resourceKindToTitle(const ResourceKind kind) {
        switch (kind) {
            case ResourceKind::Waypoint:
                return QObject::tr("Waypoints");
            case ResourceKind::Route:
                return QObject::tr("Routes");
            case ResourceKind::Region:
                return QObject::tr("Regions");
            case ResourceKind::Note:
                return QObject::tr("Notes");
            case ResourceKind::Chart:
                return QObject::tr("Charts");
        }

        return {};
    }

    QString resourceKindToSingularTitle(const ResourceKind kind) {
        switch (kind) {
            case ResourceKind::Waypoint:
                return QObject::tr("Waypoint");
            case ResourceKind::Route:
                return QObject::tr("Route");
            case ResourceKind::Region:
                return QObject::tr("Region");
            case ResourceKind::Note:
                return QObject::tr("Note");
            case ResourceKind::Chart:
                return QObject::tr("Chart");
        }

        return {};
    }

    ResourceModel::ResourceModel(const ResourceKind kind, QObject *parent)
        : QAbstractTableModel(parent),
          m_kind(kind) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        m_subscriptionPath = collection() + ".*";
        client->subscribeStream(
            "resources",
            m_subscriptionPath,
            this,
            SLOT(onResourceUpdate(QJsonObject)));
        connect(client, &fairwindsk::signalk::Client::serverStateResynchronized, this, [this](const bool recoveredFromDisconnect) {
            if (recoveredFromDisconnect) {
                reload(true);
            }
        });
        QTimer::singleShot(0, this, [this]() {
            reload(true);
        });
    }

    ResourceModel::~ResourceModel() {
        fairwindsk::FairWindSK::getInstance()->getSignalKClient()->removeSubscription(m_subscriptionPath, this);
        m_reloadInProgress = false;
        m_reloadPending = false;
    }

    int ResourceModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_ids.size();
    }

    int ResourceModel::columnCount(const QModelIndex &parent) const {
        if (parent.isValid()) {
            return 0;
        }

        switch (m_kind) {
            case ResourceKind::Waypoint:
                return 6;
            case ResourceKind::Route:
                return 5;
            case ResourceKind::Region:
                return 4;
            case ResourceKind::Note:
                return 5;
            case ResourceKind::Chart:
                return 6;
        }

        return 0;
    }

    QVariant ResourceModel::data(const QModelIndex &index, const int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size()) {
            return {};
        }

        const QJsonObject resource = m_resources.value(m_ids.at(index.row()));
        const QString geometryType = geometryTypeForResource(resource);
        const QJsonArray coordinates = coordinatesForResource(resource);

        if (role == Qt::DisplayRole) {
            switch (m_kind) {
                case ResourceKind::Waypoint:
                    switch (index.column()) {
                        case 0: return displayNameForResource(resource);
                        case 1: return descriptionForResource(resource);
                        case 2: return resource["type"].toString();
                        case 3: return coordinateLatitudeText(resource);
                        case 4: return coordinateLongitudeText(resource);
                        case 5: return timestampForDisplay(resource);
                        default: return {};
                    }
                case ResourceKind::Route:
                    switch (index.column()) {
                        case 0: return displayNameForResource(resource);
                        case 1: return descriptionForResource(resource);
                        case 2: return geometryType;
                        case 3: return pointCountForGeometry(geometryType, coordinates);
                        case 4: return timestampForDisplay(resource);
                        default: return {};
                    }
                case ResourceKind::Region:
                    switch (index.column()) {
                        case 0: return displayNameForResource(resource);
                        case 1: return descriptionForResource(resource);
                        case 2: return geometryType;
                        case 3: return timestampForDisplay(resource);
                        default: return {};
                    }
                case ResourceKind::Note:
                    switch (index.column()) {
                        case 0: return displayNameForResource(resource);
                        case 1: return descriptionForResource(resource);
                        case 2: return noteHref(resource);
                        case 3: return resource["mimeType"].toString();
                        case 4: return timestampForDisplay(resource);
                        default: return {};
                    }
                case ResourceKind::Chart:
                    switch (index.column()) {
                        case 0: return displayNameForResource(resource);
                        case 1: return descriptionForResource(resource);
                        case 2: return chartFormat(resource);
                        case 3: return chartIdentifier(resource);
                        case 4: return resource["chartUrl"].toString();
                        case 5: return timestampForDisplay(resource);
                        default: return {};
                    }
            }
        }

        if (role == Qt::TextAlignmentRole) {
            if (m_kind == ResourceKind::Waypoint && (index.column() == 3 || index.column() == 4)) {
                return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
            }
            if (m_kind == ResourceKind::Route && index.column() == 3) {
                return QVariant::fromValue(Qt::AlignCenter);
            }
        }

        return {};
    }

    QVariant ResourceModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }

        switch (m_kind) {
            case ResourceKind::Waypoint:
                switch (section) {
                    case 0: return tr("Name");
                    case 1: return tr("Description");
                    case 2: return tr("Type");
                    case 3: return tr("Latitude");
                    case 4: return tr("Longitude");
                    case 5: return tr("Timestamp");
                    default: return {};
                }
            case ResourceKind::Route:
                switch (section) {
                    case 0: return tr("Name");
                    case 1: return tr("Description");
                    case 2: return tr("Geometry");
                    case 3: return tr("Points");
                    case 4: return tr("Timestamp");
                    default: return {};
                }
            case ResourceKind::Region:
                switch (section) {
                    case 0: return tr("Name");
                    case 1: return tr("Description");
                    case 2: return tr("Geometry");
                    case 3: return tr("Timestamp");
                    default: return {};
                }
            case ResourceKind::Note:
                switch (section) {
                    case 0: return tr("Title");
                    case 1: return tr("Description");
                    case 2: return tr("Href");
                    case 3: return tr("MIME Type");
                    case 4: return tr("Timestamp");
                    default: return {};
                }
            case ResourceKind::Chart:
                switch (section) {
                    case 0: return tr("Name");
                    case 1: return tr("Description");
                    case 2: return tr("Format");
                    case 3: return tr("Identifier");
                    case 4: return tr("Chart URL");
                    case 5: return tr("Timestamp");
                    default: return {};
                }
        }

        return {};
    }

    void ResourceModel::reload(const bool force) {
        if (m_reloadInProgress) {
            m_reloadPending = m_reloadPending || force;
            return;
        }

        m_reloadInProgress = true;
        const QPointer<ResourceModel> guard(this);
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        const QString currentCollection = collection();
        const auto resources = client->getResources(currentCollection);
        if (!guard) {
            return;
        }
        const QStringList ids = sortedResourceIds(resources);

        if (!force && ids == m_ids && resources == m_resources) {
            m_reloadInProgress = false;
            if (m_reloadPending) {
                m_reloadPending = false;
                reload(true);
            }
            return;
        }

        applyResources(resources);
        m_reloadInProgress = false;

        if (m_reloadPending) {
            m_reloadPending = false;
            reload(true);
        }
    }

    QString ResourceModel::createResource(const QJsonObject &resource) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        const QJsonObject response = client->createResource(collection(), resource);
        reload(true);
        return upsertedResourceId(response, resource);
    }

    bool ResourceModel::updateResource(const QString &id, const QJsonObject &resource) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        client->putResource(collection(), id, resource);
        reload(true);
        return m_resources.contains(id);
    }

    bool ResourceModel::deleteResource(const QString &id) {
        const auto client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
        client->deleteResource(collection(), id);
        reload(true);
        return !m_resources.contains(id);
    }

    bool ResourceModel::importDocument(const QJsonDocument &document, QString *message, int *count) {
        QList<QPair<QString, QJsonObject>> entries;
        if (document.isArray()) {
            for (const auto &value : document.array()) {
                if (value.isObject()) {
                    entries.append({QString(), value.toObject()});
                }
            }
        } else if (document.isObject()) {
            entries = importEntriesFromObject(document.object());
        }

        if (entries.isEmpty()) {
            if (message) {
                *message = tr("The selected file does not contain any importable %1.").arg(resourceKindToTitle(m_kind).toLower());
            }
            return false;
        }

        int imported = 0;
        for (const auto &entry : entries) {
            QJsonObject resource = entry.second;
            QString resourceId = idForResource(entry.first, resource);
            if (resourceId.isEmpty()) {
                resourceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            }

            if (m_resources.contains(resourceId)) {
                updateResource(resourceId, resource);
            } else {
                createResource(resource);
            }
            ++imported;
        }

        if (count) {
            *count = imported;
        }
        if (message) {
            *message = tr("Imported %1 %2.").arg(imported).arg(imported == 1 ? resourceKindToSingularTitle(m_kind).toLower() : resourceKindToTitle(m_kind).toLower());
        }
        reload(true);
        return true;
    }

    QJsonDocument ResourceModel::exportDocument(const QStringList &ids) const {
        QJsonArray array;
        const QStringList exportIds = ids.isEmpty() ? m_ids : ids;
        for (const auto &id : exportIds) {
            if (m_resources.contains(id)) {
                array.append(m_resources.value(id));
            }
        }
        return QJsonDocument(array);
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

    QString ResourceModel::collection() const {
        return resourceKindToCollection(m_kind);
    }

    void ResourceModel::onResourceUpdate(const QJsonObject &update) {
        QMap<QString, QJsonObject> nextResources = m_resources;
        bool changed = false;
        const QString collectionPrefix = collection() + ".";

        const QJsonArray updates = update["updates"].toArray();
        for (const auto &updateItem : updates) {
            const QJsonArray values = updateItem.toObject()["values"].toArray();
            for (const auto &valueItem : values) {
                const QJsonObject valueObject = valueItem.toObject();
                const QString path = valueObject["path"].toString();
                if (!path.startsWith(collectionPrefix)) {
                    continue;
                }

                const QStringList parts = path.split('.', Qt::SkipEmptyParts);
                if (parts.size() < 2) {
                    continue;
                }

                const QString resourceId = parts.at(1);
                const QStringList nestedPath = parts.mid(2);
                const QJsonValue value = valueObject.value("value");

                if (nestedPath.isEmpty()) {
                    if (value.isNull() || value.isUndefined()) {
                        changed = nextResources.remove(resourceId) > 0 || changed;
                        continue;
                    }
                    if (value.isObject()) {
                        if (nextResources.value(resourceId) != value.toObject()) {
                            nextResources.insert(resourceId, value.toObject());
                            changed = true;
                        }
                    }
                    continue;
                }

                QJsonObject resource = nextResources.value(resourceId);
                const QJsonValue updatedValue = applyJsonValuePath(resource, nestedPath, 0, value);
                if (!updatedValue.isObject()) {
                    continue;
                }

                const QJsonObject updatedResource = updatedValue.toObject();
                if (resource != updatedResource) {
                    nextResources.insert(resourceId, updatedResource);
                    changed = true;
                }
            }
        }

        if (changed) {
            applyResources(nextResources);
        }
    }

    void ResourceModel::applyResources(const QMap<QString, QJsonObject> &resources) {
        beginResetModel();
        m_resources = resources;
        m_ids = sortedResourceIds(resources);
        endResetModel();
    }
}
