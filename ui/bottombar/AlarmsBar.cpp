//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AlarrmsBar.h" resolved

#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

#include "AlarmsBar.hpp"

#include "FairWindSK.hpp"
#include "ui_AlarmsBar.h"

namespace fairwindsk::ui::bottombar {

    AlarmsBar::AlarmsBar(QWidget *parent) : QWidget(parent), ui(new Ui::AlarmsBar) {

        ui->setupUi(this);

        m_alarmToolButtons["abandon"] = ui->toolButton_Abandon;
        m_alarmToolButtons["adrift"] = ui->toolButton_Adrift;
        m_alarmToolButtons["fire"] = ui->toolButton_Fire;
        m_alarmToolButtons["pob"] = ui->toolButton_POB;
        m_alarmToolButtons["piracy"] = ui->toolButton_Piracy;
        m_alarmToolButtons["sinking"] = ui->toolButton_Sinking;

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Get the configuration json object

        // Check if the configuration object contains the key 'signalk' with an object value
        if (auto configurationJsonObject = configuration->getRoot(); configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("notifications") && m_signalkPaths["notifications"].is_string()) {

                // Subscribe and update
                updateNotifications(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["notifications"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateNotifications)
                        ));
            }
        }


        // emit a signal when MOB alarm is raised
        connect(ui->toolButton_POB, &QPushButton::clicked, this, &AlarmsBar::onPobClicked);
        connect(ui->toolButton_Sinking, &QPushButton::clicked, this, &AlarmsBar::onSinkingClicked);
        connect(ui->toolButton_Fire, &QPushButton::clicked, this, &AlarmsBar::onFireClicked);
        connect(ui->toolButton_Piracy, &QPushButton::clicked, this, &AlarmsBar::onPiracyClicked);
        connect(ui->toolButton_Abandon, &QPushButton::clicked, this, &AlarmsBar::onAbandonClicked);
        connect(ui->toolButton_Adrift, &QPushButton::clicked, this, &AlarmsBar::onAdriftClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AlarmsBar::onHideClicked);



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
    void AlarmsBar::onPobClicked() { onAlarm("pob"); }
    void AlarmsBar::onPiracyClicked() { onAlarm("piracy"); }
    void AlarmsBar::onSinkingClicked() { onAlarm("sinking"); }


    void AlarmsBar::onAlarm(const QString& alarm) {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Create the key
        const QString key = "notifications." + alarm;

        // Check if the debug is on
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "AlarmsBar::onAlarm(" << alarm << ") --> " << key;
        }

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains(key.toStdString()) && m_signalkPaths[key.toStdString()].is_string()) {

            // Get the alarm key
            const auto alarmKey = QString::fromStdString(
                m_signalkPaths[key.toStdString()].get<std::string>()
                ).replace("notifications.","");

            // Set the path
            const auto getPath = "vessels.self." + m_signalkPaths[key.toStdString()].get<std::string>()+".value";

            // Check if the debug is on
            if (fairWindSK->isDebug()) {

                // Write a message
                qDebug() << "getPath: " << getPath;
            }

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            // Get the mob notification value

            // Check if the mob notification value is empty or the state is normal (no mob emergency)
            if (auto notificationObject = signalKClient->signalkGet( QString::fromStdString(getPath)); notificationObject.isEmpty() || (
                notificationObject.contains("state") &&
                notificationObject["state"].isString() &&
                notificationObject["state"].toString() == "normal")) {

                const auto postPath = signalKClient->url().toString()+"/v2/api/alarms/" + alarmKey;

                // Check if the debug is on
                if (fairWindSK->isDebug()) {
                    // Write a message
                    qDebug() << "postPath: " << postPath;
                }

                // Rise the alarm
                signalKClient->signalkPost(QUrl(postPath));

                // Set activated icon
                m_alarmToolButtons[alarm]->setChecked(true);

                // Emit the signal
                emit alarmed(alarm, true);

                } else {
                    // Delete the alarm
                    signalKClient->signalkDelete(QUrl(signalKClient->url().toString()+"/v2/api/alarms/"+alarmKey));

                    // Set regular icon
                    m_alarmToolButtons[alarm]->setChecked(false);

                    // Emit the signal
                    emit alarmed(alarm, false);
                }
        }
    }


    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AlarmsBar::updateNotifications(const QJsonObject &update) {

        qDebug() << "AlarmsBar::updateNotifications";

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Check if the update has at list one update
        if (update.contains("updates") && update["updates"].isArray() ) {

            // Get the keys of the alarm tool buttons
            const auto keys = m_alarmToolButtons.keys();

            qDebug() << "AlarmsBar::updateNotifications: updates";
            qDebug() << update["updates"];

            // For each update...
            for (const auto& updateItem : update["updates"].toArray()) {

                // Check if the update item is an object
                if (updateItem.isObject()) {

                    // Get the update item as object
                    auto updateItemObject = updateItem.toObject();

                    // Check if the update item contains values as array
                    if (updateItemObject.contains("values") && updateItemObject["values"].isArray()) {

                        // For each value of the array
                        for (const auto& value : updateItemObject["values"].toArray()) {

                            // Check if the value is an object
                            if (value.isObject()) {

                                // Get the value as an object
                                auto valueObject = value.toObject();

                                // Check the value contains path as string
                                if (valueObject.contains("path") && valueObject["path"].isString()) {

                                    // Get the path as string
                                    auto path = valueObject["path"].toString();

                                    // Separate the path elements
                                    auto pathParts = path.split('.');

                                    // Remove the first element (is always "notifications")
                                    pathParts.removeFirst();

                                    qDebug() << "AlarmsBar::updateNotifications: pathParts";
                                    qDebug() << pathParts;

                                    // Check if one of the keys is the first element of the path
                                    if (pathParts.size() == 1 && keys.contains(pathParts[0])) {

                                        qDebug() << "AlarmsBar::updateNotifications: pathParts";

                                        // By default, set emergency as false (no icon to be shown)
                                        bool emergency = false;

                                        // Check the value object contains a notification value that have to be an object
                                        if (valueObject.contains("value") && valueObject["value"].isObject()) {

                                            // Get the notification value as object
                                            auto notificationValue = valueObject["value"].toObject();

                                            // Check if the notification value contains state as string
                                            if (notificationValue.contains("state") && notificationValue["state"].isString()) {

                                                // Get the state as string
                                                auto state = notificationValue["state"].toString();

                                                // Check if the string is emergency
                                                if (state == "emergency") {

                                                    // Set the emergency flag to true
                                                    emergency = true;
                                                }
                                            }
                                        }

                                        qDebug() << "AlarmsBar::updateNotifications: " << pathParts[0] << " -> " << emergency;

                                        // Set regular icon
                                        m_alarmToolButtons[pathParts[0]]->setChecked(emergency);

                                        // Emit the signal
                                        emit alarmed(pathParts[0], emergency);

                                    }
                                }
                            }
                        }
                    }
                }
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
