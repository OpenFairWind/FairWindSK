//
// Created by Raffaele Montella on 04/06/24.
//

#ifndef FAIRWINDSK_AUTOPILOTBAR_HPP
#define FAIRWINDSK_AUTOPILOTBAR_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <QWidget>
#include <QEvent>
#include <QJsonObject>
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
        void refreshFromConfiguration();

    public slots:

        void updateRSA(const QJsonObject& update);
        void updateState(const QJsonObject& update);
        void updateTargetHeading(const QJsonObject& update);

        void onStandByClicked();
        void onAutoClicked();
        void onWindClicked();
        void onRouteClicked();
        void onPortTackClicked();
        void onStarboardTackClicked();
        void onPortGybeClicked();
        void onStarboardGybeClicked();
        void onPlus1Clicked();
        void onPlus10Clicked();
        void onMinus1Clicked();
        void onMinus10Clicked();
        void onHideClicked();
        void onNextWPTClicked();
        void onDodgeClicked();

    signals:
        void hidden();

    private:
        void changeEvent(QEvent *event) override;
        void applyComfortStyle() const;
        QString autopilotBasePath() const;
        QUrl autopilotUrl(const QString &suffix = QString()) const;
        void refreshAutopilotOptions();
        void setAutopilotControlsEnabled(bool enabled);
        QJsonObject setMode(const QString& state);
        QJsonObject setTargetHeading(float value);
        QJsonObject setTargetWindAngle(float value);
        QJsonObject adjustHeading(float value);
        QJsonObject tack(const QString& value);
        QJsonObject gybe(const QString& value);
        QJsonObject advanceWaypoint(int value);
        void checkStateAndUpdateUI(QJsonObject result);

        Ui::AutopilotBar *ui = nullptr;
        QSlider *m_slider = nullptr;
        Units *m_units = nullptr;
        nlohmann::json m_signalkPaths;
        bool m_autopilotAvailable = false;
        QJsonObject m_lastRsaUpdate;
        QJsonObject m_lastStateUpdate;
        QJsonObject m_lastTargetHeadingUpdate;

        //QGamepad *m_gamepad;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_AUTOPILOTBAR_HPP
