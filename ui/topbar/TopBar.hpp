//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef TOPBAR_HPP
#define TOPBAR_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_TopBar.h"

namespace Ui { class TopBar; }

namespace fairwindsk::ui::topbar {


    class TopBar : public QWidget {
    Q_OBJECT
    public:
        explicit TopBar(QWidget *parent = nullptr);

        ~TopBar() override;



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

#endif //TOPBAR_HPP