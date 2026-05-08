#include "BarLayout.hpp"

#include <algorithm>

#include <QObject>
#include <QHash>
#include <QSet>

#include "FairWindSK.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"

namespace fairwindsk::ui::layout {
    namespace {
        QString entryKindToString(const EntryKind kind) {
            switch (kind) {
                case EntryKind::Widget:
                    return QStringLiteral("widget");
                case EntryKind::Separator:
                    return QStringLiteral("separator");
                case EntryKind::Stretch:
                    return QStringLiteral("stretch");
            }

            return QStringLiteral("widget");
        }

        EntryKind entryKindFromString(const QString &value) {
            if (value == QStringLiteral("separator")) {
                return EntryKind::Separator;
            }
            if (value == QStringLiteral("stretch")) {
                return EntryKind::Stretch;
            }
            return EntryKind::Widget;
        }

        const QList<WidgetDefinition> &fixedWidgetDefinitionsStorage() {
            static const QList<WidgetDefinition> definitions = {
                {QStringLiteral("current_context"), QObject::tr("Current Application / Launcher Page Label"), true, false, false, true},
                {QStringLiteral("clock_icons"), QObject::tr("Clock and Icons"), true, false, false, true},
                {QStringLiteral("open_apps"), QObject::tr("Currently Open Applications"), false, true, false, true},
                {QStringLiteral("mydata"), QObject::tr("MyData"), false, true},
                {QStringLiteral("pob"), QObject::tr("POB"), false, true},
                {QStringLiteral("autopilot"), QObject::tr("Autopilot"), false, true},
                {QStringLiteral("apps"), QObject::tr("Apps"), false, true},
                {QStringLiteral("anchor"), QObject::tr("Anchor"), false, true},
                {QStringLiteral("alarms"), QObject::tr("Alarms"), false, true},
                {QStringLiteral("settings"), QObject::tr("Settings"), false, true},
                {QStringLiteral("signalk_status"), QObject::tr("SignalK Status"), false, true, false, true}
            };
            return definitions;
        }

        QList<WidgetDefinition> widgetDefinitionsStorage(const nlohmann::json &root) {
            QList<WidgetDefinition> definitions;
            const auto dataWidgets = fairwindsk::ui::widgets::dataWidgetDefinitions(root);
            definitions.reserve(dataWidgets.size() + fixedWidgetDefinitionsStorage().size());

            for (const auto &dataWidget : dataWidgets) {
                definitions.append({
                    dataWidget.id,
                    dataWidget.name,
                    dataWidget.defaultTopEnabled,
                    dataWidget.defaultBottomEnabled,
                    true,
                    dataWidget.expandHorizontally,
                    dataWidget.expandVertically
                });
            }

            definitions.append(fixedWidgetDefinitionsStorage());
            return definitions;
        }

        const nlohmann::json &runtimeConfigurationRoot() {
            static const nlohmann::json emptyRoot = nlohmann::json::object();
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            return configuration ? configuration->getRoot() : emptyRoot;
        }

        QHash<QString, WidgetDefinition> widgetDefinitionIndex(const nlohmann::json &root) {
            QHash<QString, WidgetDefinition> index;
            const auto definitions = widgetDefinitionsStorage(root);
            for (const auto &definition : definitions) {
                index.insert(definition.id, definition);
            }
            return index;
        }

        bool fixedWidgetAvailableOnBar(const BarId barId, const QString &widgetId) {
            if (barId == BarId::Top) {
                return widgetId == QStringLiteral("current_context") ||
                       widgetId == QStringLiteral("clock_icons");
            }

            return widgetId == QStringLiteral("open_apps") ||
                   widgetId == QStringLiteral("mydata") ||
                   widgetId == QStringLiteral("pob") ||
                   widgetId == QStringLiteral("autopilot") ||
                   widgetId == QStringLiteral("apps") ||
                   widgetId == QStringLiteral("anchor") ||
                   widgetId == QStringLiteral("alarms") ||
                   widgetId == QStringLiteral("settings") ||
                   widgetId == QStringLiteral("signalk_status");
        }

