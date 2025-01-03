//
// Created by Raffaele Montella on 08/12/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AnchorBar.h" resolved


#include <QPushButton>
#include <QSlider>

#include "AnchorBar.hpp"

#include "FairWindSK.hpp"
#include "ui_AnchorBar.h"


namespace fairwindsk::ui::bottombar {
    AnchorBar::AnchorBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AnchorBar) {
        ui->setupUi(this);

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        // Get the configuration json object

        // Check if the configuration object contains the key 'signalk' with an object value
        if (auto configurationJsonObject = configuration->getRoot(); configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.position") && m_signalkPaths["anchor.position"].is_string()) {

                // Subscribe and update
                updatePosition(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.position"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updatePosition)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.depth") && m_signalkPaths["anchor.depth"].is_string()) {

                // Subscribe and update
                updateDepth(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.depth"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateDepth)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.bearing") && m_signalkPaths["anchor.bearing"].is_string()) {

                // Subscribe and update
                updateBearing(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.bearing"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateBearing)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.distance") && m_signalkPaths["anchor.distance"].is_string()) {

                // Subscribe and update
                updateDistance(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.distance"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateDistance)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.rode") && m_signalkPaths["anchor.rode"].is_string()) {

                // Subscribe and update
                updateRode(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.rode"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateRode)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.radius") && m_signalkPaths["anchor.radius"].is_string()) {

                // Subscribe and update
                updateCurrentRadius(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.radius"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateCurrentRadius)
                        ));
            }

            // Check if the Options object has the rsa key and if it is a string
            if (m_signalkPaths.contains("anchor.max") && m_signalkPaths["anchor.max"].is_string()) {

                // Subscribe and update
                updateMaxRadius(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["anchor.max"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AnchorBar::updateMaxRadius)
                        ));
            }

        }

        // Not visible by default
        QWidget::setVisible(false);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &AnchorBar::onHideClicked);

        connect(ui->toolButton_Reset, &QToolButton::clicked, this, &AnchorBar::onResetClicked);
        connect(ui->toolButton_Up, &QToolButton::pressed, this, &AnchorBar::onUpPressed);
        connect(ui->toolButton_Up, &QToolButton::released, this, &AnchorBar::onUpReleased);
        connect(ui->toolButton_Raise, &QToolButton::clicked, this, &AnchorBar::onRaiseClicked);
        connect(ui->pushButton_SetRadius, &QPushButton::clicked, this, &AnchorBar::onSetRadiusClicked);
        connect(ui->horizontalSlider_CurrentRadius, &QSlider::valueChanged, this, &AnchorBar::onCurrentRadiusChanged);
        connect(ui->toolButton_Drop, &QToolButton::clicked, this, &AnchorBar::onDropClicked);
        connect(ui->toolButton_Down, &QToolButton::pressed, this, &AnchorBar::onDownPressed);
        connect(ui->toolButton_Down, &QToolButton::released, this, &AnchorBar::onDownReleased);
        connect(ui->toolButton_Release, &QToolButton::clicked, this, &AnchorBar::onReleaseClicked);

    }

    void AnchorBar::onHideClicked() {
        setVisible(false);
        emit hidden();
    }

    void AnchorBar::onResetClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.reset") && m_signalkPaths["anchor.actions.reset"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.reset"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit resetCounter();
        }
    }

    void AnchorBar::onUpPressed() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.up") && m_signalkPaths["anchor.actions.up"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.up"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit chainUpPressed();
        }


    }

    void AnchorBar::onUpReleased() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.up") && m_signalkPaths["anchor.actions.up"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.up"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkDelete(QUrl(url));

            emit chainUpReleased();
        }
    }

    void AnchorBar::onRaiseClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.raise") && m_signalkPaths["anchor.actions.raise"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.raise"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit raiseAnchor();
        }
    }

    void AnchorBar::onCurrentRadiusChanged() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.radius") && m_signalkPaths["anchor.actions.radius"].is_string()) {

            // Convert m/s to knots
            auto value = m_units->convert(
                FairWindSK::getInstance()->getConfiguration()->getDepthUnits(),"m",
                ui->horizontalSlider_CurrentRadius->value());

            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            auto payload = QJsonObject();
            payload["radius"] = value;

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.radius"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url), payload);

            emit radiusChanged();
        }


    }

    void AnchorBar::onDropClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.drop") && m_signalkPaths["anchor.actions.drop"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.drop"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit dropAnchor();
        }
    }

    void AnchorBar::onSetRadiusClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.radius") && m_signalkPaths["anchor.actions.radius"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.radius"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit radiusSet();
        }
    }

    void AnchorBar::onDownPressed() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.down") && m_signalkPaths["anchor.actions.down"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.down"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit chainDownPressed();
        }
    }

    void AnchorBar::onDownReleased() {
        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.down") && m_signalkPaths["anchor.actions.down"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.down"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkDelete(QUrl(url));

            emit chainDownReleased();
        }
    }

    void AnchorBar::onReleaseClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.release") && m_signalkPaths["anchor.actions.release"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.release"].get<std::string>()).replace(".","/"
                        );

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            emit chainRelease();
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updatePosition(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(update);

        if (!value.isValid())
        {
            ui->widget_Position->setVisible(false);
        } else {

            auto texts = value.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",");

            // Set the latitude
            ui->label_Latitude->setText(texts[0]);

            // Set the longitude
            ui->label_Longitude->setText(texts[1]);

            if (!ui->widget_Position->isVisible()) {
                ui->widget_Position->setVisible(true);
            }
        }
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateDepth(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Depth->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getDepthUnits(), value);

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getDepthUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Depth->setText(text);

            if (!ui->widget_Depth->isVisible()) {
                ui->widget_Depth->setVisible(true);
            }
        }
    }

    /*
     * updateBearing
     * Method called in accordance to signalk to update the bearing
     */
    void AnchorBar::updateBearing(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Bearing->setVisible(false);
        } else {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            auto text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_Bearing->setText(text);

            if (!ui->widget_Bearing->isVisible()) {
                ui->widget_Bearing->setVisible(true);
            }
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateDistance(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Distance->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getDistanceUnits(), value);

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getDistanceUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Distance->setText(text);

            if (!ui->widget_Distance->isVisible()) {
                ui->widget_Distance->setVisible(true);
            }
        }
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateRode(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Rode->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Rode->setText(text);

            if (!ui->widget_Rode->isVisible()) {
                ui->widget_Rode->setVisible(true);
            }
        }
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateCurrentRadius(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Radius->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the slider value
            ui->horizontalSlider_CurrentRadius->setValue(static_cast<int>(value));

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_CurrentRadius->setText(text);

            if (!ui->widget_Radius->isVisible()) {
                ui->widget_Radius->setVisible(true);
            }
        }
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateMaxRadius(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        // Check if value is valid
        if (std::isnan(value)) {
            ui->widget_Radius->setVisible(false);
        } else {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);


            // Set the maximum value on the slider
            ui->horizontalSlider_CurrentRadius->setMaximum(static_cast<int>(value));

            // Build the formatted value
            auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_MaxRadius->setText(text);

            if (!ui->widget_Radius->isVisible()) {
                ui->widget_Radius->setVisible(true);
            }
        }
    }



    AnchorBar::~AnchorBar() {
        if (ui) {
            delete ui;
            ui = nullptr;
        }

    }
} // fairwindsk::ui::bottombar
