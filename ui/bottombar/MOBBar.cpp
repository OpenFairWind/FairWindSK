//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MOBBar.h" resolved

#include "MOBBar.hpp"

#include <QJsonDocument>
#include <QTimer>
#include <QDateTime>

#include "FairWindSK.hpp"
#include "ui_MOBBar.h"

namespace fairwindsk::ui::bottombar {
    MOBBar::MOBBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::MOBBar) {
        ui->setupUi(this);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        // Create a new timer which will contain the current time
        m_timer = new QTimer(this);

        // When the timer stops, update the time
        connect(m_timer, &QTimer::timeout, this, &MOBBar::updateElapsed);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->pushButton_MOB_Cancel, &QPushButton::clicked, this, &MOBBar::onCancelClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &MOBBar::onHideClicked);

        // Not visible by default
        QWidget::setVisible(false);

        // Get the configuration json object
        auto configurationJsonObject = configuration->getRoot();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object())
        {
            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("notifications.mob") && m_signalkPaths["notifications.mob"].is_string()) {



                // Subscribe and update
                updateMOB(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["notifications.mob"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::MOBBar::updateMOB)
                        ));
            }
        }
    }

    void MOBBar::MOB() {

        auto signalKClient = FairWindSK::getInstance()->getSignalKClient();

        QString url = signalKClient->url().toString()+"/v1/api/"+signalKClient->getSelf()+"/notifications/mob/value/";


        auto result = signalKClient->signalkGet(QUrl(url.replace(".","/")));

        if (result.isEmpty() || (
            result.contains("state") &&
            result["state"].isString() &&
            result["state"].toString() == "normal")) {

            url = signalKClient->url().toString()+"/v2/api/alarms/mob";

            QJsonObject payload;
            signalKClient->signalkPost(QUrl(url),payload);
        } else {
            setVisible(true);
        }
    }

    void MOBBar::onCancelClicked() {
        auto signalKClient = FairWindSK::getInstance()->getSignalKClient();

        setVisible(false);

        QString url = signalKClient->url().toString()+"/v2/api/alarms/mob";

        QJsonObject payload;
        signalKClient->signalkDelete(QUrl(url),payload);

        emit cancelMOB();
    }

    void MOBBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    MOBBar::~MOBBar() {

        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        delete ui;
    }

    /*
 * updateElapsed
 * Method called to update the current datetime
 */
    void MOBBar::updateElapsed() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("mob.startTime") && m_signalkPaths["mob.startTime"].is_string()) {

            // Get the key
            auto mobStartTime = QString::fromStdString(m_signalkPaths["mob.startTime"].get<std::string>());

            // Get the Signal K client
            auto signalKClient = FairWindSK::getInstance()->getSignalKClient();

            // Get the url
            QString url = signalKClient->url().toString()+"/v1/api/vessels/self/" + mobStartTime.replace(".","/");

            // Get the start time invoking the course api
            auto jsonObjectStartTime = signalKClient->signalkGet(QUrl(url));

            // Get the value as string
            auto startTimeIso8601 = jsonObjectStartTime["value"].toString();

            // Create a QDateTime from the string
            auto startDateTimeUTC = QDateTime::fromString(startTimeIso8601, "yyyy-MM-ddThh:mm:ss.zzzZ");

            // Set the start date time as UTC
            startDateTimeUTC.setOffsetFromUtc(0);

            // Get the current date
            auto currentDateTimeUTC = QDateTime::currentDateTimeUtc();

            // Get the elapsed seconds
            auto elapsedSecs = startDateTimeUTC.secsTo(currentDateTimeUTC);

            // Set the time label from the UI to the formatted time
            ui->label_Elapsed->setText(QDateTime::fromSecsSinceEpoch( elapsedSecs, Qt::UTC ).toString( "hh:mm:ss" ));
        }
    }

    /*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the navigation position
 */
    void MOBBar::updateMOB(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        auto value = fairwindsk::signalk::Client::getObjectFromUpdateByPath(update,"notifications.mob");

        if (value.isEmpty()) {
            return;
        }

        // There is no mob
        if (value.contains("state") && value["state"].isString()) {
            if (value["state"].toString() == "normal") {

                // Check if the Options object has the rsa key and if it is a string
                if (m_signalkPaths.contains("mob.bearing") && m_signalkPaths["mob.bearing"].is_string()) {

                    // Unsubscribe
                    FairWindSK::getInstance()->getSignalKClient()->removeSubscription(
                        QString::fromStdString(m_signalkPaths["mob.bearing"].get<std::string>()),
                        this);
                }

                // Check if the Options object has the rsa key and if it is a string
                if (m_signalkPaths.contains("mob.distance") && m_signalkPaths["mob.distance"].is_string()) {

                    // Unsubscribe
                    FairWindSK::getInstance()->getSignalKClient()->removeSubscription(
                        QString::fromStdString(m_signalkPaths["mob.distance"].get<std::string>()),
                        this);
                }

                // Stop the timer
                m_timer->stop();

                // Hide the MOBBar
                setVisible(false);
            } else if (value["state"].toString() == "emergency")
            {
                if (value.contains("data") && value["data"].isObject()) {
                    auto data = value["data"].toObject();
                    if (data.contains("position") && data["position"].isObject()) {
                        auto position = data["position"].toObject();
                        QGeoCoordinate geoCoordinate;
                        if (position.contains("latitude")) {
                            geoCoordinate.setLatitude(position["latitude"].toDouble());
                        }
                        if (position.contains("longitude")) {
                            geoCoordinate.setLongitude(position["longitude"].toDouble());
                        }
                        if (position.contains("altitude")) {
                            geoCoordinate.setAltitude(position["altitude"].toDouble());
                        }
                        auto text = geoCoordinate.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",");
                        ui->label_Latitude->setText(text[0]);
                        ui->label_Longitude->setText(text[1]);
                    }
                }

                // Check if the Options object has the rsa key and if it is a string
                if (m_signalkPaths.contains("mob.bearing") && m_signalkPaths["mob.bearing"].is_string()) {

                    // Subscribe and update
                    updateBearing(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                            QString::fromStdString(m_signalkPaths["mob.bearing"].get<std::string>()),
                            this,
                            SLOT(fairwindsk::ui::bottombar::MOBBar::updateBearing)
                            ));
                }

                // Check if the Options object has the rsa key and if it is a string
                if (m_signalkPaths.contains("mob.distance") && m_signalkPaths["mob.distance"].is_string()) {

                    // Subscribe and update
                    updateDistance(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                            QString::fromStdString(m_signalkPaths["mob.distance"].get<std::string>()),
                            this,
                            SLOT(fairwindsk::ui::bottombar::MOBBar::updateDistance)
                            ));
                }

                // Start the timer
                m_timer->start(1000);

                // Show the MOBBar
                setVisible(true);
            }
        }
    }

    /*
 * updateBearing
 * Method called in accordance to signalk to update the bearing
 */
    void MOBBar::updateBearing(const QJsonObject &update) {
        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (!std::isnan(value)) {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            auto text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_Bearing->setText(text);
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void MOBBar::updateDistance(const QJsonObject &update) {
        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (!std::isnan(value)) {
            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Distance->setText(text);
        }
    }

} // fairwindsk::ui::bottombar
