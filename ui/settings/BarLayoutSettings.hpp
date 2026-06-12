#ifndef FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
#define FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP

#include <QListWidget>
#include <QToolButton>
#include <QWidget>

#include "LayoutPreviewListWidget.hpp"
#include "Settings.hpp"
#include "WidgetPalette.hpp"
#include "ui/layout/BarLayout.hpp"

class QLabel;
class QListWidgetItem;
class QFrame;
class QEvent;
class QTimer;

namespace fairwindsk::ui::widgets {
    class TouchCheckBox;
}

namespace fairwindsk::ui::settings {
    class BarLayoutSettings : public QWidget {
        Q_OBJECT

    public:
        explicit BarLayoutSettings(Settings *settings,
                                   fairwindsk::ui::layout::BarId barId,
                                   QWidget *parent = nullptr);
        ~BarLayoutSettings() override = default;

        void refreshFromConfiguration();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;

    private slots:
        void onItemChanged(QListWidgetItem *item);
        void onSelectionChanged();
        void onPaletteEntryActivated(const fairwindsk::ui::layout::LayoutEntry &entry);
        void onMinimumWidthSelected();
        void onMaximumWidthSelected();
        void onMinimumHeightSelected();
        void onMaximumHeightSelected();
        void onMoveSelectedLeft();
        void onMoveSelectedRight();
        void onRemoveSelected();
        void onResetDefaults();
        void onPreviewEdited();
        void onDisplayOptionChanged();

    private:
        void buildUi();
        void applyChrome();
        void populateFromConfiguration();
        void persistToConfiguration();
        fairwindsk::ui::layout::LayoutEntry entryForItem(const QListWidgetItem *item) const;
        QListWidgetItem *createItem(const fairwindsk::ui::layout::LayoutEntry &entry);
        void refreshPreviewItem(QListWidgetItem *item) const;
        void refreshPreviewItems() const;
        void refreshPaletteEntries(const QList<fairwindsk::ui::layout::LayoutEntry> &activeEntries = {});
        int minimumItemWidth(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        QSize itemSizeHint(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        void updateActions();
        void updateDisplayOptionControls();
        bool isDataWidgetEntry(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        void scheduleRuntimeReconfigure();
        void appendPlaceholder(fairwindsk::ui::layout::EntryKind kind);
        void activateWidgetEntry(const QString &widgetId);
        void moveCurrentItem(int offset);
        bool defaultExpandHorizontally(const QString &widgetId) const;
        bool m_populating = false;
        Settings *m_settings = nullptr;
        fairwindsk::ui::layout::BarId m_barId = fairwindsk::ui::layout::BarId::Top;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QLabel *m_previewLabel = nullptr;
        QFrame *m_previewFrame = nullptr;
        LayoutPreviewListWidget *m_listWidget = nullptr;
        QToolButton *m_minimumWidthButton = nullptr;
        QToolButton *m_maximumWidthButton = nullptr;
        QToolButton *m_minimumHeightButton = nullptr;
        QToolButton *m_expandHeightButton = nullptr;
        QToolButton *m_moveLeftButton = nullptr;
        QToolButton *m_moveRightButton = nullptr;
        QToolButton *m_removeSelectedButton = nullptr;
        QToolButton *m_resetDefaultsButton = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showIconCheckBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showTextCheckBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showUnitsCheckBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showTrendCheckBox = nullptr;
        QLabel *m_paletteLabel = nullptr;
        WidgetPalette *m_paletteWidget = nullptr;
        QTimer *m_reconfigureTimer = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_BARLAYOUTSETTINGS_HPP
