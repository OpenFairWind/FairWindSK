//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AlarrmsBar.h" resolved

#include <QPushButton>
#include <QJsonObject>

#include "AlarmsBar.hpp"

#include "FairWindSK.hpp"
#include "ui_AlarmsBar.h"

namespace fairwindsk::ui::bottombar {

    AlarmsBar::AlarmsBar(QWidget *parent) : QWidget(parent), ui(new Ui::AlarmsBar) {

        ui->setupUi(this);

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Get the configuration json object

        // Check if the configuration object contains the key 'signalk' with an object value
        if (auto configurationJsonObject = configuration->getRoot(); configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];
        }


        // emit a signal when MOB alarm is raised
        connect(ui->toolButton_MOB, &QPushButton::clicked, this, &AlarmsBar::onMobClicked);
        connect(ui->toolButton_Sinking, &QPushButton::clicked, this, &AlarmsBar::onSinkingClicked);
        connect(ui->toolButton_Fire, &QPushButton::clicked, this, &AlarmsBar::onFireClicked);
        connect(ui->toolButton_Piracy, &QPushButton::clicked, this, &AlarmsBar::onPiracyClicked);
        connect(ui->toolButton_Abandon, &QPushButton::clicked, this, &AlarmsBar::onAbandonClicked);
        connect(ui->toolButton_Adrift, &QPushButton::clicked, this, &AlarmsBar::onAdriftClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AlarmsBar::onHideClicked);

        m_alarmToolButtons["abandon"] = ui->toolButton_Abandon;
        m_alarmToolButtons["adrift"] = ui->toolButton_Adrift;
        m_alarmToolButtons["fire"] = ui->toolButton_Fire;
        m_alarmToolButtons["mob"] = ui->toolButton_MOB;
        m_alarmToolButtons["piracy"] = ui->toolButton_Piracy;
        m_alarmToolButtons["sinking"] = ui->toolButton_Sinking;

        // Not visible by default
        QWidget::setVisible(false);
    }

    void AlarmsBar::onHideClicked() {
        setVisible(false);
        emit hidden();
    }

    void AlarmsBar::onAbandonClicked() { onAlarm("abandon"); }
    void AlarmsBar::onAdriftClicked() { onAlarm("adrift"); }
    void AlarmsBar::onFireClicked() { onAlarm("fire"); }
    void AlarmsBar::onMobClicked() { onAlarm("mob"); }
    void AlarmsBar::onPiracyClicked() { onAlarm("piracy"); }
    void AlarmsBar::onSinkingClicked() { onAlarm("sinking"); }


    void AlarmsBar::onAlarm(const QString& alarm) {

        // Create the key
        QString key = "notifications." + alarm;

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains(key.toStdString()) && m_signalkPaths[key.toStdString()].is_string()) {

            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            // Get the mob notification value
            auto notificationObject = signalKClient->signalkGet(
                QString::fromStdString("vessels.self." + m_signalkPaths[key.toStdString()].get<std::string>()+".value")
                );

            // Check if the mob notification value is empty or the state is normal (no mob emergency)
            if (notificationObject.isEmpty() || (
                notificationObject.contains("state") &&
                notificationObject["state"].isString() &&
                notificationObject["state"].toString() == "normal")) {

                // Rise the alarm
                signalKClient->signalkPost(QUrl(signalKClient->url().toString()+"/v2/api/alarms/"+alarm));

                // Set activated icon
                m_alarmToolButtons[alarm]->setChecked(true);

                // Emit the signal
                emit alarmed(alarm, true);

                } else {
                    // Delete the alarm
                    signalKClient->signalkDelete(QUrl(signalKClient->url().toString()+"/v2/api/alarms/"+alarm));

                    // Set regular icon
                    m_alarmToolButtons[alarm]->setChecked(false);

                    // Emit the signal
                    emit alarmed(alarm, false);
                }
        }
    }


    AlarmsBar::~AlarmsBar() {
        if (ui) {

            delete ui;

            ui = nullptr;
        }
    }
} // fairwindsk::ui::bottombar
