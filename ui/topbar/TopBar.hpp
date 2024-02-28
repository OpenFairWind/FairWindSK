//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef FAIRWINDSK_TOPBAR_HPP
#define FAIRWINDSK_TOPBAR_HPP

#include <QWidget>
#include <FairWindSK.hpp>

namespace Ui {
    class TopBar;
}

namespace fairwindsk::ui::topbar {
    class TopBar : public QWidget {
    Q_OBJECT
    public:
        explicit TopBar(QWidget *parent = 0);

        ~TopBar();



    public slots:
        void toolbuttonUL_clicked();
        void toolbuttonUR_clicked();

    public slots:

        void updateNavigationPosition(const QJsonObject& update);
        void updateNavigationCourseOverGroundTrue(const QJsonObject& update);
        void updateNavigationSpeedOverGround(const QJsonObject& update);

        void updateTime();

        signals:
        void clickedToolbuttonUL();
        void clickedToolbuttonUR();

    private:
        Ui::TopBar *ui;

    };
}

#endif //FAIRWIND_TOPBAR_HPP