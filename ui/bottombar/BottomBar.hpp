//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef BOTTOMBAR_HPP
#define BOTTOMBAR_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include <ui_BottomBar.h>

namespace Ui { class BottomBar; }

namespace fairwindsk::ui::bottombar {





    class BottomBar : public QWidget {
        Q_OBJECT
    public:
        explicit BottomBar(QWidget *parent = 0);

        ~BottomBar() override;

    public
        slots:
        void myData_clicked();
        void mob_clicked();
        void apps_clicked();
        void alarms_clicked();
        void settings_clicked();


        signals:
        void setMyData();
        void setMOB();
        void setApps();
        void setAlarms();
        void setSettings();

    private:
        Ui::BottomBar *ui;
    };
}

#endif //BOTTOMBAR_HPP