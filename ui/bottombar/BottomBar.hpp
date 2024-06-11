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

namespace Ui { class BottomBar; }

namespace fairwindsk::ui::bottombar {

    class BottomBar : public QWidget {
        Q_OBJECT
    public:
        explicit BottomBar(QWidget *parent = 0);

        void setAutopilotIcon(bool value);
        void setAnchorIcon(bool value);

        ~BottomBar() override;

    public
        slots:
        void myData_clicked();
        void mob_clicked();
        void autopilot_clicked();
        void apps_clicked();
        void anchor_clicked();
        void alarms_clicked();
        void settings_clicked();

        signals:
        void setMyData();
        void setApps();
        void setSettings();

    private:
        Ui::BottomBar *ui;

        AlarmsBar *m_AlarmsBar;
        MOBBar *m_MOBBar;
        AutopilotBar *m_AutopilotBar;
    };
}

#endif //BOTTOMBAR_HPP