        bool widgetAvailableOnBarInternal(const nlohmann::json &root,
                                          const BarId barId,
                                          const QString &widgetId) {
            if (widgetId.trimmed().isEmpty()) {
                return false;
            }
            if (fairwindsk::ui::widgets::isDataWidgetId(root, widgetId)) {
                return true;
            }

            return fixedWidgetAvailableOnBar(barId, widgetId);
        }

        QString configKeyForBar(const BarId barId) {
            return barId == BarId::Top ? QStringLiteral("top") : QStringLiteral("bottom");
        }

        bool shouldDefaultToMaximumWidth(const nlohmann::json &root, const BarId barId, const QString &widgetId) {
            const auto definitions = widgetDefinitionsStorage(root);
            const auto definitionIt = std::find_if(definitions.cbegin(), definitions.cend(), [&widgetId](const WidgetDefinition &definition) {
                return definition.id == widgetId;
            });
            if (definitionIt != definitions.cend() && definitionIt->expandHorizontally) {
                return true;
            }

            if (barId == BarId::Top) {
                return widgetId == QStringLiteral("current_context") ||
                       widgetId == QStringLiteral("clock_icons");
            }

            return widgetId == QStringLiteral("open_apps") ||
                   widgetId == QStringLiteral("signalk_status");
        }

        LayoutEntry parseEntry(const nlohmann::json &jsonEntry) {
            LayoutEntry entry;
            if (!jsonEntry.is_object()) {
                return entry;
            }

            if (jsonEntry.contains("kind") && jsonEntry["kind"].is_string()) {
                entry.kind = entryKindFromString(QString::fromStdString(jsonEntry["kind"].get<std::string>()));
            }
            if (jsonEntry.contains("widgetId") && jsonEntry["widgetId"].is_string()) {
                entry.widgetId = QString::fromStdString(jsonEntry["widgetId"].get<std::string>());
            }
            if (jsonEntry.contains("instanceId") && jsonEntry["instanceId"].is_string()) {
                entry.instanceId = QString::fromStdString(jsonEntry["instanceId"].get<std::string>());
            }
            if (jsonEntry.contains("enabled") && jsonEntry["enabled"].is_boolean()) {
                entry.enabled = jsonEntry["enabled"].get<bool>();
            }
            if (jsonEntry.contains("expandHorizontally") && jsonEntry["expandHorizontally"].is_boolean()) {
                entry.expandHorizontally = jsonEntry["expandHorizontally"].get<bool>();
            }
            if (jsonEntry.contains("expandVertically") && jsonEntry["expandVertically"].is_boolean()) {
                entry.expandVertically = jsonEntry["expandVertically"].get<bool>();
            }
            if (jsonEntry.contains("showIcon") && jsonEntry["showIcon"].is_boolean()) {
                entry.showIcon = jsonEntry["showIcon"].get<bool>();
            }
            if (jsonEntry.contains("showText") && jsonEntry["showText"].is_boolean()) {
                entry.showText = jsonEntry["showText"].get<bool>();
            }
            if (jsonEntry.contains("showUnits") && jsonEntry["showUnits"].is_boolean()) {
                entry.showUnits = jsonEntry["showUnits"].get<bool>();
            }
            if (jsonEntry.contains("showTrend") && jsonEntry["showTrend"].is_boolean()) {
                entry.showTrend = jsonEntry["showTrend"].get<bool>();
            }

            if (entry.kind == EntryKind::Widget && entry.instanceId.isEmpty()) {
                entry.instanceId = entry.widgetId;
            }
            if (entry.kind != EntryKind::Widget && entry.instanceId.isEmpty()) {
                entry.instanceId = entryKindToString(entry.kind);
            }
            if (entry.kind == EntryKind::Stretch && !jsonEntry.contains("expandHorizontally")) {
                entry.expandHorizontally = true;
            }

            return entry;
        }

