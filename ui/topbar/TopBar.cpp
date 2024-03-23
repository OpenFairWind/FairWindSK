//
// Created by Raffaele Montella on 12/04/21.
//

#include <QTimer>


#include <QAbstractButton>
#include <QGeoCoordinate>
#include <QJsonObject>
#include <QToolButton>

#include "TopBar.hpp"
#include "../web/Web.hpp"
#include <signalk/Waypoint.hpp>

namespace fairwindsk::ui::topbar {
/*
 * TopBar
 * Public Constructor - This presents some useful infos at the top of the screen
 */
    TopBar::TopBar(QWidget *parent) :
            QWidget(parent),
            ui(new Ui::TopBar) {
        // Setup the UI
        ui->setupUi(parent);

        ui->toolButton_UL->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/fairwind_icon.png")));
        ui->toolButton_UL->setIconSize(QSize(32, 32));

        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(32, 32));

        // emit a signal when the Apps tool button from the UI is clicked
        connect(ui->toolButton_UL, &QToolButton::released, this, &TopBar::toolbuttonUL_clicked);

        // emit a signal when the Settings tool button from the UI is clicked
        connect(ui->toolButton_UR, &QToolButton::released, this, &TopBar::toolbuttonUR_clicked);

        // Create a new timer which will contain the current time
        auto *timer = new QTimer(this);

        // When the timer stops, update the time
        connect(timer, &QTimer::timeout, this, &TopBar::updateTime);

        // Start the timer
        timer->start(1000);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get the signalk document from the FairWind singleton
        auto signalKDocument = fairWindSK->getSignalKDocument();

        // Get the configuration object
        auto configuration = fairWindSK->getConfiguration();

        if (configuration.contains("signalk") && configuration["signalk"].isObject()) {
            auto signalkPaths = configuration["signalk"].toObject();


            // Check if the Options object has tHe Position key and if it is a string
            if (signalkPaths.contains("position") && signalkPaths["position"].isString()) {

                // Get the position SignalK key
                auto positionSignalK = signalkPaths["position"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(positionSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updatePosition));
            }

            // Check if the Options object has tHe Heading key and if it is a string
            if (signalkPaths.contains("cog") && signalkPaths["cog"].isString()) {

                // Get the heading SignalK key
                auto headingSignalK = signalkPaths["cog"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(headingSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateCOG));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("sog") && signalkPaths["sog"].isString()) {

                // Get the speed SignalK key
                auto speedSignalK = signalkPaths["sog"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(speedSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateSOG));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("wpt") && signalkPaths["wpt"].isString()) {

                // Get the speed SignalK key
                auto nextPointSignalK = signalkPaths["wpt"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(nextPointSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateWPT));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("brg") && signalkPaths["brg"].isString()) {

                // Get the speed SignalK key
                auto bearingSignalK = signalkPaths["brg"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(bearingSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateBRG));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("dtg") && signalkPaths["dtg"].isString()) {

                // Get the speed SignalK key
                auto distanceSignalK = signalkPaths["dtg"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(distanceSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateDTG));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("xte") && signalkPaths["xte"].isString()) {

                // Get the speed SignalK key
                auto crossTrackErrorSignalK = signalkPaths["xte"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(crossTrackErrorSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateXTE));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("vmg") && signalkPaths["vmg"].isString()) {

                // Get the speed SignalK key
                auto velocityMadeGoodSignalK = signalkPaths["vmg"].toString();

                // Subscribe to signalk and make sure that navigation infos are updated accordingly
                signalKDocument->subscribe(velocityMadeGoodSignalK, this,
                                           SLOT(fairwindsk::ui::topbar::TopBar::updateVMG));
            }


        }
    }

/*
 * updateTime
 * Method called to update the current datetime
 */
    void TopBar::updateTime() {
        // Get the current time
        QTime time = QTime::currentTime();
        // Get the current time in hh:mm format
        QString text = time.toString("hh:mm");

        if ((time.second() % 2) == 0)
            text[2] = ' ';
        // Set the time label from the UI to the formatted time
        ui->label_Time->setText(text);

        // Get the current date
        QDateTime dateTime = QDateTime::currentDateTime();
        // Get the current date in dd-MM-yyy format
        text = dateTime.toString("dd-MM-yyyy");
        // Set the date label from the UI to the formatted date
        ui->label_Date->setText(text);
    }

