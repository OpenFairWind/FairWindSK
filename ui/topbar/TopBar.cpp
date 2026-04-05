//
// Created by Raffaele Montella on 12/04/21.
//

#include <QTimer>
#include <QAbstractButton>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QString>
#include <QFontMetrics>
#include <QPixmap>
#include <nlohmann/json.hpp>

#include "TopBar.hpp"
#include "../web/Web.hpp"
#include "ui/IconUtils.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include <signalk/Waypoint.hpp>

namespace fairwindsk::ui::topbar {
    namespace {
        const QString kChromeToolButtonStyle = QStringLiteral(
            "QToolButton {"
            " background: transparent;"
            " border: none;"
            " padding: 6px;"
            " }"
            "QToolButton:hover { background: rgba(127, 127, 127, 0.18); border-radius: 8px; }"
            "QToolButton:pressed { background: rgba(127, 127, 127, 0.28); border-radius: 8px; }");

        QString comfortViewIconPath(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == "dawn") {
                return QStringLiteral(":/resources/svg/OpenBridge/comfort-dawn.svg");
            }
            if (preset == "sunset") {
                return QStringLiteral(":/resources/svg/OpenBridge/comfort-sunset.svg");
            }
            if (preset == "dusk") {
                return QStringLiteral(":/resources/svg/OpenBridge/comfort-dusk.svg");
            }
            if (preset == "night") {
                return QStringLiteral(":/resources/svg/OpenBridge/comfort-night.svg");
            }
            return QStringLiteral(":/resources/svg/OpenBridge/comfort-day.svg");
        }
    }

    void TopBar::refreshMetricLabelWidths() const {
        const QFontMetrics valueMetrics(ui->label_COG->font());
        const QFontMetrics unitMetrics(ui->label_unitCOG->font());
        const QFontMetrics appMetrics(ui->label_ApplicationName->font());

        const int positionWidth = valueMetrics.horizontalAdvance(QStringLiteral("59°59.999' N 179°59.999' E"));
        const int angleWidth = valueMetrics.horizontalAdvance(QStringLiteral("360.0"));
        const int distanceWidth = valueMetrics.horizontalAdvance(QStringLiteral("9999.9"));
        const int timeWidth = valueMetrics.horizontalAdvance(QStringLiteral("23:59"));
        const int etaWidth = valueMetrics.horizontalAdvance(QStringLiteral("31-12 23:59"));
        const int waypointWidth = valueMetrics.horizontalAdvance(QStringLiteral("WAYPOINT NAME"));
        const int unitWidth = unitMetrics.horizontalAdvance(QStringLiteral("nmi"));
        const int appWidth = appMetrics.horizontalAdvance(QStringLiteral("Application launcher"));

        ui->label_POS->setFixedWidth(positionWidth);
        ui->label_COG->setFixedWidth(angleWidth);
        ui->label_SOG->setFixedWidth(distanceWidth);
        ui->label_HDG->setFixedWidth(angleWidth);
        ui->label_STW->setFixedWidth(distanceWidth);
        ui->label_DPT->setFixedWidth(distanceWidth);
        ui->label_WPT->setFixedWidth(waypointWidth);
        ui->label_BTW->setFixedWidth(angleWidth);
        ui->label_DTG->setFixedWidth(distanceWidth);
        ui->label_TTG->setFixedWidth(timeWidth);
        ui->label_ETA->setFixedWidth(etaWidth);
        ui->label_XTE->setFixedWidth(distanceWidth);
        ui->label_VMG->setFixedWidth(distanceWidth);

        ui->label_unitCOG->setFixedWidth(unitWidth);
        ui->label_unitSOG->setFixedWidth(unitWidth);
        ui->label_unitHDG->setFixedWidth(unitWidth);
        ui->label_unitSTW->setFixedWidth(unitWidth);
        ui->label_unitDPT->setFixedWidth(unitWidth);
        ui->label_unitBTW->setFixedWidth(unitWidth);
        ui->label_unitDTG->setFixedWidth(unitWidth);
        ui->label_unitXTE->setFixedWidth(unitWidth);
        ui->label_unitVMG->setFixedWidth(unitWidth);
        ui->label_ApplicationName->setMinimumWidth(appWidth);
    }

/*
 * TopBar
 * Public Constructor - This presents some useful infos at the top of the screen
 */
    TopBar::TopBar(QWidget *parent) :
            QWidget(parent),
            ui(new Ui::TopBar) {
        // Setup the UI
        ui->setupUi(this);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        ui->toolButton_UL->setIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png")));
        ui->toolButton_UL->setIconSize(QSize(32, 32));
        ui->toolButton_UL->setAutoRaise(true);
        ui->toolButton_UL->setStyleSheet(kChromeToolButtonStyle);

        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(32, 32));
        ui->toolButton_UR->setAutoRaise(true);
        ui->toolButton_UR->setStyleSheet(kChromeToolButtonStyle);
        
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
        updateTime();
        updateComfortViewIcon();
        resetCurrentAppPresentation();

        // Get the configuration json object
        auto confiurationJsonObject = configuration->getRoot();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (confiurationJsonObject.contains("signalk") && confiurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            auto signalkPaths = confiurationJsonObject["signalk"];

            if (signalkPaths.contains("cog") && signalkPaths["cog"].is_string()) {
                m_pathCOG = QString::fromStdString(signalkPaths["cog"].get<std::string>());
            }
            if (signalkPaths.contains("sog") && signalkPaths["sog"].is_string()) {
                m_pathSOG = QString::fromStdString(signalkPaths["sog"].get<std::string>());
            }
            if (signalkPaths.contains("hdg") && signalkPaths["hdg"].is_string()) {
                m_pathHDG = QString::fromStdString(signalkPaths["hdg"].get<std::string>());
            }
            if (signalkPaths.contains("stw") && signalkPaths["stw"].is_string()) {
                m_pathSTW = QString::fromStdString(signalkPaths["stw"].get<std::string>());
            }
            if (signalkPaths.contains("dpt") && signalkPaths["dpt"].is_string()) {
                m_pathDPT = QString::fromStdString(signalkPaths["dpt"].get<std::string>());
            }
            if (signalkPaths.contains("btw") && signalkPaths["btw"].is_string()) {
                m_pathBTW = QString::fromStdString(signalkPaths["btw"].get<std::string>());
            }
            if (signalkPaths.contains("dtg") && signalkPaths["dtg"].is_string()) {
                m_pathDTG = QString::fromStdString(signalkPaths["dtg"].get<std::string>());
            }
            if (signalkPaths.contains("xte") && signalkPaths["xte"].is_string()) {
                m_pathXTE = QString::fromStdString(signalkPaths["xte"].get<std::string>());
            }
            if (signalkPaths.contains("vmg") && signalkPaths["vmg"].is_string()) {
                m_pathVMG = QString::fromStdString(signalkPaths["vmg"].get<std::string>());
            }

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

        ui->label_unitCOG->setText(m_units->getSignalKUnitLabel(m_pathCOG, "deg"));
        ui->label_unitBTW->setText(m_units->getSignalKUnitLabel(m_pathBTW, "deg"));
        ui->label_unitHDG->setText(m_units->getSignalKUnitLabel(m_pathHDG, "deg"));
        ui->label_unitDPT->setText(m_units->getSignalKUnitLabel(m_pathDPT, configuration->getDepthUnits()));
        updateSpeedLabels();
        updateDistanceLabels();
        refreshMetricLabelWidths();
    }

    void TopBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);

        if (event && event->type() == QEvent::FontChange) {
            refreshMetricLabelWidths();
        }

        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            updateComfortViewIcon();
        }
    }

    void TopBar::updateComfortViewIcon() const {
        if (!ui || !ui->label_ComfortViewIcon) {
            return;
        }

        const auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset() : QStringLiteral("day");
        const QColor iconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Window),
            {palette().color(QPalette::WindowText),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::Text)});
        const QIcon icon = fairwindsk::ui::tintedIcon(QIcon(comfortViewIconPath(preset)), iconColor, ui->label_ComfortViewIcon->size());
        ui->label_ComfortViewIcon->setPixmap(icon.pixmap(ui->label_ComfortViewIcon->size()));
        ui->label_ComfortViewIcon->setToolTip(tr("Comfort view: %1").arg(preset));
    }

/*
 * updateTime
 * Method called to update the current datetime
 */
    void TopBar::updateTime() {
        const QDateTime dateTime = QDateTime::currentDateTime();
        QString text = dateTime.toString(QStringLiteral("dd-MM-yyyy hh:mm"));

        if ((dateTime.time().second() % 2) == 0 && text.size() >= 13) {
            text[13] = QChar(' ');
        }

        ui->label_DateTime->setText(text);
        updateComfortViewIcon();
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
        if (!m_currentApp) {
            return;
        }

        if (auto *webView = qobject_cast<fairwindsk::ui::web::Web *>(m_currentApp->getWidget())) {
            webView->toggleNavigationBar();
        }
    }

    void TopBar::refreshFromConfiguration() {
        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        ui->label_unitCOG->setText(m_units->getSignalKUnitLabel(m_pathCOG, "deg"));
        ui->label_unitBTW->setText(m_units->getSignalKUnitLabel(m_pathBTW, "deg"));
        ui->label_unitHDG->setText(m_units->getSignalKUnitLabel(m_pathHDG, "deg"));
        ui->label_unitDPT->setText(m_units->getSignalKUnitLabel(m_pathDPT, configuration->getDepthUnits()));
        updateSpeedLabels();
        updateDistanceLabels();
        refreshMetricLabelWidths();

        updatePOS(m_lastPosUpdate);
        updateCOG(m_lastCogUpdate);
        updateSOG(m_lastSogUpdate);
        updateHDG(m_lastHdgUpdate);
        updateSTW(m_lastStwUpdate);
        updateDPT(m_lastDptUpdate);
        updateWPT(m_lastWptUpdate);
        updateBTW(m_lastBtwUpdate);
        updateDTG(m_lastDtgUpdate);
        updateTTG(m_lastTtgUpdate);
        updateETA(m_lastEtaUpdate);
        updateXTE(m_lastXteUpdate);
        updateVMG(m_lastVmgUpdate);
    }

/*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the navigation position
 */
    void TopBar::updatePOS(const QJsonObject &update) {
        m_lastPosUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }


        // Get the value
        const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(update);

        if (!value.isValid()) {
            ui->widget_POS->setVisible(false);
        } else {


            const auto text = fairwindsk::ui::geo::formatCoordinate(
                value,
                FairWindSK::getInstance()->getConfiguration()->getCoordinateFormat());

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
        m_lastCogUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_COG->setVisible(false);
        } else {

            // Convert rad to deg
            const auto text = m_units->formatSignalKValue(m_pathCOG, value, "rad", "deg");

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
        m_lastSogUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_SOG->setVisible(false);
        } else {

            const auto vesselSpeedUnits = FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits();
            const auto text = m_units->formatSignalKValue(m_pathSOG, value, "ms-1", vesselSpeedUnits);

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
        m_lastHdgUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_HDG->setVisible(false);
        } else {

            // Convert rad to deg
            const auto text = m_units->formatSignalKValue(m_pathHDG, value, "rad", "deg");

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
        m_lastStwUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_STW->setVisible(false);
        } else {

            const auto vesselSpeedUnits = FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits();
            const auto text = m_units->formatSignalKValue(m_pathSTW, value, "ms-1", vesselSpeedUnits);

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
        m_lastDptUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_DPT->setVisible(false);
        } else {

            const auto text = m_units->formatSignalKValue(
                m_pathDPT,
                value,
                "mt",
                FairWindSK::getInstance()->getConfiguration()->getDepthUnits());

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
        m_lastWptUpdate = update;

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
        m_lastBtwUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value =fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_BTW->setVisible(false);
        } else {

            // Convert rad to deg
            const auto text = m_units->formatSignalKValue(m_pathBTW, value, "rad", "deg");

            // Set the course over ground label from the UI to the formatted value
            ui->label_BTW->setText(text);

            if (!ui->widget_BTW->isVisible()) {
                ui->widget_BTW->setVisible(true);
            }
        }

    }

    void TopBar::updateDTG(const QJsonObject &update) {
        m_lastDtgUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_DTG->setVisible(false);
        } else {

            const auto distanceUnits = FairWindSK::getInstance()->getConfiguration()->getDistanceUnits();
            const auto text = m_units->formatSignalKValue(m_pathDTG, value, "m", distanceUnits);

            // Set the course over ground label from the UI to the formatted value
            ui->label_DTG->setText(text);

            if (!ui->widget_DTG->isVisible()) {
                ui->widget_DTG->setVisible(true);
            }
        }

    }

    void TopBar::updateTTG(const QJsonObject &update) {
        m_lastTtgUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(update);

        if (value.isNull()) {
            ui->widget_TTG->setVisible(false);
        } else {

            // Convert meters to nautical miles
            //value = m_units->convert("m","nm", value);

            // Build the formatted value
            //text = m_units->format("nm", value);
            const auto text = value.toLocalTime().toString("hh:mm");

            // Set the course over ground label from the UI to the formatted value
            ui->label_TTG->setText(text);

            if (!ui->widget_TTG->isVisible()) {
                ui->widget_TTG->setVisible(true);
            }
        }

    }

    void TopBar::updateETA(const QJsonObject &update) {
        m_lastEtaUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(update);

        if (value.isNull()) {
            ui->widget_ETA->setVisible(false);
        } else {

            // Convert meters to nautical miles
            //value = m_units->convert("m","nm", value);

            // Build the formatted value
            //text = m_units->format("nm", value);
            const auto text = value.toLocalTime().toString("dd-MM-yyyy hh:mm");

            // Set the course over ground label from the UI to the formatted value
            ui->label_ETA->setText(text);

            if (!ui->widget_ETA->isVisible()) {
                ui->widget_ETA->setVisible(true);
            }
        }

    }

    void TopBar::updateXTE(const QJsonObject &update) {
        m_lastXteUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_XTE->setVisible(false);
        } else {

            const auto distanceUnits = FairWindSK::getInstance()->getConfiguration()->getDistanceUnits();
            const auto text = m_units->formatSignalKValue(m_pathXTE, value, "m", distanceUnits);

            // Set the course over ground label from the UI to the formatted value
            ui->label_XTE->setText(text);

            if (!ui->widget_XTE->isVisible()) {
                ui->widget_XTE->setVisible(true);
            }
        }

    }

    void TopBar::updateVMG(const QJsonObject &update) {
        m_lastVmgUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_VMG->setVisible(false);
        } else {

            const auto vesselSpeedUnits = FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits();
            const auto text = m_units->formatSignalKValue(m_pathVMG, value, "ms-1", vesselSpeedUnits);

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
            ui->toolButton_UR->setEnabled(qobject_cast<fairwindsk::ui::web::Web *>(m_currentApp->getWidget()) != nullptr);
        } else {
            resetCurrentAppPresentation();
        }
    }

    void TopBar::setCurrentContext(const QString &name, const QString &tooltip, const QIcon &icon, const bool enableButton) {
        m_currentApp = nullptr;
        ui->toolButton_UR->setIcon(icon.isNull()
                                           ? QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png"))
                                           : icon);
        ui->toolButton_UR->setIconSize(QSize(32, 32));
        ui->toolButton_UR->setEnabled(enableButton);
        ui->label_ApplicationName->setText(name);
        ui->label_ApplicationName->setToolTip(tooltip);
    }

    void TopBar::updateDistanceLabels() const {
        const auto distanceUnits = FairWindSK::getInstance()->getConfiguration()->getDistanceUnits();
        ui->label_unitDTG->setText(m_units->getSignalKUnitLabel(m_pathDTG, distanceUnits));
        ui->label_unitXTE->setText(m_units->getSignalKUnitLabel(m_pathXTE, distanceUnits));
    }

    void TopBar::updateSpeedLabels() const {
        const auto vesselSpeedUnits = FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits();
        ui->label_unitSOG->setText(m_units->getSignalKUnitLabel(m_pathSOG, vesselSpeedUnits));
        ui->label_unitSTW->setText(m_units->getSignalKUnitLabel(m_pathSTW, vesselSpeedUnits));
        ui->label_unitVMG->setText(m_units->getSignalKUnitLabel(m_pathVMG, vesselSpeedUnits));
    }

    void TopBar::resetCurrentAppPresentation() const {
        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(32, 32));
        ui->toolButton_UR->setEnabled(false);
        ui->label_ApplicationName->setText("");
        ui->label_ApplicationName->setToolTip("");
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