        nlohmann::json toJson(const LayoutEntry &entry) {
            nlohmann::json jsonEntry = nlohmann::json::object();
            jsonEntry["kind"] = entryKindToString(entry.kind).toStdString();
            jsonEntry["enabled"] = entry.enabled;
            if (!entry.widgetId.isEmpty()) {
                jsonEntry["widgetId"] = entry.widgetId.toStdString();
            }
            if (!entry.instanceId.isEmpty()) {
                jsonEntry["instanceId"] = entry.instanceId.toStdString();
            }
            jsonEntry["expandHorizontally"] = entry.expandHorizontally;
            jsonEntry["expandVertically"] = entry.expandVertically;
            jsonEntry["showIcon"] = entry.showIcon;
            jsonEntry["showText"] = entry.showText;
            jsonEntry["showUnits"] = entry.showUnits;
            jsonEntry["showTrend"] = entry.showTrend;
            return jsonEntry;
        }

        void removeDuplicateWidgetEntries(QList<LayoutEntry> &entries) {
            QSet<QString> seenWidgets;
            QList<LayoutEntry> normalized;
            normalized.reserve(entries.size());

            for (const auto &entry : entries) {
                if (entry.kind == EntryKind::Widget) {
                    if (seenWidgets.contains(entry.widgetId)) {
                        continue;
                    }
                    seenWidgets.insert(entry.widgetId);
                }
                normalized.append(entry);
            }

            entries = normalized;
        }

        void removeUnsupportedWidgetEntries(QList<LayoutEntry> &entries,
                                            const nlohmann::json &root,
                                            const BarId barId) {
            QList<LayoutEntry> normalized;
            normalized.reserve(entries.size());

            for (const auto &entry : entries) {
                if (entry.kind == EntryKind::Widget &&
                    !widgetAvailableOnBarInternal(root, barId, entry.widgetId)) {
                    continue;
                }
                normalized.append(entry);
            }

            entries = normalized;
        }

        void ensureAllWidgetEntriesPresent(QList<LayoutEntry> &entries,
                                           const nlohmann::json &root,
                                           const BarId barId) {
            const auto definitions = widgetDefinitionsStorage(root);
            for (const auto &definition : definitions) {
                if (!widgetAvailableOnBarInternal(root, barId, definition.id)) {
                    continue;
                }

                const bool present = std::any_of(entries.cbegin(), entries.cend(), [&definition](const LayoutEntry &entry) {
                    return entry.kind == EntryKind::Widget && entry.widgetId == definition.id;
                });
                if (!present) {
                    LayoutEntry entry;
                    entry.kind = EntryKind::Widget;
                    entry.widgetId = definition.id;
                    entry.instanceId = definition.id;
                    entry.enabled = false;
                    entry.expandHorizontally = shouldDefaultToMaximumWidth(root, barId, definition.id);
                    entry.expandVertically = definition.expandVertically;
                    entry.showIcon = !definition.dataWidget;
                    entries.append(entry);
                }
            }
        }
    }

    QList<WidgetDefinition> widgetDefinitions() {
        return widgetDefinitions(runtimeConfigurationRoot());
    }

    QList<WidgetDefinition> widgetDefinitions(const nlohmann::json &root) {
        return widgetDefinitionsStorage(root);
    }

    QList<WidgetDefinition> widgetDefinitions(const nlohmann::json &root, const BarId barId) {
        QList<WidgetDefinition> definitions;
        const auto allDefinitions = widgetDefinitionsStorage(root);
        definitions.reserve(allDefinitions.size());

        for (const auto &definition : allDefinitions) {
            if (widgetAvailableOnBarInternal(root, barId, definition.id)) {
                definitions.append(definition);
            }
        }

        return definitions;
    }

    bool widgetAvailableOnBar(const nlohmann::json &root,
                              const BarId barId,
                              const QString &widgetId) {
        return widgetAvailableOnBarInternal(root, barId, widgetId);
    }

    QString barLabel(const BarId barId) {
        return barId == BarId::Top ? QObject::tr("Top Bar") : QObject::tr("Bottom Bar");
    }

    QString barConfigKey(const BarId barId) {
        return configKeyForBar(barId);
    }

    QString entryLabel(const LayoutEntry &entry) {
        return entryLabel(entry, runtimeConfigurationRoot());
    }

    QString entryLabel(const LayoutEntry &entry, const nlohmann::json &root) {
        if (entry.kind == EntryKind::Separator) {
            return QObject::tr("Separator");
        }
        if (entry.kind == EntryKind::Stretch) {
            return QObject::tr("Elastic Extender");
        }

        const QHash<QString, WidgetDefinition> index = widgetDefinitionIndex(root);
        if (index.contains(entry.widgetId)) {
            return index.value(entry.widgetId).label;
        }

        return entry.widgetId;
    }

