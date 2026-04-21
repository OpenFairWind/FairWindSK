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
    };

    struct LayoutEntry {
        EntryKind kind = EntryKind::Widget;
        QString widgetId;
        QString instanceId;
        bool enabled = false;
        bool expandHorizontally = false;
        bool expandVertically = false;
    };

    QList<WidgetDefinition> widgetDefinitions();
    QString barLabel(BarId barId);
    QString barConfigKey(BarId barId);
    QString entryLabel(const LayoutEntry &entry);
    bool isWidgetEntry(const LayoutEntry &entry);
    bool isPlaceholderEntry(const LayoutEntry &entry);
    QList<LayoutEntry> defaultEntries(BarId barId);
    QList<LayoutEntry> entriesForBar(const nlohmann::json &root, BarId barId);
    void setEntriesForBar(nlohmann::json &root, BarId barId, const QList<LayoutEntry> &entries);
    void removeWidgetFromBar(nlohmann::json &root, BarId barId, const QString &widgetId);
}

#endif // FAIRWINDSK_UI_LAYOUT_BARLAYOUT_HPP
