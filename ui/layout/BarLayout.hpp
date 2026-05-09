#ifndef FAIRWINDSK_UI_LAYOUT_BARLAYOUT_HPP
#define FAIRWINDSK_UI_LAYOUT_BARLAYOUT_HPP

#include <QString>
#include <QList>
#include <nlohmann/json.hpp>

namespace fairwindsk::ui::layout {
    enum class BarId {
        Top,
        Bottom
    };

    enum class EntryKind {
        Widget,
        Separator,
        Stretch
    };

    struct WidgetDefinition {
        QString id;
        QString label;
        bool defaultTopEnabled = false;
        bool defaultBottomEnabled = false;
        bool dataWidget = false;
        bool expandHorizontally = false;
        bool expandVertically = false;
    };

    struct LayoutEntry {
        EntryKind kind = EntryKind::Widget;
        QString widgetId;
        QString instanceId;
        bool enabled = false;
        bool expandHorizontally = false;
        bool expandVertically = false;
        bool showIcon = true;
        bool showText = true;
        bool showUnits = true;
        bool showTrend = false;
    };

    QList<WidgetDefinition> widgetDefinitions();
    QList<WidgetDefinition> widgetDefinitions(const nlohmann::json &root);
    QList<WidgetDefinition> widgetDefinitions(const nlohmann::json &root, BarId barId);
    bool widgetAvailableOnBar(const nlohmann::json &root, BarId barId, const QString &widgetId);
    QString barLabel(BarId barId);
    QString barConfigKey(BarId barId);
    QString entryLabel(const LayoutEntry &entry);
    QString entryLabel(const LayoutEntry &entry, const nlohmann::json &root);
    QString horizontalSizeLabel(const LayoutEntry &entry);
    bool isWidgetEntry(const LayoutEntry &entry);
    bool isPlaceholderEntry(const LayoutEntry &entry);
    bool defaultExpandHorizontally(BarId barId, const QString &widgetId);
    bool defaultExpandHorizontally(const nlohmann::json &root, BarId barId, const QString &widgetId);
    QList<LayoutEntry> defaultEntries(BarId barId);
    QList<LayoutEntry> defaultEntries(const nlohmann::json &root, BarId barId);
    QList<LayoutEntry> entriesForBar(const nlohmann::json &root, BarId barId);
    QString layoutSignature(const nlohmann::json &root, BarId barId);
    void setEntriesForBar(nlohmann::json &root, BarId barId, const QList<LayoutEntry> &entries);
    void removeWidgetFromBar(nlohmann::json &root, BarId barId, const QString &widgetId);
}

#endif // FAIRWINDSK_UI_LAYOUT_BARLAYOUT_HPP