    QString horizontalSizeLabel(const LayoutEntry &entry) {
        if (entry.kind == EntryKind::Stretch) {
            return QObject::tr("Maximum possible size");
        }

        return entry.expandHorizontally
                   ? QObject::tr("Maximum possible size")
                   : QObject::tr("Minimum needed size");
    }

    bool isWidgetEntry(const LayoutEntry &entry) {
        return entry.kind == EntryKind::Widget;
    }

    bool isPlaceholderEntry(const LayoutEntry &entry) {
        return entry.kind == EntryKind::Separator || entry.kind == EntryKind::Stretch;
    }

    bool defaultExpandHorizontally(const BarId barId, const QString &widgetId) {
        return defaultExpandHorizontally(runtimeConfigurationRoot(), barId, widgetId);
    }

    QList<LayoutEntry> defaultEntries(const BarId barId) {
        return defaultEntries(runtimeConfigurationRoot(), barId);
    }

    bool defaultExpandHorizontally(const nlohmann::json &root, const BarId barId, const QString &widgetId) {
        return shouldDefaultToMaximumWidth(root, barId, widgetId);
    }

    QList<LayoutEntry> defaultEntries(const nlohmann::json &root, const BarId barId) {
        QList<LayoutEntry> entries;
        const auto definitions = widgetDefinitions(root, barId);
        for (const auto &definition : definitions) {
            LayoutEntry entry;
            entry.kind = EntryKind::Widget;
            entry.widgetId = definition.id;
            entry.instanceId = definition.id;
            entry.enabled = barId == BarId::Top ? definition.defaultTopEnabled : definition.defaultBottomEnabled;
            entry.expandHorizontally = entry.enabled && shouldDefaultToMaximumWidth(root, barId, definition.id);
            entry.expandVertically = entry.enabled && definition.expandVertically;
            entry.showIcon = !definition.dataWidget;
            entries.append(entry);
        }
        return entries;
    }

    QList<LayoutEntry> entriesForBar(const nlohmann::json &root, const BarId barId) {
        QList<LayoutEntry> entries;
        if (root.contains("barLayouts") &&
            root["barLayouts"].is_object() &&
            root["barLayouts"].contains(configKeyForBar(barId).toStdString()) &&
            root["barLayouts"][configKeyForBar(barId).toStdString()].is_array()) {
            for (const auto &jsonEntry : root["barLayouts"][configKeyForBar(barId).toStdString()]) {
                const auto entry = parseEntry(jsonEntry);
                if (entry.kind == EntryKind::Widget && entry.widgetId.isEmpty()) {
                    continue;
                }
                entries.append(entry);
            }
        }

        if (entries.isEmpty()) {
            entries = defaultEntries(root, barId);
        } else {
            removeDuplicateWidgetEntries(entries);
            removeUnsupportedWidgetEntries(entries, root, barId);
            ensureAllWidgetEntriesPresent(entries, root, barId);
        }

        return entries;
    }

    void setEntriesForBar(nlohmann::json &root, const BarId barId, const QList<LayoutEntry> &entries) {
        auto &layoutRoot = root["barLayouts"];
        layoutRoot[configKeyForBar(barId).toStdString()] = nlohmann::json::array();
        for (const auto &entry : entries) {
            if (entry.kind == EntryKind::Widget && entry.widgetId.isEmpty()) {
                continue;
            }
            if (entry.kind == EntryKind::Widget && !widgetAvailableOnBarInternal(root, barId, entry.widgetId)) {
                continue;
            }
            layoutRoot[configKeyForBar(barId).toStdString()].push_back(toJson(entry));
        }
    }

    void removeWidgetFromBar(nlohmann::json &root, const BarId barId, const QString &widgetId) {
        auto entries = entriesForBar(root, barId);
        for (auto &entry : entries) {
            if (entry.kind == EntryKind::Widget && entry.widgetId == widgetId) {
                entry.enabled = false;
            }
        }
        setEntriesForBar(root, barId, entries);
    }
}
