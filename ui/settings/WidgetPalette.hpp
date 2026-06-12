#ifndef FAIRWINDSK_UI_SETTINGS_WIDGETPALETTE_HPP
#define FAIRWINDSK_UI_SETTINGS_WIDGETPALETTE_HPP

#include <QIcon>
#include <QScrollArea>

#include "ui/layout/BarLayout.hpp"

class QGridLayout;
class QPushButton;
class QWidget;

namespace fairwindsk::ui::settings {
    class WidgetPalette : public QScrollArea {
        Q_OBJECT

    public:
        explicit WidgetPalette(QWidget *parent = nullptr);
        ~WidgetPalette() override = default;

        void setEntries(const QList<fairwindsk::ui::layout::LayoutEntry> &entries);
        void setActiveEntries(const QList<fairwindsk::ui::layout::LayoutEntry> &entries);

        static QString mimeType();
        static QByteArray encodeEntry(const fairwindsk::ui::layout::LayoutEntry &entry);
        static fairwindsk::ui::layout::LayoutEntry decodeEntry(const QByteArray &payload);
        static QIcon entryIcon(const fairwindsk::ui::layout::LayoutEntry &entry);

    signals:
        void entryActivated(const fairwindsk::ui::layout::LayoutEntry &entry);

    private:
        struct PaletteButton {
            fairwindsk::ui::layout::LayoutEntry entry;
            QPushButton *button = nullptr;
        };

        void rebuild();
        void applyChrome();

        QWidget *m_container = nullptr;
        QGridLayout *m_layout = nullptr;
        QList<PaletteButton> m_buttons;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_WIDGETPALETTE_HPP
