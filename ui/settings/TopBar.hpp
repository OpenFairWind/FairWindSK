#ifndef FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP
#define FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP

#include <QListWidget>
#include <QToolButton>
#include <QWidget>

#include "Settings.hpp"
#include "ui/layout/BarLayout.hpp"

class QLabel;
class QListWidgetItem;

namespace fairwindsk::ui::settings {
    class TopBar : public QWidget {
        Q_OBJECT

    public:
        explicit TopBar(Settings *settings, QWidget *parent = nullptr);
        ~TopBar() override = default;

        void refreshFromConfiguration();

        enum ItemRoles {
            RoleKind = Qt::UserRole + 1,
            RoleWidgetId,
            RoleInstanceId,
            RoleExpandHorizontally,
            RoleExpandVertically,
            RolePaletteKind,
            RolePaletteWidgetId
        };

    private slots:
        void onPaletteItemClicked(QListWidgetItem *item);
        void onPreviewSelectionChanged();
        void onExpandWidthToggled(bool checked);
        void onExpandHeightToggled(bool checked);
        void onRemoveSelected();
        void onResetDefaults();
        void onPreviewEdited();

    private:

        void buildUi();
        void applyChrome();
        void populateFromConfiguration();
        void persistToConfiguration();
        void updateInspector();
        void updatePaletteState();
        fairwindsk::ui::layout::LayoutEntry entryFromPaletteItem(const QListWidgetItem *item) const;
        fairwindsk::ui::layout::LayoutEntry entryForPreviewItem(const QListWidgetItem *item) const;
        void appendPaletteEntryToPreview(const fairwindsk::ui::layout::LayoutEntry &entry);
        void refreshPreviewItem(QListWidgetItem *item) const;
        QListWidgetItem *createPreviewItem(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QListWidgetItem *createPaletteItem(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QListWidgetItem *findPreviewWidgetItem(const QString &widgetId) const;
        QString itemSummary(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QSize itemSizeHint(const fairwindsk::ui::layout::LayoutEntry &entry) const;

        bool m_populating = false;
        Settings *m_settings = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QLabel *m_previewLabel = nullptr;
        QListWidget *m_previewWidget = nullptr;
        QToolButton *m_expandWidthButton = nullptr;
        QToolButton *m_expandHeightButton = nullptr;
        QToolButton *m_removeSelectedButton = nullptr;
        QToolButton *m_resetDefaultsButton = nullptr;
        QLabel *m_paletteLabel = nullptr;
        QListWidget *m_paletteWidget = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP
