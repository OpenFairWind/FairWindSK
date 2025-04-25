//
// Created by Raffaele Montella on 12/04/21.
//

#include <QTimer>


#include <QAbstractButton>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QString>
#include <nlohmann/json.hpp>

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

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        ui->toolButton_UL->setIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png")));
        ui->toolButton_UL->setIconSize(QSize(32, 32));

        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(32, 32));
        
        ui->label_unitCOG->setText(m_units->getLabel("deg"));
        ui->label_unitBTW->setText(m_units->getLabel("deg"));
        ui->label_unitHDG->setText(m_units->getLabel("deg"));

        ui->label_unitSOG->setText(m_units->getLabel(configuration->getVesselSpeedUnits()));
        ui->label_unitSTW->setText(m_units->getLabel(configuration->getVesselSpeedUnits()));
        ui->label_unitVMG->setText(m_units->getLabel(configuration->getVesselSpeedUnits()));

        ui->label_unitDPT->setText(m_units->getLabel(configuration->getDepthUnits()));

        ui->label_unitDTG->setText(m_units->getLabel(configuration->getDistanceUnits()));
        ui->label_unitXTE->setText(m_units->getLabel(configuration->getDistanceUnits()));


        double c=1.15;
        ui->label_POS->setFixedWidth(ui->label_POS->sizeHint().width()*c);
        ui->label_COG->setFixedWidth(ui->label_COG->sizeHint().width()*c);
        ui->label_SOG->setFixedWidth(ui->label_SOG->sizeHint().width()*c);
        ui->label_HDG->setFixedWidth(ui->label_HDG->sizeHint().width()*c);
        ui->label_STW->setFixedWidth(ui->label_STW->sizeHint().width()*c);
        ui->label_DPT->setFixedWidth(ui->label_DPT->sizeHint().width()*c);

        ui->label_BTW->setFixedWidth(ui->label_BTW->sizeHint().width()*c);
        ui->label_DTG->setFixedWidth(ui->label_DTG->sizeHint().width()*c);
        ui->label_TTG->setFixedWidth(ui->label_TTG->sizeHint().width()*c);
        ui->label_ETA->setFixedWidth(ui->label_ETA->sizeHint().width()*c);
        ui->label_XTE->setFixedWidth(ui->label_XTE->sizeHint().width()*c);
        ui->label_VMG->setFixedWidth(ui->label_VMG->sizeHint().width()*c);

        ui->widget_POS->setVisible(false);
        ui->widget_COG->setVisible(false);
        ui->widget_SOG->setVisible(false);
        ui->widget_HDG->setVisible(false);
        ui->widget_STW->setVisible(false);
        ui->widget_DPT->setVisible(false);

        ui->widget_WPT->setVisible(false);
        ui->widget_BTW->setVisible(false);
        ui->widget_DTG->setVisible(false);
        ui->widget_TTG->setVisible(false);
        ui->widget_ETA->setVisible(false);
        ui->widget_XTE->setVisible(false);
        ui->widget_VMG->setVisible(false);





        // emit a signal when the Apps tool button from the UI is clicked
        connect(ui->toolButton_UL, &QToolButton::released, this, &TopBar::toolbuttonUL_clicked);

        // emit a signal when the Settings tool button from the UI is clicked
        connect(ui->toolButton_UR, &QToolButton::released, this, &TopBar::toolbuttonUR_clicked);

        // Create a new timer which will contain the current time
        m_timer = new QTimer(this);

        // When the timer stops, update the time
        connect(m_timer, &QTimer::timeout, this, &TopBar::updateTime);

        // Start the timer
        m_timer->start(1000);

        // Get the configuration json object
        auto confiurationJsonObject = configuration->getRoot();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (confiurationJsonObject.contains("signalk") && confiurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            auto signalkPaths = confiurationJsonObject["signalk"];

            // Check if the Options object has tHe Position key and if it is a string
            if (signalkPaths.contains("pos") && signalkPaths["pos"].is_string()) {

                // Subscribe and update
                updatePOS(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["pos"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updatePOS)
                        ));
            }

            // Check if the Options object has tHe Heading key and if it is a string
            if (signalkPaths.contains("cog") && signalkPaths["cog"].is_string()) {

                // Subscribe and update
                updateCOG(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["cog"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateCOG)
                ));



            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("sog") && signalkPaths["sog"].is_string()) {

                // Subscribe and update
                updateSOG( FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["sog"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateSOG)
                ));
            }

            // Check if the Options object has tHe Heading key and if it is a string
            if (signalkPaths.contains("hdg") && signalkPaths["hdg"].is_string()) {

                // Subscribe and update
                updateHDG( FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["hdg"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateHDG)
                ));


            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("stw") && signalkPaths["stw"].is_string()) {

                /// Subscribe and update
                updateSTW( FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["stw"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateSTW)
                ));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("dpt") && signalkPaths["dpt"].is_string()) {

                // Subscribe and update
                updateDPT(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["dpt"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateDPT)
                ));


            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("wpt") && signalkPaths["wpt"].is_string()) {

                // Subscribe and update
                updateWPT( FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["wpt"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateWPT)
                ));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("btw") && signalkPaths["btw"].is_string()) {

                // Subscribe and update
                updateBTW(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["btw"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateBTW)
                ));


            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("dtg") && signalkPaths["dtg"].is_string()) {

                // Subscribe and update
                updateDTG(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["dtg"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateDTG)
                ));


            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("ttg") && signalkPaths["ttg"].is_string()) {

                // Subscribe and update
                updateTTG(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["ttg"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateTTG)
                ));

            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("eta") && signalkPaths["eta"].is_string()) {

                // Subscribe and update
                updateETA( FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["eta"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateETA)
                ));
            }



            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("xte") && signalkPaths["xte"].is_string()) {

                // Subscribe and update
                updateXTE(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["xte"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateXTE)
                ));
            }

            // Check if the Options object has tHe Speed key and if it is a string
            if (signalkPaths.contains("vmg") && signalkPaths["vmg"].is_string()) {

                // Subscribe and update
                updateVMG(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(signalkPaths["vmg"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::topbar::TopBar::updateVMG)
                ));
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
        if (m_currentApp) {
            ((fairwindsk::ui::web::Web *)(m_currentApp->getWidget()))->toggleNavigationBar();
        }
    }

/*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the navigation position
 */
    void TopBar::updatePOS(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }


        // Get the value
        const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(update);

        if (!value.isValid()) {
            ui->widget_POS->setVisible(false);
        } else {


            const auto text = value.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere);

            // Set the course over ground label from the UI to the formatted value
            ui->label_POS->setText(text);

            if (!ui->widget_POS->isVisible()) {
                ui->widget_POS->setVisible(true);
            }
        }
    }


    /*
 * updateNavigationCourseOverGroundTrue
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void TopBar::updateCOG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }


        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_COG->setVisible(false);
        } else {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_COG->setText(text);

            if (!ui->widget_COG->isVisible()) {
                ui->widget_COG->setVisible(true);
            }
        }

    }

/*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateSOG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_SOG->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("ms-1",FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits(), value);

            // Build the formatted value
            text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_SOG->setText(text);

            if (!ui->widget_SOG->isVisible()) {
                ui->widget_SOG->setVisible(true);
            }
        }


    }

    /*
 * updateNavigationCourseOverGroundTrue
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void TopBar::updateHDG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_HDG->setVisible(false);
        } else {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_HDG->setText(text);

            if (!ui->widget_HDG->isVisible()) {
                ui->widget_HDG->setVisible(true);
            }
        }

    }

/*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateSTW(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_STW->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("ms-1",FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits(), value);

            // Build the formatted value
            text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_STW->setText(text);

            if (!ui->widget_STW->isVisible()) {
                ui->widget_STW->setVisible(true);
            }
        }


    }

    /*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateDPT(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_DPT->setVisible(false);
        } else {

            // Convert m to depth units
            value = m_units->convert("mt",FairWindSK::getInstance()->getConfiguration()->getDepthUnits(), value);

            // Build the formatted value
            text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getDepthUnits(), value);

            // Set the the formatted value
            ui->label_DPT->setText(text);

	    // Check if the widget is visible
            if (!ui->widget_DPT->isVisible()) {

		// Set the widget visible
                ui->widget_DPT->setVisible(true);
            }
        }


    }

    void TopBar::updateWPT(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value =fairwindsk::signalk::Client::getObjectFromUpdateByPath(update);



        if (value.isEmpty()) {
            ui->widget_WPT->setVisible(false);
        } else {
            if (value.contains("href") && value["href"].isString()) {
                auto href = value["href"].toString();

                auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref(href);

                ui->label_WPT->setText(waypoint.getName());

                if (!ui->widget_WPT->isVisible()) {
                    ui->widget_WPT->setVisible(true);
                }
            }
        }

    }

    void TopBar::updateBTW(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value =fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_BTW->setVisible(false);
        } else {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_BTW->setText(text);

            if (!ui->widget_BTW->isVisible()) {
                ui->widget_BTW->setVisible(true);
            }
        }

    }

    void TopBar::updateDTG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_DTG->setVisible(false);
        } else {

            // Convert meters to nautical miles
            value = m_units->convert("m","nm", value);

            // Build the formatted value
            text = m_units->format("nm", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_DTG->setText(text);

            if (!ui->widget_DTG->isVisible()) {
                ui->widget_DTG->setVisible(true);
            }
        }

    }

    void TopBar::updateTTG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(update);

        if (value.isNull()) {
            ui->widget_TTG->setVisible(false);
        } else {

            // Convert meters to nautical miles
            //value = m_units->convert("m","nm", value);

            // Build the formatted value
            //text = m_units->format("nm", value);
            text = value.toString();

            // Set the course over ground label from the UI to the formatted value
            ui->label_TTG->setText(text);

            if (!ui->widget_TTG->isVisible()) {
                ui->widget_TTG->setVisible(true);
            }
        }

    }

    void TopBar::updateETA(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(update);

        if (value.isNull()) {
            ui->widget_ETA->setVisible(false);
        } else {

            // Convert meters to nautical miles
            //value = m_units->convert("m","nm", value);

            // Build the formatted value
            //text = m_units->format("nm", value);
            text = value.toString();

            // Set the course over ground label from the UI to the formatted value
            ui->label_ETA->setText(text);

            if (!ui->widget_ETA->isVisible()) {
                ui->widget_ETA->setVisible(true);
            }
        }

    }

    void TopBar::updateXTE(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_XTE->setVisible(false);
        } else {

            // Convert meters to nautical miles
            value = m_units->convert("m","nm", value);

            // Build the formatted value
            text = m_units->format("nm", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_XTE->setText(text);

            if (!ui->widget_XTE->isVisible()) {
                ui->widget_XTE->setVisible(true);
            }
        }

    }

    void TopBar::updateVMG(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        QString text;

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_VMG->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("ms-1","kn", value);

            // Build the formatted value
            text = m_units->format("kn", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_VMG->setText(text);

            if (!ui->widget_VMG->isVisible()) {
                ui->widget_VMG->setVisible(true);
            }
        }


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

/*
 * ~TopBar
 * TopBar's destructor
 */
    TopBar::~TopBar() {

        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        delete ui;
    }
}
