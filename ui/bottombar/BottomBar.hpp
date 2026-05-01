//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef BOTTOMBAR_HPP
#define BOTTOMBAR_HPP

#include <QHash>
#include <QHBoxLayout>
#include <QPointer>
#include <QVector>
#include <QWidget>

#include <FairWindSK.hpp>
#include <ui_BottomBar.h>
#include "POBBar.hpp"
#include "AlarmsBar.hpp"
#include "AutopilotBar.hpp"
#include "AnchorBar.hpp"
#include "ui/layout/BarLayout.hpp"
#include "ui/widgets/SignalKServerBox.hpp"

namespace Ui { class BottomBar; }
class QGraphicsEffect;

namespace fairwindsk::ui::bottombar {

    class BottomBar : public QWidget {
        Q_OBJECT
    public:
        // Constructor
        explicit BottomBar(QWidget *parent = 0);
        static BottomBar *instance();

        // Set Autopilot Icon visibility
        void setAutopilotIcon(bool value) const;

        // Set Anchor Icon visibility
        void setAnchorIcon(bool value) const;

        // Set POB Icon visibility
        void setPOBIcon(bool value) const;
        void refreshFromConfiguration() const;
        void setLayoutEditHighlightEnabled(bool enabled);
        QWidget *widgetForItemId(const QString &itemId) const;
        bool isTransientPanelVisible() const;

        // Add application icon to the shortcut
        void addApp(const QString& name);

        // Remove application icon to the shortcut
        void removeApp(const QString& name);

        // Destructor
        ~BottomBar() override;

    protected:
        void changeEvent(QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;

    public
        slots:

        // MyData button handler
        void myData_clicked();

        // POB button handler
        void pob_clicked();

        // Autopilot button handler
        void autopilot_clicked();

        // Apps (Home) button handler
        void apps_clicked();

        // Anchor button handler
        void anchor_clicked();

        // Alarms button handler
        void alarms_clicked();

        // Settings button handler
        void settings_clicked();

        // An app shortcut has been selected
        void app_clicked();

        signals:

        // Set MyData Widget
        void setMyData();

        // Set the Apps (Home) Widget (Launcher)
        void setApps();

        // Set the Settings Widget
        void setSettings();

        // Change the app
        void foregroundAppChanged(QString name);

    private:
        void applyNavigationButtonIcons() const;
        void rebalanceNavigationBlock() const;
        void setRegularBarVisible(bool visible) const;
        void restoreRegularBarVisibility() const;
        void setPanelVisibility(QWidget *panel, bool visible) const;
        void hideTransientPanels(QWidget *except = nullptr) const;
        void updateTransientPanelHeight(QWidget *panel) const;
        void updateHealthChrome();
        void rebuildLayout();
        QWidget *createSeparatorWidget();
        void clearConfiguredLayout();
        void applyEntrySizing(const fairwindsk::ui::layout::LayoutEntry &entry,
                              const QString &itemId,
                              QWidget *widget,
                              QHBoxLayout *layout);
        void clearLayoutEditHints();
        void applyLayoutEditHints(const QList<fairwindsk::ui::layout::LayoutEntry> &entries);

    private slots:
        void onRuntimeHealthChanged(fairwindsk::FairWindSK::RuntimeHealthState state,
                                    const QString &summary,
                                    const QString &badgeText);

    private:
        // Pointer to UI
        Ui::BottomBar *ui = nullptr;

        // Pointer to the alarms bar
        AlarmsBar *m_AlarmsBar = nullptr;

        // Pointer to the POB bar
        POBBar *m_POBBar = nullptr;

        // Pointer to the autopilot bar
        AutopilotBar *m_AutopilotBar = nullptr;

        // Pointer to the anchor bar
        AnchorBar *m_AnchorBar = nullptr;

        widgets::SignalKServerBox *m_signalKServerBox = nullptr;

        int m_iconSize;
        QMap<QString, QToolButton *> m_buttons;
        fairwindsk::FairWindSK::RuntimeHealthState m_runtimeHealthState = fairwindsk::FairWindSK::RuntimeHealthState::Disconnected;
        QString m_runtimeHealthSummary;
        QHash<QString, QSizePolicy> m_baseSizePolicies;
        QHash<QWidget *, QPointer<QGraphicsEffect>> m_layoutHintEffects;
        bool m_layoutEditHighlightEnabled = false;
        QVector<QPointer<QWidget>> m_dynamicLayoutWidgets;
        inline static BottomBar *s_instance = nullptr;
    };
}

#endif //BOTTOMBAR_HPP
