//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_ALARMSBAR_HPP
#define FAIRWINDSK_ALARMSBAR_HPP

#include <QWidget>
#include <QToolButton>
#include <nlohmann/json.hpp>

namespace Ui { class AlarmsBar; }

namespace fairwindsk::ui::bottombar {

    class AlarmsBar : public QWidget {
    Q_OBJECT

    public:
        explicit AlarmsBar(QWidget *parent = nullptr);

        ~AlarmsBar() override;

    public
        slots:
        void onHideClicked();

        void onMobClicked();
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
        void onAlarm(const QString& alarm);


    private:
        Ui::AlarmsBar *ui;
        QMap<QString, QToolButton*> m_alarmToolButtons;
        nlohmann::json m_signalkPaths;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_ALARMSBAR_HPP
