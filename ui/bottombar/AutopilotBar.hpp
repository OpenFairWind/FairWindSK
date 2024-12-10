//
// Created by Raffaele Montella on 04/06/24.
//

#ifndef FAIRWINDSK_AUTOPILOTBAR_HPP
#define FAIRWINDSK_AUTOPILOTBAR_HPP

#include <QWidget>
#include <QSlider>
#include "Units.hpp"

namespace Ui { class AutopilotBar; }

namespace fairwindsk::ui::bottombar {

    // Based on https://github.com/SignalK/signalk-autopilot/ APIs

    class AutopilotBar : public QWidget {
    Q_OBJECT

    public:
        explicit AutopilotBar(QWidget *parent = nullptr);

        ~AutopilotBar() override;

    public slots:

        void updateRSA(const QJsonObject& update);
        void updateState(const QJsonObject& update);
        void updateMode(const QJsonObject& update);
        void updateTargetHeading(const QJsonObject& update);

        void onStandByClicked();
        void onAutoClicked();
        void onWindClicked();
        void onRouteClicked();
        void onPortTackClicked();
        void onStarboardTackClicked();
        void onPlus1Clicked();
        void onPlus10Clicked();
        void onMinus1Clicked();
        void onMinus10Clicked();
        void onHideClicked();
        void onNextWPTClicked();
        void onSteerClicked();

    signals:
        void hide();

    private:
        QJsonObject setState(const QString& state);
        QJsonObject setMode(const QString& mode);
        QJsonObject setTargetHeading(int value);
        QJsonObject setTargetWindAngle(float value);
        QJsonObject adjustHeading(int value);
        QJsonObject tack(const QString& value);
        QJsonObject advanceWaypoint(int value);

        Ui::AutopilotBar *ui = nullptr;
        QSlider *m_slider = nullptr;
        Units *m_units = nullptr;
        nlohmann::json m_signalkPaths;

        //QGamepad *m_gamepad;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_AUTOPILOTBAR_HPP
