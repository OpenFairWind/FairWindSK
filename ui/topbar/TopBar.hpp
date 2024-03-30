//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef TOPBAR_HPP
#define TOPBAR_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_TopBar.h"
#include "Units.hpp"

namespace Ui { class TopBar; }

namespace fairwindsk::ui::topbar {


    class TopBar : public QWidget {
    Q_OBJECT
    public:
        explicit TopBar(QWidget *parent = nullptr);

        ~TopBar() override;

        void setCurrentApp(AppItem *appItem);

    public slots:
        void toolbuttonUL_clicked();
        void toolbuttonUR_clicked();

    public slots:

        void updatePOS(const QJsonObject& update);
        void updateCOG(const QJsonObject& update);
        void updateSOG(const QJsonObject& update);
        void updateHDG(const QJsonObject& update);
        void updateSTW(const QJsonObject& update);
        void updateDPT(const QJsonObject& update);
        void updateWPT(const QJsonObject& update);
        void updateBTW(const QJsonObject& update);
        void updateDTG(const QJsonObject& update);
        void updateTTG(const QJsonObject& update);
        void updateETA(const QJsonObject& update);
        void updateXTE(const QJsonObject& update);
        void updateVMG(const QJsonObject& update);

        void updateTime();

        signals:
        void clickedToolbuttonUL();
        void clickedToolbuttonUR();

    private:
        Ui::TopBar *ui;
        AppItem *m_currentApp;
        Units *m_units;
    };
}

#endif //TOPBAR_HPP