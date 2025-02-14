//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef BOTTOMBAR_HPP
#define BOTTOMBAR_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include <ui_BottomBar.h>
#include "POBBar.hpp"
#include "AlarmsBar.hpp"
#include "AutopilotBar.hpp"
#include "AnchorBar.hpp"

namespace Ui { class BottomBar; }

namespace fairwindsk::ui::bottombar {

    class BottomBar : public QWidget {
        Q_OBJECT
    public:
        // Constructor
        explicit BottomBar(QWidget *parent = 0);

        // Set Autopilot Icon visibility
        void setAutopilotIcon(bool value) const;

        // Set Anchor Icon visibility
        void setAnchorIcon(bool value) const;

        // Set POB Icon visibility
        void setPOBIcon(bool value) const;

        // Add application icon to the shortcut
        void addApp(const QString& name);

        // Remove application icon to the shortcut
        void removeApp(const QString& name);

        // Destructor
        ~BottomBar() override;

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

        int m_iconSize;
        QMap<QString, QToolButton *> m_buttons;
    };
}

#endif //BOTTOMBAR_HPP