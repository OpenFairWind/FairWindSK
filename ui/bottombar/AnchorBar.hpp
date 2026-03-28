//
// Created by Raffaele Montella on 08/12/24.
//

#ifndef ANCHORBAR_H
#define ANCHORBAR_H

#include <QWidget>
#include <QJsonObject>
#include <nlohmann/json.hpp>
#include "Units.hpp"

namespace Ui { class AnchorBar; }

namespace fairwindsk::ui::bottombar {

    class AnchorBar : public QWidget {
        Q_OBJECT

        public:
        explicit AnchorBar(QWidget *parent = nullptr);

        ~AnchorBar() override;
        void refreshFromConfiguration();

        public
            slots:
            void onHideClicked();

            void onResetClicked();
            void onUpPressed();
        void onUpReleased();
            void onRaiseClicked();
        void onSetRadiusClicked();
            void onCurrentRadiusChanged();
            void onDropClicked();
            void onDownPressed();
        void onDownReleased();
            void onReleaseClicked();

        void updatePosition(const QJsonObject& update);
        void updateDepth(const QJsonObject& update);
        void updateBearing(const QJsonObject& update);
        void updateDistance(const QJsonObject& update);
        void updateRode(const QJsonObject& update);
        void updateCurrentRadius(const QJsonObject& update);
        void updateMaxRadius(const QJsonObject& update);

        signals:
            void hidden();

        void resetCounter();
        void chainUpPressed();
        void chainUpReleased();
        void raiseAnchor();
        void radiusChanged();
        void radiusSet();
        void dropAnchor();
        void chainDownPressed();
        void chainDownReleased();
        void chainRelease();


    private:
        void updateUnitLabels() const;

        Ui::AnchorBar *ui;
        Units *m_units;
        nlohmann::json m_signalkPaths;
        QJsonObject m_lastPositionUpdate;
        QJsonObject m_lastDepthUpdate;
        QJsonObject m_lastBearingUpdate;
        QJsonObject m_lastDistanceUpdate;
        QJsonObject m_lastRodeUpdate;
        QJsonObject m_lastCurrentRadiusUpdate;
        QJsonObject m_lastMaxRadiusUpdate;
    };
} // fairwindsk::ui::bottombar


#endif //ANCHORBAR_H
