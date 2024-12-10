//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Autopilot.h" resolved

#include <QtWidgets/QAbstractButton>
#include "AutopilotBar.hpp"

#include "FairWindSK.hpp"
#include "ui_AutopilotBar.h"

namespace fairwindsk::ui::bottombar {
    AutopilotBar::AutopilotBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AutopilotBar) {
        ui->setupUi(this);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        int rudderMin = -30;
        int rudderMax = 30;
        int rudderStep = 5;
        int columnSpan = 1+(abs(rudderMin) + abs(rudderMax))/rudderStep;

        m_slider = new QSlider(Qt::Horizontal, ui->widget_Rudder);
        m_slider->setRange(rudderMin, rudderMax);
        m_slider->setTickInterval(rudderStep);
        m_slider->setEnabled(false);
        m_slider->setTickPosition(QSlider::TicksBelow);


        auto *layout = new QGridLayout();
        layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(m_slider, 0, 0, 1, columnSpan);
        int col=0;
        for (auto i = rudderMin; i<=rudderMax;i=i+rudderStep) {
            auto s = QString::number(i);
            if (i>0) s = "+" + s;
            auto *labelTick = new QLabel( s,  ui->widget_Rudder);
            /*
            if (i % 2) {
                labelTick->setStyleSheet("background-color:blue");
            }
             */
            if (i<0) {
                labelTick->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            } else if (i>0) {
                labelTick->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else {
                labelTick->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            }
            layout->addWidget(labelTick, 1, col, 1, 1);
            col++;
        }

        ui->widget_Rudder->setLayout( layout);


        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &AutopilotBar::onHideClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_StandBy, &QToolButton::clicked, this, &AutopilotBar::onStandByClicked);
        connect(ui->toolButton_Auto, &QToolButton::clicked, this, &AutopilotBar::onAutoClicked);
        connect(ui->toolButton_Route, &QToolButton::clicked, this, &AutopilotBar::onRouteClicked);
        connect(ui->toolButton_Wind, &QToolButton::clicked, this, &AutopilotBar::onWindClicked);
        connect(ui->toolButton_PTack, &QToolButton::clicked, this, &AutopilotBar::onPortTackClicked);
        connect(ui->toolButton_STack, &QToolButton::clicked, this, &AutopilotBar::onStarboardTackClicked);
        connect(ui->toolButton_Plus1, &QToolButton::clicked, this, &AutopilotBar::onPlus1Clicked);
        connect(ui->toolButton_Plus10, &QToolButton::clicked, this, &AutopilotBar::onPlus10Clicked);
        connect(ui->toolButton_Minus1, &QToolButton::clicked, this, &AutopilotBar::onMinus1Clicked);
        connect(ui->toolButton_Minus10, &QToolButton::clicked, this, &AutopilotBar::onMinus10Clicked);
        connect(ui->toolButton_NextWPT, &QToolButton::clicked, this, &AutopilotBar::onNextWPTClicked);
        connect(ui->toolButton_Steer, &QToolButton::clicked, this, &AutopilotBar::onSteerClicked);

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
            if (m_signalkPaths.contains("rsa") && m_signalkPaths["rsa"].is_string()) {

                // Subscribe and update
                updateRSA(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["rsa"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateRSA)
                        ));
            }

            // Check if the Options object has the target heading  key and if it is a string
            if (m_signalkPaths.contains("autopilot.target.heading") && m_signalkPaths["autopilot.target.heading"].is_string()) {

                // Subscribe and update
                updateTargetHeading(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["autopilot.target.heading"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateTargetHeading())
                        ));
            }

            // Check if the Options object has the target state key and if it is a string
            if (m_signalkPaths.contains("autopilot.state") && m_signalkPaths["autopilot.state"].is_string()) {

                // Subscribe and update
                updateTargetHeading(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["autopilot.state"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateState())
                        ));
            }

            // Check if the Options object has the target mode key and if it is a string
            if (m_signalkPaths.contains("autopilot.mode") && m_signalkPaths["autopilot.mode"].is_string()) {

                // Subscribe and update
                updateTargetHeading(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["autopilot.mode"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateMode())
                        ));
            }
        }
    }

    /*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the rudder angle
 */
    void AutopilotBar::updateRSA(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->widget_Rudder->setVisible(false);
        } else {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Set the slider value
            m_slider->setValue(static_cast<int>(value));

            // Build the formatted value
            //QString text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            //ui->label_RudderAngle->setText(text);

            if (!ui->widget_Rudder->isVisible()) {
                ui->widget_Rudder->setVisible(true);
            }
        }
    }

    /*
 * updateState
 * Method called in accordance to signalk to update the autopilot state
 */
    void AutopilotBar::updateState(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getStringFromUpdateByPath(update);

        if (value.isEmpty()) {
            ui->label_State->setVisible(false);
        } else {
            // Convert rad to deg
            //value = m_units->convert("rad","deg", value);

            // Build the formatted value
            QString text = value; // m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_State->setText(text);

            if (!ui->label_State->isVisible()) {
                ui->label_State->setVisible(true);
            }
        }

    }

    /*
 * updateState
 * Method called in accordance to signalk to update the autopilot mode
 */
    void AutopilotBar::updateMode(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getStringFromUpdateByPath(update);

        if (value.isEmpty()) {
            ui->label_Mode->setVisible(false);
        } else {
            // Convert rad to deg
            //value = m_units->convert("rad","deg", value);

            // Build the formatted value
            QString text = value; // m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_Mode->setText(text);

            if (!ui->label_Mode->isVisible()) {
                ui->label_Mode->setVisible(true);
            }
        }

    }

    /*
 * updateTargetHeading
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void AutopilotBar::updateTargetHeading(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {
            return;
        }

        // Get the value
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);

        if (std::isnan(value)) {
            ui->label_TargetHeading->setVisible(false);
        } else {
            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            QString text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_TargetHeading->setText(text);

            if (!ui->label_TargetHeading->isVisible()) {
                ui->label_TargetHeading->setVisible(true);
            }
        }
    }

    void AutopilotBar::onStandByClicked() {
        setState("standby");
    }

    void AutopilotBar::onAutoClicked() {
        setState("auto");
    }

    void AutopilotBar::onWindClicked() {
        setState("wind");
    }

    void AutopilotBar::onRouteClicked() {
        setState("route");
    }

    void AutopilotBar::onPortTackClicked() {
        tack("port");
    }

    void AutopilotBar::onStarboardTackClicked() {
        tack("starboard");
    }

    void AutopilotBar::onPlus1Clicked(){
        adjustHeading(1);
    }

    void AutopilotBar::onPlus10Clicked(){
        adjustHeading(10);
    }

    void AutopilotBar::onMinus1Clicked() {
        adjustHeading(-1);
    }

    void AutopilotBar::onMinus10Clicked() {
        adjustHeading(-10);
    }

    void AutopilotBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    void AutopilotBar::onNextWPTClicked() {
        advanceWaypoint(1);
    }

    void AutopilotBar::onSteerClicked() {
        setState("auto");
    }

    QJsonObject AutopilotBar::setState(const QString& state) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.state"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",QJsonValue(state));
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::setMode(const QString& mode) {
       auto path = QString::fromStdString(m_signalkPaths["autopilot.mode"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",QJsonValue(mode));
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::tack(const QString& value) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.actions.tack"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",QJsonValue(value));
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::advanceWaypoint(int value) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.actions.advanceWaypoint"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",value);
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::setTargetHeading(int value) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.target.heading"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",value);
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::setTargetWindAngle(float value) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.target.windAngle"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",value);
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    QJsonObject AutopilotBar::adjustHeading(const int value) {
        auto path = QString::fromStdString(m_signalkPaths["autopilot.actions.adjustHeading"].get<std::string>());
        QJsonObject jsonPut;
        jsonPut.insert("path",QJsonValue(path));
        jsonPut.insert("value",value);
        QJsonObject jsonUpdate;
        jsonUpdate.insert("put",jsonPut);
        FairWindSK::getInstance()->getSignalKClient()->sendMessage(jsonUpdate);
        return jsonUpdate;
    }

    AutopilotBar::~AutopilotBar() {

        if (m_slider)
        {
            delete m_slider;
            m_slider = nullptr;
        }

        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }
} // fairwindsk::ui::autopilot
