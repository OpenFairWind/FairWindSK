//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_ALARMSBAR_HPP
#define FAIRWINDSK_ALARMSBAR_HPP

#include <QWidget>
#include <QToolButton>
#include <QEvent>
#include <QDateTime>

#include "signalk/Client.hpp"

namespace Ui { class AlarmsBar; }

namespace fairwindsk::ui::bottombar {

    class AlarmsBar : public QWidget {
    Q_OBJECT

    public:
        explicit AlarmsBar(QWidget *parent = nullptr);

        ~AlarmsBar() override;
        void refreshFromConfiguration();

    public
        slots:
        void onHideClicked();

        void onPobClicked();
        void onFireClicked();
        void onAbandonClicked();
        void onAdriftClicked();
        void onPiracyClicked();
        void onSinkingClicked();

        void updateNotifications(const QJsonObject& update);

    signals:
        void hidden();
        void alarmed(QString alarm, bool status);

    private:
        void changeEvent(QEvent *event) override;
        void onAlarm(const QString& alarm);
        QString alarmApiKey(const QString &alarm) const;
        QString alarmUiKey(const QString &apiKey) const;
        void setAlarmState(const QString &apiKey, bool active);
        void applyComfortStyle() const;
        void updateControlTooltips();


    private:
        Ui::AlarmsBar *ui = nullptr;
        QMap<QString, QToolButton*> m_alarmToolButtons;
        fairwindsk::signalk::Client::ConnectionHealthState m_connectionState =
            fairwindsk::signalk::Client::ConnectionHealthState::Disconnected;
        QString m_connectionStatusText;
        QDateTime m_lastStreamUpdate;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_ALARMSBAR_HPP