/*
 * settings_clicked
 * Method called when the user wants to view the settings screen
 */
    void TopBar::toolbuttonUL_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit clickedToolbuttonUL();
    }

/*
 * apps_clicked
 * Method called when the user wants to view the apps screen
 */
    void TopBar::toolbuttonUR_clicked() {
        m_currentApp->getWeb()->toggleButtons();
    }

/*
 * ~TopBar
 * TopBar's destructor
 */
    TopBar::~TopBar() {
        delete ui;
    }

/*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the navigation position
 */
    void TopBar::updatePosition(const QJsonObject &update) {
        //qDebug() << "TopBar::updateNavigationPosition:" << update;

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get the signalk document from the FairWind singleton itself
        auto signalKDocument = fairWindSK->getSignalKDocument();

        // Show the coordinates
        ui->label_Position_value->setText(
                signalKDocument->getNavigationPosition().toString(
                        QGeoCoordinate::DegreesMinutesSecondsWithHemisphere
                )
        );

    }


    /*
 * updateNavigationCourseOverGroundTrue
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void TopBar::updateCOG(const QJsonObject &update) {

        QString text;

        // Get the value
        auto value = FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "___._";
        } else {

            // Convert rad to deg
            value = value  * 57.2958;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 4, 'f', 1, '0');
        }
        // Set the course over ground label from the UI to the formatted value
        ui->label_COG->setText(text);
    }

/*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateSOG(const QJsonObject &update) {

        QString text;

        // Get the value
        auto value = FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "__._";
        } else {

            // Convert m/s to knots
            value = value  * 1.94384;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 3, 'f', 1, '0');
        }

        // Set the speed over ground label from the UI to the formatted value
        ui->label_SOG->setText(text);
    }

    void TopBar::updateWPT(const QJsonObject &update) {

        // Get the value
        auto value =FairWindSK::getInstance()->getSignalKClient()->getObjectFromUpdateByPath(update);

        if (value.isEmpty()) {
            ui->label_Waypoint->setText("");
        } else {
            if (value.contains("href") && value["href"].isString()) {
                auto href = value["href"].toString();

                auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref(href);

                ui->label_Waypoint->setText(waypoint.getName());
            }
        }

    }
    void TopBar::updateBRG(const QJsonObject &update) {

        QString text;

        // Get the value
        auto value =FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "___._";
        } else {

            // Convert rad to deg
            value = value  * 57.2958;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 4, 'f', 1, '0');
        }
        // Set the course over ground label from the UI to the formatted value
        ui->label_BRG->setText(text);
    }

    void TopBar::updateDTG(const QJsonObject &update) {


        QString text;

        // Get the value
        auto value = FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "____._";
        } else {

            // Convert meters to nautical miles
            value = value / 1852.00;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 5, 'f', 1, '0');
        }
        // Set the course over ground label from the UI to the formatted value
        ui->label_DTG->setText(text);
    }

    void TopBar::updateXTE(const QJsonObject &update) {

        QString text;

        // Get the value
        auto value = FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "__._";
        } else {

            // Convert meters to nautical miles
            value = value  / 1852.00;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 3, 'f', 1, '0');
        }
        // Set the course over ground label from the UI to the formatted value
        ui->label_XTE->setText(text);
    }

    void TopBar::updateVMG(const QJsonObject &update) {
        QString text;

        // Get the value
        auto value = FairWindSK::getInstance()->getSignalKClient()->getDoubleFromUpdateByPath(update);

        if (isnan(value)) {
            text = "__._";
        } else {

            // Convert m/s to knots
            value = value  * 1.94384;

            // Build the formatted value
            text = QString{"%1"}.arg(value, 3, 'f', 1, '0');
        }
        // Set the course over ground label from the UI to the formatted value
        ui->label_VMG->setText(text);

    }

    void TopBar::setCurrentApp(AppItem *appItem) {
        m_currentApp = appItem;
        if (m_currentApp) {
            ui->toolButton_UR->setIcon(m_currentApp->getIcon());
            ui->toolButton_UR->setIconSize(QSize(32, 32));
            ui->label_ApplicationName->setText(m_currentApp->getDisplayName());
            ui->label_ApplicationName->setToolTip(m_currentApp->getDescription());
        } else {
            ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":resources/images/icons/apps_icon.png")));
            ui->toolButton_UR->setIconSize(QSize(32, 32));
            ui->label_ApplicationName->setText("");
            ui->label_ApplicationName->setToolTip("");
        }
    }

}