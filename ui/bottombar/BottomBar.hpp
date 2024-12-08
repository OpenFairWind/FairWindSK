//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef BOTTOMBAR_HPP
#define BOTTOMBAR_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include <ui_BottomBar.h>
#include "MOBBar.hpp"
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
        void setAutopilotIcon(bool value);

        // Set Anchor Icon visibility
        void setAnchorIcon(bool value);

        // Set MOB Icon visibility
        void setMOBIcon(bool value);

        // Destructor
        ~BottomBar() override;

    public
        slots:

        // MyData button handler
        void myData_clicked();

        // MOB button handler
        void mob_clicked();

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

        signals:

        // Set MyData Widget
        void setMyData();

        // Set the Apps (Home) Widget (Launcher)
        void setApps();

        // Set the Settings Widget
        void setSettings();

    private:
        // Pointer to UI
        Ui::BottomBar *ui = nullptr;

        // Pointer to the alarms bar
        AlarmsBar *m_AlarmsBar = nullptr;

        // Pointer to the MOB bar
        MOBBar *m_MOBBar = nullptr;

        // Pointer to the autopilot bar
        AutopilotBar *m_AutopilotBar = nullptr;

        // Pointer to the anchor bar
        AnchorBar *m_AnchorBar = nullptr;
    };
}

#endif //BOTTOMBAR_HPP