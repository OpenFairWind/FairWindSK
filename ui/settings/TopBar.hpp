#ifndef FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP
#define FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP

#include <QListWidget>
#include <QToolButton>
#include <QWidget>

#include "Settings.hpp"
#include "WidgetPalette.hpp"
#include "ui/layout/BarLayout.hpp"

class QLabel;
class QListWidgetItem;
class QFrame;
class QEvent;

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
        void onPaletteEntryActivated(const fairwindsk::ui::layout::LayoutEntry &entry);
        void onPreviewSelectionChanged();
        void onMinimumWidthSelected();
        void onMaximumWidthSelected();
        void onMinimumHeightSelected();
        void onMaximumHeightSelected();
        void onMoveSelectedLeft();
        void onMoveSelectedRight();
        void onRemoveSelected();
        void onResetDefaults();
        void onPreviewEdited();

    private:

        void buildUi();
        void applyChrome();
        bool eventFilter(QObject *watched, QEvent *event) override;
        void populateFromConfiguration();
        void persistToConfiguration();
        void updateInspector();
        void updatePaletteState();
        void moveCurrentPreviewItem(int offset);
        void refreshPreviewItems() const;
        fairwindsk::ui::layout::LayoutEntry entryForPreviewItem(const QListWidgetItem *item) const;
        void appendPaletteEntryToPreview(const fairwindsk::ui::layout::LayoutEntry &entry);
        void refreshPreviewItem(QListWidgetItem *item) const;
        QListWidgetItem *createPreviewItem(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QListWidgetItem *findPreviewWidgetItem(const QString &widgetId) const;
        QString itemSummary(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        int minimumItemWidth(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QSize itemSizeHint(const fairwindsk::ui::layout::LayoutEntry &entry) const;

        bool m_populating = false;
        Settings *m_settings = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QLabel *m_previewLabel = nullptr;
        QFrame *m_previewFrame = nullptr;
        QToolButton *m_leftShellButton = nullptr;
        QToolButton *m_rightShellButton = nullptr;
        QListWidget *m_previewWidget = nullptr;
        QToolButton *m_minimumWidthButton = nullptr;
        QToolButton *m_maximumWidthButton = nullptr;
        QToolButton *m_minimumHeightButton = nullptr;
        QToolButton *m_expandHeightButton = nullptr;
        QToolButton *m_moveLeftButton = nullptr;
        QToolButton *m_moveRightButton = nullptr;
        QToolButton *m_removeSelectedButton = nullptr;
        QToolButton *m_resetDefaultsButton = nullptr;
        QLabel *m_paletteLabel = nullptr;
        WidgetPalette *m_paletteWidget = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_TOPBAR_HPP
