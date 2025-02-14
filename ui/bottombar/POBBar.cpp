//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_POBBar.h" resolved

#include "POBBar.hpp"

#include <QJsonDocument>
#include <QTimer>
#include <QDateTime>

#include "FairWindSK.hpp"
#include "ui_POBBar.h"

namespace fairwindsk::ui::bottombar {
    POBBar::POBBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::POBBar) {
        ui->setupUi(this);

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        // Create a new timer which will contain the current time
        m_timer = new QTimer(this);

        // When the timer stops, update the time
        connect(m_timer, &QTimer::timeout, this, &POBBar::updateElapsed);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->pushButton_POB_Cancel, &QPushButton::clicked, this, &POBBar::onCancelClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &POBBar::onHideClicked);

        // Not visible by default
        QWidget::setVisible(false);

        // Get the configuration json object
        auto configurationJsonObject = configuration->getRoot();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("notifications.pob") && m_signalkPaths["notifications.pob"].is_string()) {

                // Subscribe and update
                updatePOB(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["notifications.pob"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::POBBar::updatePOB)
                        ));
            }
        }
    }

    /*
     * POB()
     * Cancel or set POB emergency
     */
    void POBBar::POB() {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (auto configurationJsonObject = configuration->getRoot(); configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {
            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("notifications.pob") && m_signalkPaths["notifications.pob"].is_string()) {

                // Get the alarm key
                const auto alarmKey = QString::fromStdString(
                    m_signalkPaths["notifications.pob"].get<std::string>()
                    ).replace("notifications.","");

                // Get th getPath
                const auto getPath = QString::fromStdString(
                    "vessels.self." + m_signalkPaths["notifications.pob"].get<std::string>()+".value"
                    );

                // Get the Signal K client
                const auto signalKClient = fairWindSK->getSignalKClient();

                // Check if the pob notification value is empty or the state is normal (no pob emergency)
                if (const auto notificationObject = signalKClient->signalkGet(getPath); notificationObject.isEmpty() || (
                    notificationObject.contains("state") &&
                    notificationObject["state"].isString() &&
                    notificationObject["state"].toString() == "normal")) {

                    const auto postPath = signalKClient->url().toString()+"/v2/api/alarms/" + alarmKey;

                    // Rise the alarm
                    signalKClient->signalkPost(QUrl(postPath));

                } else {
                    // Just set visible the widget
                    setVisible(true);
                }
            }
        }
    }

    /*
     * onCancelClicked
     * Cancel the POB emergency
     */
    void POBBar::onCancelClicked() {

        // Get the Signal K client
        const auto signalKClient = FairWindSK::getInstance()->getSignalKClient();

        // Get the alarm key
        const auto alarmKey = QString::fromStdString(
            m_signalkPaths["notifications.pob"].get<std::string>()
            ).replace("notifications.","");

        // Prepare the API URL
        const QString url = signalKClient->url().toString()+"/v2/api/alarms/"+alarmKey;

        // Invoke the Signal K delete
        signalKClient->signalkDelete(QUrl(url));

        // Hide the widget window
        setVisible(false);

        // Emit the cancel pob signal
        emit cancelPOB();
    }

    /*
     * onHideClicked
     * Hides the widget window
     */
    void POBBar::onHideClicked() {

        // Hide the widget
        setVisible(false);

        // Emit the hide signal
        emit hidden();
    }

    /*
     * ~POBBar
     * Destructor
     */
    POBBar::~POBBar() {

        // Check if the timer is allocated
        if (m_timer) {

            // Stop the timer
            m_timer->stop();

            // Delete the timer
            delete m_timer;

            // Set the pointer to null
            m_timer = nullptr;
        }

        // Check if the ui is allocated
        if (ui) {

            // Delete the ui
            delete ui;

            // Set the pointer to null
            ui = nullptr;
        }
    }

    /*
 * updateElapsed
 * Method called to update the current datetime
 */
    void POBBar::updateElapsed() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("pob.startTime") && m_signalkPaths["pob.startTime"].is_string()) {

            // Get the key
            auto pobStartTime = QString::fromStdString(m_signalkPaths["pob.startTime"].get<std::string>());

            // Get the Signal K client
            auto signalKClient = FairWindSK::getInstance()->getSignalKClient();

            // Get the url
            QString url = signalKClient->url().toString()+"/v1/api/vessels/self/" + pobStartTime.replace(".","/");

            // Get the start time invoking the course api
            auto jsonObjectStartTime = signalKClient->signalkGet(QUrl(url));

            // Get the value as string
            auto startTimeIso8601 = jsonObjectStartTime["value"].toString();

            // Create a QDateTime from the string
            auto startDateTimeUTC = QDateTime::fromString(startTimeIso8601, "yyyy-MM-ddThh:mm:ss.zzzZ");

            // Set the start date time as UTC
            startDateTimeUTC.setOffsetFromUtc(0);

            // Get the current date
            const auto currentDateTimeUTC = QDateTime::currentDateTimeUtc();

            // Get the elapsed seconds
            const auto elapsedSecs = startDateTimeUTC.secsTo(currentDateTimeUTC);

            // Set the time label from the UI to the formatted time
            ui->label_Elapsed->setText(QDateTime::fromSecsSinceEpoch( elapsedSecs, Qt::UTC ).toString( "hh:mm:ss" ));
        }
    }

    /*
 * updatePOB
 * Method called in accordance to signalk to update the navigation position
 */
    void POBBar::updatePOB(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Check if there is a notifications.pob field and it is a string
        if (m_signalkPaths.contains("notifications.pob") && m_signalkPaths["notifications.pob"].is_string()) {

            // get the value of the notification object
            auto value = fairwindsk::signalk::Client::getObjectFromUpdateByPath(
                update,
                QString::fromStdString(m_signalkPaths["notifications.pob"].get<std::string>())
                );

            // Check if value is valid
            if (value.isEmpty()) {

                // Exit the method
                return;
            }

            // Check if there is a state field and it is a string
            if (value.contains("state") && value["state"].isString()) {

                // Check if the state is "normal" (no POB emergency)
                if (value["state"].toString() == "normal") {

                    // Check if the Options object has the rsa key and if it is a string
                    if (m_signalkPaths.contains("pob.bearing") && m_signalkPaths["pob.bearing"].is_string()) {

                        // Unsubscribe
                        FairWindSK::getInstance()->getSignalKClient()->removeSubscription(
                            QString::fromStdString(m_signalkPaths["pob.bearing"].get<std::string>()),
                            this);
                    }

                    // Check if the Options object has the rsa key and if it is a string
                    if (m_signalkPaths.contains("pob.distance") && m_signalkPaths["pob.distance"].is_string()) {

                        // Unsubscribe
                        FairWindSK::getInstance()->getSignalKClient()->removeSubscription(
                            QString::fromStdString(m_signalkPaths["pob.distance"].get<std::string>()),
                            this);
                    }

                    // Stop the timer
                    m_timer->stop();

                    // Hide the POBBar
                    setVisible(false);

                    // Check if the state is "emergency" (POB emergency)
                } else if (value["state"].toString() == "emergency") {

                    // Check if a data field is available
                    if (value.contains("data") && value["data"].isObject()) {

                        // Get the data object
                        auto data = value["data"].toObject();

                        // Check if data has the position field
                        if (data.contains("position") && data["position"].isObject()) {

                            // Get the position field
                            auto position = data["position"].toObject();

                            // The geographic coordinates object
                            QGeoCoordinate geoCoordinate;

                            // Check the position object contains a latitude field
                            if (position.contains("latitude")) {

                                // Assign the latitude at the geographic coordinates
                                geoCoordinate.setLatitude(position["latitude"].toDouble());
                            }

                            // Check the position object contains a longitude field
                            if (position.contains("longitude")) {

                                // Assign the longitude at the geographic coordinates
                                geoCoordinate.setLongitude(position["longitude"].toDouble());
                            }

                            // Check the position object contains a altitude field
                            if (position.contains("altitude")) {

                                // Assign the altitude at the geographic coordinates
                                geoCoordinate.setAltitude(position["altitude"].toDouble());
                            }

                            // Get the geographic coordinates as text list
                            auto texts = geoCoordinate.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",");

                            // Set the latitude
                            ui->label_Latitude->setText(texts[0]);

                            // Set the longitude
                            ui->label_Longitude->setText(texts[1]);
                        }
                    }

                    // Check if the Options object has the rsa key and if it is a string
                    if (m_signalkPaths.contains("pob.bearing") && m_signalkPaths["pob.bearing"].is_string()) {

                        // Subscribe and update
                        updateBearing(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                                QString::fromStdString(m_signalkPaths["pob.bearing"].get<std::string>()),
                                this,
                                SLOT(fairwindsk::ui::bottombar::POBBar::updateBearing)
                                ));
                    }

                    // Check if the Options object has the rsa key and if it is a string
                    if (m_signalkPaths.contains("pob.distance") && m_signalkPaths["pob.distance"].is_string()) {

                        // Subscribe and update
                        updateDistance(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                                QString::fromStdString(m_signalkPaths["pob.distance"].get<std::string>()),
                                this,
                                SLOT(fairwindsk::ui::bottombar::POBBar::updateDistance)
                                ));
                    }

                    // Start the timer
                    m_timer->start(1000);

                    // Show the POBBar
                    setVisible(true);
                }
            }
        }
    }

    /*
 * updateBearing
 * Method called in accordance to signalk to update the bearing
 */
    void POBBar::updateBearing(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            const auto text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_Bearing->setText(text);
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void POBBar::updateDistance(const QJsonObject &update) {


        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }


        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Build the formatted value
            const auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Distance->setText(text);
        }
    }

} // fairwindsk::ui::bottombar
