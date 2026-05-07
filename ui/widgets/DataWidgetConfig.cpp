#include "DataWidgetConfig.hpp"

#include <algorithm>

#include <QCoreApplication>
#include <QRegularExpression>

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr auto kDataWidgetsKey = "dataWidgets";

        QString jsonString(const nlohmann::json &object, const char *key, const QString &fallback = QString()) {
            if (!object.is_object() || !object.contains(key) || !object[key].is_string()) {
                return fallback;
            }
            return QString::fromStdString(object[key].get<std::string>());
        }

        int jsonInt(const nlohmann::json &object, const char *key, const int fallback) {
            if (!object.is_object() || !object.contains(key) || !object[key].is_number_integer()) {
                return fallback;
            }
            return object[key].get<int>();
        }

        double jsonDouble(const nlohmann::json &object, const char *key, const double fallback) {
            if (!object.is_object() || !object.contains(key) || !object[key].is_number()) {
                return fallback;
            }
            return object[key].get<double>();
        }

        bool jsonBool(const nlohmann::json &object, const char *key, const bool fallback = false) {
            if (!object.is_object() || !object.contains(key) || !object[key].is_boolean()) {
                return fallback;
            }
            return object[key].get<bool>();
        }

        QString legacySignalKPath(const nlohmann::json &root,
                                  const QString &key,
                                  const QString &fallback) {
            const auto keyString = key.toStdString();
            if (root.is_object() &&
                root.contains("signalk") &&
                root["signalk"].is_object() &&
                root["signalk"].contains(keyString) &&
                root["signalk"][keyString].is_string()) {
                return QString::fromStdString(root["signalk"][keyString].get<std::string>());
            }
            return fallback;
        }

        QString normalizedIdBase(const QString &value) {
            QString id = value.trimmed().toLower();
            id.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("_"));
            id.replace(QRegularExpression(QStringLiteral("^_+|_+$")), QString());
            return id.isEmpty() ? QStringLiteral("data_widget") : id;
        }

        DataWidgetDefinition parseDataWidgetDefinition(const nlohmann::json &object) {
            DataWidgetDefinition definition;
            definition.id = jsonString(object, "id");
            definition.name = jsonString(object, "name", definition.id);
            definition.icon = jsonString(object, "icon");
            definition.signalKPath = jsonString(object, "signalKPath");
            definition.sourceUnit = jsonString(object, "sourceUnit");
            definition.defaultUnit = jsonString(object, "defaultUnit");
            definition.updatePolicy = jsonString(object, "updatePolicy", QStringLiteral("ideal"));
            definition.dateTimeFormat = jsonString(object, "dateTimeFormat");
            definition.kind = dataWidgetKindFromId(jsonString(object, "type", QStringLiteral("numeric")));
            definition.period = std::max(100, jsonInt(object, "period", 1000));
            definition.minPeriod = std::max(0, jsonInt(object, "minPeriod", 200));
            definition.minimum = jsonDouble(object, "minimum", 0.0);
            definition.maximum = jsonDouble(object, "maximum", 100.0);
            if (definition.maximum <= definition.minimum) {
                definition.maximum = definition.minimum + 100.0;
            }
            definition.defaultTopEnabled = jsonBool(object, "defaultTopEnabled", false);
            definition.defaultBottomEnabled = jsonBool(object, "defaultBottomEnabled", false);
            definition.expandHorizontally = jsonBool(object, "expandHorizontally", false);
            definition.expandVertically = jsonBool(object, "expandVertically", false);

            if (definition.id.trimmed().isEmpty()) {
                definition.id = normalizedIdBase(definition.name);
            }
            if (definition.name.trimmed().isEmpty()) {
                definition.name = definition.id;
            }
            return definition;
        }

        DataWidgetDefinition legacyDataWidget(const nlohmann::json &root,
                                              const QString &id,
                                              const QString &name,
                                              const QString &icon,
                                              const QString &legacyPathKey,
                                              const QString &fallbackPath,
                                              const DataWidgetKind kind,
                                              const QString &sourceUnit = QString(),
                                              const QString &defaultUnit = QString(),
                                              const QString &dateTimeFormat = QString(),
                                              const bool expandHorizontally = false) {
            DataWidgetDefinition definition;
            definition.id = id;
            definition.name = name;
            definition.icon = icon;
            definition.signalKPath = legacySignalKPath(root, legacyPathKey, fallbackPath);
            definition.sourceUnit = sourceUnit;
            definition.defaultUnit = defaultUnit;
            definition.dateTimeFormat = dateTimeFormat;
            definition.kind = kind;
            definition.updatePolicy = QStringLiteral("ideal");
            definition.period = 1000;
            definition.minPeriod = 200;
            definition.defaultTopEnabled = true;
            definition.defaultBottomEnabled = false;
            definition.expandHorizontally = expandHorizontally;
            definition.expandVertically = false;
            return definition;
        }

        QList<DataWidgetDefinition> legacyDataWidgetDefinitions(const nlohmann::json &root) {
            return {
                legacyDataWidget(root, QStringLiteral("position"), QCoreApplication::translate("DataWidgetConfig", "Position"), QStringLiteral(":/resources/svg/OpenBridge/lcd-position.svg"), QStringLiteral("pos"), QStringLiteral("navigation.position"), DataWidgetKind::Position, {}, {}, {}, true),
                legacyDataWidget(root, QStringLiteral("cog"), QCoreApplication::translate("DataWidgetConfig", "COG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-cog.svg"), QStringLiteral("cog"), QStringLiteral("navigation.courseOverGroundTrue"), DataWidgetKind::Numeric, QStringLiteral("rad"), QStringLiteral("deg")),
                legacyDataWidget(root, QStringLiteral("sog"), QCoreApplication::translate("DataWidgetConfig", "SOG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-sog.svg"), QStringLiteral("sog"), QStringLiteral("navigation.speedOverGround"), DataWidgetKind::Numeric, QStringLiteral("ms-1"), QStringLiteral("kn")),
                legacyDataWidget(root, QStringLiteral("hdg"), QCoreApplication::translate("DataWidgetConfig", "HDG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-hdg.svg"), QStringLiteral("hdg"), QStringLiteral("navigation.headingTrue"), DataWidgetKind::Numeric, QStringLiteral("rad"), QStringLiteral("deg")),
                legacyDataWidget(root, QStringLiteral("stw"), QCoreApplication::translate("DataWidgetConfig", "STW"), QStringLiteral(":/resources/svg/OpenBridge/lcd-stw.svg"), QStringLiteral("stw"), QStringLiteral("navigation.speedThroughWater"), DataWidgetKind::Numeric, QStringLiteral("ms-1"), QStringLiteral("kn")),
                legacyDataWidget(root, QStringLiteral("dpt"), QCoreApplication::translate("DataWidgetConfig", "DPT"), QStringLiteral(":/resources/svg/OpenBridge/lcd-dpt.svg"), QStringLiteral("dpt"), QStringLiteral("environment.depth.belowTransducer"), DataWidgetKind::Numeric, QStringLiteral("m"), QStringLiteral("mt")),
                legacyDataWidget(root, QStringLiteral("wpt"), QCoreApplication::translate("DataWidgetConfig", "WPT"), QStringLiteral(":/resources/svg/OpenBridge/lcd-wpt.svg"), QStringLiteral("wpt"), QStringLiteral("navigation.course.nextPoint"), DataWidgetKind::Waypoint),
                legacyDataWidget(root, QStringLiteral("btw"), QCoreApplication::translate("DataWidgetConfig", "BTW"), QStringLiteral(":/resources/svg/OpenBridge/lcd-btw.svg"), QStringLiteral("btw"), QStringLiteral("navigation.course.calcValues.bearingTrue"), DataWidgetKind::Numeric, QStringLiteral("rad"), QStringLiteral("deg")),
                legacyDataWidget(root, QStringLiteral("dtg"), QCoreApplication::translate("DataWidgetConfig", "DTG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-dtg.svg"), QStringLiteral("dtg"), QStringLiteral("navigation.course.calcValues.distance"), DataWidgetKind::Numeric, QStringLiteral("m"), QStringLiteral("nm")),
                legacyDataWidget(root, QStringLiteral("ttg"), QCoreApplication::translate("DataWidgetConfig", "TTG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-ttg.svg"), QStringLiteral("ttg"), QStringLiteral("navigation.course.calcValues.timeToGo"), DataWidgetKind::DateTime, {}, {}, QStringLiteral("hh:mm")),
                legacyDataWidget(root, QStringLiteral("eta"), QCoreApplication::translate("DataWidgetConfig", "ETA"), QStringLiteral(":/resources/svg/OpenBridge/lcd-eta.svg"), QStringLiteral("eta"), QStringLiteral("navigation.course.calcValues.estimatedTimeOfArrival"), DataWidgetKind::DateTime, {}, {}, QStringLiteral("dd-MM-yyyy hh:mm")),
                legacyDataWidget(root, QStringLiteral("xte"), QCoreApplication::translate("DataWidgetConfig", "XTE"), QStringLiteral(":/resources/svg/OpenBridge/lcd-xte.svg"), QStringLiteral("xte"), QStringLiteral("navigation.course.calcValues.crossTrackError"), DataWidgetKind::Numeric, QStringLiteral("m"), QStringLiteral("nm")),
                legacyDataWidget(root, QStringLiteral("vmg"), QCoreApplication::translate("DataWidgetConfig", "VMG"), QStringLiteral(":/resources/svg/OpenBridge/lcd-vmg.svg"), QStringLiteral("vmg"), QStringLiteral("performance.velocityMadeGood"), DataWidgetKind::Numeric, QStringLiteral("ms-1"), QStringLiteral("kn"))
            };
        }
    }

    QString dataWidgetConfigKey() {
        return QString::fromLatin1(kDataWidgetsKey);
    }

    QString dataWidgetKindId(const DataWidgetKind kind) {
        switch (kind) {
            case DataWidgetKind::Gauge:
                return QStringLiteral("gauge");
            case DataWidgetKind::Position:
                return QStringLiteral("position");
            case DataWidgetKind::DateTime:
                return QStringLiteral("datetime");
            case DataWidgetKind::Waypoint:
                return QStringLiteral("waypoint");
            case DataWidgetKind::Numeric:
            default:
                return QStringLiteral("numerical");
        }
    }

    QString dataWidgetKindLabel(const DataWidgetKind kind) {
        switch (kind) {
            case DataWidgetKind::Gauge:
                return QCoreApplication::translate("DataWidgetConfig", "Gauge");
            case DataWidgetKind::Position:
                return QCoreApplication::translate("DataWidgetConfig", "Position");
            case DataWidgetKind::DateTime:
                return QCoreApplication::translate("DataWidgetConfig", "Date/Time");
            case DataWidgetKind::Waypoint:
                return QCoreApplication::translate("DataWidgetConfig", "Waypoint");
            case DataWidgetKind::Numeric:
            default:
                return QCoreApplication::translate("DataWidgetConfig", "Numerical");
        }
    }

    DataWidgetKind dataWidgetKindFromId(const QString &id) {
        const QString normalized = id.trimmed().toLower();
        if (normalized == QStringLiteral("gauge")) {
            return DataWidgetKind::Gauge;
        }
        if (normalized == QStringLiteral("position")) {
            return DataWidgetKind::Position;
        }
        if (normalized == QStringLiteral("datetime") ||
            normalized == QStringLiteral("date_time") ||
            normalized == QStringLiteral("date-time")) {
            return DataWidgetKind::DateTime;
        }
        if (normalized == QStringLiteral("waypoint")) {
            return DataWidgetKind::Waypoint;
        }
        if (normalized == QStringLiteral("numeric") ||
            normalized == QStringLiteral("number") ||
            normalized == QStringLiteral("numerical")) {
            return DataWidgetKind::Numeric;
        }
        return DataWidgetKind::Numeric;
    }

    QList<DataWidgetDefinition> dataWidgetDefinitions(const nlohmann::json &root) {
        QList<DataWidgetDefinition> definitions;
        if (!root.is_object() || !root.contains(kDataWidgetsKey) || !root[kDataWidgetsKey].is_array()) {
            return legacyDataWidgetDefinitions(root);
        }

        QStringList ids;
        for (const auto &item : root[kDataWidgetsKey]) {
            if (!item.is_object()) {
                continue;
            }

            auto definition = parseDataWidgetDefinition(item);
            if (definition.id.trimmed().isEmpty() || ids.contains(definition.id)) {
                continue;
            }
            ids.append(definition.id);
            definitions.append(definition);
        }
        return definitions;
    }

    DataWidgetDefinition dataWidgetDefinition(const nlohmann::json &root, const QString &id) {
        const auto definitions = dataWidgetDefinitions(root);
        const auto it = std::find_if(definitions.cbegin(), definitions.cend(), [&id](const DataWidgetDefinition &definition) {
            return definition.id == id;
        });
        return it == definitions.cend() ? DataWidgetDefinition{} : *it;
    }

    bool isDataWidgetId(const nlohmann::json &root, const QString &id) {
        return !dataWidgetDefinition(root, id).id.isEmpty();
    }

    QString uniqueDataWidgetId(const nlohmann::json &root, const QString &name) {
        const QString base = normalizedIdBase(name);
        QString candidate = base;
        int suffix = 2;
        while (isDataWidgetId(root, candidate)) {
            candidate = QStringLiteral("%1_%2").arg(base).arg(suffix++);
        }
        return candidate;
    }

    nlohmann::json dataWidgetDefinitionToJson(const DataWidgetDefinition &definition) {
        nlohmann::json object = nlohmann::json::object();
        object["id"] = definition.id.toStdString();
        object["name"] = definition.name.toStdString();
        object["icon"] = definition.icon.toStdString();
        object["type"] = dataWidgetKindId(definition.kind).toStdString();
        object["signalKPath"] = definition.signalKPath.toStdString();
        object["sourceUnit"] = definition.sourceUnit.toStdString();
        object["defaultUnit"] = definition.defaultUnit.toStdString();
        object["updatePolicy"] = definition.updatePolicy.toStdString();
        object["period"] = definition.period;
        object["minPeriod"] = definition.minPeriod;
        object["minimum"] = definition.minimum;
        object["maximum"] = definition.maximum;
        object["dateTimeFormat"] = definition.dateTimeFormat.toStdString();
        object["defaultTopEnabled"] = definition.defaultTopEnabled;
        object["defaultBottomEnabled"] = definition.defaultBottomEnabled;
        object["expandHorizontally"] = definition.expandHorizontally;
        object["expandVertically"] = definition.expandVertically;
        return object;
    }

    void setDataWidgetDefinitions(nlohmann::json &root, const QList<DataWidgetDefinition> &definitions) {
        root[kDataWidgetsKey] = nlohmann::json::array();
        for (const auto &definition : definitions) {
            if (definition.id.trimmed().isEmpty()) {
                continue;
            }
            root[kDataWidgetsKey].push_back(dataWidgetDefinitionToJson(definition));
        }
    }

    bool upsertDataWidgetDefinition(nlohmann::json &root, const DataWidgetDefinition &definition) {
        if (definition.id.trimmed().isEmpty()) {
            return false;
        }

        auto definitions = dataWidgetDefinitions(root);
        bool updated = false;
        for (auto &item : definitions) {
            if (item.id == definition.id) {
                item = definition;
                updated = true;
                break;
            }
        }
        if (!updated) {
            definitions.append(definition);
        }
        setDataWidgetDefinitions(root, definitions);
        return true;
    }

    bool removeDataWidgetDefinition(nlohmann::json &root, const QString &id) {
        auto definitions = dataWidgetDefinitions(root);
        const int before = definitions.size();
        definitions.erase(std::remove_if(definitions.begin(), definitions.end(), [&id](const DataWidgetDefinition &definition) {
            return definition.id == id;
        }), definitions.end());
        setDataWidgetDefinitions(root, definitions);
        return definitions.size() != before;
    }
}
