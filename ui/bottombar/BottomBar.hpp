//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef FAIRWINDSK_BOTTOMBAR_HPP
#define FAIRWINDSK_BOTTOMBAR_HPP

#include <QWidget>

namespace Ui {
    class BottomBar;
}

namespace fairwindsk::ui::bottombar {
    class BottomBar : public QWidget {
        Q_OBJECT
    public:
        explicit BottomBar(QWidget *parent = 0);

        ~BottomBar();

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

#endif //FAIRWINDSK_BOTTOMBAR_HPP