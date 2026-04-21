#ifndef FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
#define FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

#include "Settings.hpp"
#include "ui/layout/BarLayout.hpp"

class QLabel;
class QListWidgetItem;

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
        void onAddSeparator();
        void onAddStretch();
        void onRemoveSelected();
        void onResetDefaults();

    private:
        enum ItemRoles {
            RoleKind = Qt::UserRole + 1,
            RoleWidgetId,
            RoleInstanceId
        };

        void buildUi();
        void populateFromConfiguration();
        void persistToConfiguration();
        fairwindsk::ui::layout::LayoutEntry entryForItem(const QListWidgetItem *item) const;
        QListWidgetItem *createItem(const fairwindsk::ui::layout::LayoutEntry &entry);
        void updateActions();
        void appendPlaceholder(fairwindsk::ui::layout::EntryKind kind);
        bool m_populating = false;
        Settings *m_settings = nullptr;
        fairwindsk::ui::layout::BarId m_barId = fairwindsk::ui::layout::BarId::Top;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QListWidget *m_listWidget = nullptr;
        QPushButton *m_addSeparatorButton = nullptr;
        QPushButton *m_addStretchButton = nullptr;
        QPushButton *m_removeSelectedButton = nullptr;
        QPushButton *m_resetDefaultsButton = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
