//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef TOPBAR_HPP
#define TOPBAR_HPP

#include <QIcon>
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
        void setCurrentContext(const QString &name,
                               const QString &tooltip = QString(),
                               const QIcon &icon = QIcon(),
                               bool enableButton = false);

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
        void changeEvent(QEvent *event) override;
        void updateDistanceLabels() const;
        void updateSpeedLabels() const;
        void refreshMetricLabelWidths() const;
        void resetCurrentAppPresentation() const;

        Ui::TopBar *ui;
        AppItem *m_currentApp = nullptr;
        Units *m_units = nullptr;
        QTimer *m_timer = nullptr;
        QString m_pathCOG;
        QString m_pathSOG;
        QString m_pathHDG;
        QString m_pathSTW;
        QString m_pathDPT;
        QString m_pathBTW;
        QString m_pathDTG;
        QString m_pathXTE;
        QString m_pathVMG;
    };
}

#endif //TOPBAR_HPP
