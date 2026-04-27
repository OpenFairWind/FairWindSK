#ifndef FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
#define FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP

#include <QListWidget>
#include <QToolButton>
#include <QWidget>

#include "Settings.hpp"
#include "WidgetPalette.hpp"
#include "ui/layout/BarLayout.hpp"

class QLabel;
class QListWidgetItem;
class QFrame;

namespace fairwindsk::ui::settings {
    class BarLayoutSettings : public QWidget {
        Q_OBJECT

    public:
        explicit BarLayoutSettings(Settings *settings,
                                   fairwindsk::ui::layout::BarId barId,
                                   QWidget *parent = nullptr);
        ~BarLayoutSettings() override = default;

        void refreshFromConfiguration();

    private slots:
        void onItemChanged(QListWidgetItem *item);
        void onSelectionChanged();
        void onPaletteEntryActivated(const fairwindsk::ui::layout::LayoutEntry &entry);
        void onMinimumWidthSelected();
        void onMaximumWidthSelected();
        void onRemoveSelected();
        void onResetDefaults();
        void onPreviewEdited();

    private:
        enum ItemRoles {
            RoleKind = Qt::UserRole + 1,
            RoleWidgetId,
            RoleInstanceId,
            RoleExpandHorizontally
        };

        void buildUi();
        void applyChrome();
        void populateFromConfiguration();
        void persistToConfiguration();
        fairwindsk::ui::layout::LayoutEntry entryForItem(const QListWidgetItem *item) const;
        QListWidgetItem *createItem(const fairwindsk::ui::layout::LayoutEntry &entry);
        void refreshPreviewItem(QListWidgetItem *item) const;
        QSize itemSizeHint(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        void updateActions();
        void appendPlaceholder(fairwindsk::ui::layout::EntryKind kind);
        void activateWidgetEntry(const QString &widgetId);
        bool defaultExpandHorizontally(const QString &widgetId) const;
        bool m_populating = false;
        Settings *m_settings = nullptr;
        fairwindsk::ui::layout::BarId m_barId = fairwindsk::ui::layout::BarId::Top;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QLabel *m_previewLabel = nullptr;
        QFrame *m_previewFrame = nullptr;
        QListWidget *m_listWidget = nullptr;
        QToolButton *m_minimumWidthButton = nullptr;
        QToolButton *m_maximumWidthButton = nullptr;
        QToolButton *m_removeSelectedButton = nullptr;
        QToolButton *m_resetDefaultsButton = nullptr;
        QLabel *m_paletteLabel = nullptr;
        WidgetPalette *m_paletteWidget = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
