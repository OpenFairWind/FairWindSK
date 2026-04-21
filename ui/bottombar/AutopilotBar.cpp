//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Autopilot.h" resolved

#include <QtWidgets/QAbstractButton>
#include "AutopilotBar.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QToolButton>

#include "FairWindSK.hpp"
#include "runtime/DiagnosticsSupport.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/SignalKMetricState.hpp"
#include "ui_AutopilotBar.h"

namespace fairwindsk::ui::bottombar {
    namespace {
        constexpr auto kDefaultAutopilotId = "_default";

        void setOpenBridgeIcon(QToolButton *button, const QString &resourcePath) {
            if (!button || resourcePath.isEmpty()) {
                return;
            }
            button->setIcon(QIcon(resourcePath));
        }
    }

    AutopilotBar::AutopilotBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AutopilotBar) {
        ui->setupUi(this);

        // Not visible by default
        QWidget::setVisible(false);

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
        connect(ui->toolButton_Route, &QToolButton::clicked, this, &AutopilotBar::onRouteClicked);
        connect(ui->toolButton_Wind, &QToolButton::clicked, this, &AutopilotBar::onWindClicked);
        connect(ui->toolButton_PTack, &QToolButton::clicked, this, &AutopilotBar::onPortTackClicked);
        connect(ui->toolButton_STack, &QToolButton::clicked, this, &AutopilotBar::onStarboardTackClicked);
        connect(ui->toolButton_PGybe, &QToolButton::clicked, this, &AutopilotBar::onPortGybeClicked);
        connect(ui->toolButton_SGybe, &QToolButton::clicked, this, &AutopilotBar::onStarboardGybeClicked);
        connect(ui->toolButton_Plus1, &QToolButton::clicked, this, &AutopilotBar::onPlus1Clicked);
        connect(ui->toolButton_Plus10, &QToolButton::clicked, this, &AutopilotBar::onPlus10Clicked);
        connect(ui->toolButton_Minus1, &QToolButton::clicked, this, &AutopilotBar::onMinus1Clicked);
        connect(ui->toolButton_Minus10, &QToolButton::clicked, this, &AutopilotBar::onMinus10Clicked);
        connect(ui->toolButton_NextWPT, &QToolButton::clicked, this, &AutopilotBar::onNextWPTClicked);
        connect(ui->toolButton_Dodge, &QToolButton::clicked, this, &AutopilotBar::onDodgeClicked);
        connect(ui->toolButton_Auto, &QToolButton::clicked, this, &AutopilotBar::onAutoClicked);
        ui->label_Port->hide();
        ui->label_Starboard->hide();

        setOpenBridgeIcon(ui->toolButton_StandBy, QStringLiteral(":/resources/svg/OpenBridge/close-google.svg"));
        setOpenBridgeIcon(ui->toolButton_Auto, QStringLiteral(":/resources/svg/OpenBridge/command-autopilot.svg"));
        setOpenBridgeIcon(ui->toolButton_Route, QStringLiteral(":/resources/svg/OpenBridge/navigation-route.svg"));
        setOpenBridgeIcon(ui->toolButton_Wind, QStringLiteral(":/resources/svg/OpenBridge/up-iec.svg"));
        setOpenBridgeIcon(ui->toolButton_PTack, QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        setOpenBridgeIcon(ui->toolButton_STack, QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg"));
        setOpenBridgeIcon(ui->toolButton_PGybe, QStringLiteral(":/resources/svg/OpenBridge/chevron-double-left-google.svg"));
        setOpenBridgeIcon(ui->toolButton_SGybe, QStringLiteral(":/resources/svg/OpenBridge/chevron-double-right-google.svg"));
        setOpenBridgeIcon(ui->toolButton_Minus1, QStringLiteral(":/resources/svg/OpenBridge/spinbox-minus.svg"));
        setOpenBridgeIcon(ui->toolButton_Minus10, QStringLiteral(":/resources/svg/OpenBridge/chevron-double-left-google.svg"));
        setOpenBridgeIcon(ui->toolButton_Plus1, QStringLiteral(":/resources/svg/OpenBridge/spinbox-plus.svg"));
        setOpenBridgeIcon(ui->toolButton_Plus10, QStringLiteral(":/resources/svg/OpenBridge/chevron-double-right-google.svg"));
        setOpenBridgeIcon(ui->toolButton_NextWPT, QStringLiteral(":/resources/svg/OpenBridge/waypoint-optional-iec.svg"));
        setOpenBridgeIcon(ui->toolButton_Dodge, QStringLiteral(":/resources/svg/OpenBridge/alerts.svg"));


        // Get the configuration json object
        const auto configurationJsonObject = configuration->getRoot();

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
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateTargetHeading)
                        ));
            }

            // Check if the Options object has the target state key and if it is a string
            if (m_signalkPaths.contains("autopilot.state") && m_signalkPaths["autopilot.state"].is_string()) {

                // Subscribe and update
                updateState(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                        QString::fromStdString(m_signalkPaths["autopilot.state"].get<std::string>()),
                        this,
                        SLOT(fairwindsk::ui::bottombar::AutopilotBar::updateState)
                        ));
            }


        }

        refreshAutopilotOptions();
        applyComfortStyle();
        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged, this,
                    [this](const fairwindsk::signalk::Client::ConnectionHealthState,
                           const QString &,
                           const QDateTime &,
                           const QString &) {
                        updateRSA(m_lastRsaUpdate);
                        updateTargetHeading(m_lastTargetHeadingUpdate);
                        updateState(m_lastStateUpdate);
                    });
        }
    }

    void AutopilotBar::refreshFromConfiguration() {
        applyComfortStyle();
        refreshAutopilotOptions();
    }

    void AutopilotBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            applyComfortStyle();
        }
    }

    void AutopilotBar::applyComfortStyle() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), true);
        for (auto *button : findChildren<QToolButton *>()) {
            const bool isTransparentIconButton =
                button == ui->toolButton_Autopilot || button == ui->toolButton_Hide;
            fairwindsk::ui::applyBottomBarToolButtonChrome(
                button,
                colors,
                isTransparentIconButton
                    ? fairwindsk::ui::BottomBarButtonChrome::Transparent
                    : fairwindsk::ui::BottomBarButtonChrome::Flat,
                QSize(40, 40),
                88);
        }
    }

    /*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the rudder angle
 */
    void AutopilotBar::updateRSA(const QJsonObject &update) {
        m_lastRsaUpdate = update;
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        if (hasValue) {
            value = m_units->convert("rad", "deg", value);
            m_slider->setValue(static_cast<int>(value));
        }

        const auto state = fairwindsk::ui::widgets::signalKMetricState(hasValue);
        const QString detail = hasValue
                                   ? tr("Current rudder angle %1").arg(m_units->format("deg", value))
                                   : QString();
        const QString tooltip = fairwindsk::ui::widgets::signalKMetricTooltip(
            tr("Autopilot rudder angle"),
            state,
            true,
            detail);
        ui->widget_Rudder->setVisible(state != fairwindsk::ui::widgets::SignalKMetricState::Missing);
        ui->widget_Rudder->setToolTip(tooltip);
        m_slider->setEnabled(state != fairwindsk::ui::widgets::SignalKMetricState::Missing);
        m_slider->setToolTip(tooltip);
    }

    /*
 * updateState
 * Method called in accordance to signalk to update the autopilot state
 */
    void AutopilotBar::updateState(const QJsonObject &update) {
        m_lastStateUpdate = update;

        const QString value = fairwindsk::signalk::Client::getStringFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !value.isEmpty();
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_State,
            nullptr,
            ui->label_State,
            tr("Autopilot state"),
            value,
            fairwindsk::ui::widgets::signalKMetricState(hasValue),
            true,
            true,
            tr("NO PILOT"));
    }

    /*
 * updateTargetHeading
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void AutopilotBar::updateTargetHeading(const QJsonObject &update) {
        m_lastTargetHeadingUpdate = update;

        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        QString text;
        if (hasValue) {
            value = m_units->convert("rad","deg", value);
            text = m_units->format("deg", value);
        }

        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_TargetHeading,
            nullptr,
            ui->label_TargetHeading,
            tr("Autopilot target heading"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
        ui->label_TargetHeading->setVisible(true);
    }

    void AutopilotBar::onStandByClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("standby"), QStringLiteral("disengage"));
        checkStateAndUpdateUI(FairWindSK::getInstance()->getSignalKClient()->signalkPost(autopilotUrl("disengage")));
    }

    void AutopilotBar::onAutoClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("auto"), QStringLiteral("engage"));
        checkStateAndUpdateUI(FairWindSK::getInstance()->getSignalKClient()->signalkPost(autopilotUrl("engage")));
    }

    void AutopilotBar::onWindClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("mode"), QStringLiteral("wind"));
        setMode("wind");
    }

    void AutopilotBar::onRouteClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("mode"), QStringLiteral("route"));
        setMode("route");
    }

    void AutopilotBar::onPortTackClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("tack"), QStringLiteral("port"));
        tack("port");
    }

    void AutopilotBar::onStarboardTackClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("tack"), QStringLiteral("starboard"));
        tack("starboard");
    }

    void AutopilotBar::onPortGybeClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("gybe"), QStringLiteral("port"));
        gybe("port");
    }

    void AutopilotBar::onStarboardGybeClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("gybe"), QStringLiteral("starboard"));
        gybe("starboard");
    }

    void AutopilotBar::onPlus1Clicked(){
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("adjust_heading"), QStringLiteral("+1"));
        adjustHeading(1);
    }

    void AutopilotBar::onPlus10Clicked(){
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("adjust_heading"), QStringLiteral("+10"));
        adjustHeading(10);
    }

    void AutopilotBar::onMinus1Clicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("adjust_heading"), QStringLiteral("-1"));
        adjustHeading(-1);
    }

    void AutopilotBar::onMinus10Clicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("adjust_heading"), QStringLiteral("-10"));
        adjustHeading(-10);
    }

    void AutopilotBar::onHideClicked() {
        setVisible(false);
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("hide"), QStringLiteral("panel"));
        emit hidden();
    }

    void AutopilotBar::onNextWPTClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("advance_waypoint"), QStringLiteral("+1"));
        advanceWaypoint(1);
    }

    void AutopilotBar::onDodgeClicked() {
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("autopilot"), QStringLiteral("dodge"), QStringLiteral("request"));
        checkStateAndUpdateUI(FairWindSK::getInstance()->getSignalKClient()->signalkPost(autopilotUrl("dodge")));
    }

    QString AutopilotBar::autopilotBasePath() const {
        return QString("/signalk/v2/api/vessels/self/autopilots/%1").arg(kDefaultAutopilotId);
    }

    QUrl AutopilotBar::autopilotUrl(const QString &suffix) const {
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        QString path = client->server().toString() + autopilotBasePath();
        if (!suffix.isEmpty()) {
            path += "/" + suffix;
        }
        return QUrl(path);
    }

    void AutopilotBar::setAutopilotControlsEnabled(const bool enabled) {
        ui->toolButton_StandBy->setEnabled(enabled);
        ui->toolButton_NextWPT->setEnabled(enabled);
        ui->toolButton_Wind->setEnabled(enabled);
        ui->toolButton_PTack->setEnabled(enabled);
        ui->toolButton_Minus10->setEnabled(enabled);
        ui->toolButton_Minus1->setEnabled(enabled);
        ui->toolButton_Plus1->setEnabled(enabled);
        ui->toolButton_Plus10->setEnabled(enabled);
        ui->toolButton_STack->setEnabled(enabled);
        ui->toolButton_Route->setEnabled(enabled);
        ui->toolButton_Dodge->setEnabled(enabled);
        ui->toolButton_Auto->setEnabled(enabled);
    }

    void AutopilotBar::refreshAutopilotOptions() {
        const auto fairWindSK = FairWindSK::getInstance();
        const auto client = fairWindSK->getSignalKClient();
        const auto result = client->signalkGet(autopilotUrl("options"));

        m_autopilotAvailable = !result.isEmpty() && !result.contains("statusCode");
        setAutopilotControlsEnabled(m_autopilotAvailable);

        if (!m_autopilotAvailable) {
            ui->label_State->setText(tr("Autopilot unavailable"));
            ui->toolButton_Route->setVisible(true);
            ui->toolButton_Wind->setVisible(true);
            ui->toolButton_Auto->setVisible(true);
            ui->toolButton_Dodge->setVisible(true);
            return;
        }

        if (result.contains("modes") && result["modes"].isArray()) {
            const auto modes = result["modes"].toArray();
            ui->toolButton_Route->setVisible(modes.contains("route"));
            ui->toolButton_Wind->setVisible(modes.contains("wind"));
            ui->toolButton_Auto->setVisible(modes.contains("auto"));
            ui->toolButton_Dodge->setVisible(modes.contains("dodge"));
        }
    }

    QJsonObject AutopilotBar::setMode(const QString& mode) {

        // Define the result
        QJsonObject result;

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        auto payload = R"({ "value": ")" + mode + R"(" })";
        result = client->signalkPut(autopilotUrl("mode"), payload);
        checkStateAndUpdateUI(result);

        // Return the value
        return result;
    }

    QJsonObject AutopilotBar::tack(const QString& value) {

        // Set the result
        QJsonObject result;

        result = FairWindSK::getInstance()->getSignalKClient()->signalkPost(autopilotUrl("tack/" + value));
        checkStateAndUpdateUI(result);

        // Return the result
        return result;
    }

    QJsonObject AutopilotBar::gybe(const QString& value) {

        // Set the result
        QJsonObject result;

        result = FairWindSK::getInstance()->getSignalKClient()->signalkPost(autopilotUrl("gybe/" + value));
        checkStateAndUpdateUI(result);

        // Return the result
        return result;
    }

    QJsonObject AutopilotBar::advanceWaypoint(int value) {

        // Set the result
        QJsonObject result;

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        auto payload =  R"({ "value": )" + QString{"%1"}.arg(value) + R"( })";
        result = client->signalkPut(QUrl(client->server().toString()+"/signalk/v2/api/vessels/self/navigation/course/activeRoute/nextPoint"), payload);
        checkStateAndUpdateUI(result);

        // Return the result
        return result;
    }

    QJsonObject AutopilotBar::setTargetHeading(const float value) {

        // Set the result
        QJsonObject result;

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        auto payload = R"({ "units": "deg", "value": )" + QString{"%1"}.arg(value) + R"( })";
        result = client->signalkPut(autopilotUrl("target"), payload);
        checkStateAndUpdateUI(result);

        // Return the result
        return result;
    }

    QJsonObject AutopilotBar::setTargetWindAngle(const float value) {

        // Get the Signal K client
        auto client = FairWindSK::getInstance()->getSignalKClient();

        // Get the path
        auto path = "vessels.self." + QString::fromStdString(m_signalkPaths["autopilot.target.windAngle"].get<std::string>());

        // Set the payload strig
        auto payload = R"({ "value": )" + QString{"%1"}.arg(value) + R"( })";

        // Perform the PUT request
        auto result = client->signalkPut(  path, payload);

        // Return the value
        return result;
    }

    QJsonObject AutopilotBar::adjustHeading(const float value) {

        // Set the result
        QJsonObject result;

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        auto payload = R"({ "units": "deg", "value": )" + QString{"%1"}.arg(value) + R"( })";
        result = client->signalkPut(autopilotUrl("target/adjust"), payload);
        checkStateAndUpdateUI(result);

        // Return the result
        return result;
    }

    void AutopilotBar::checkStateAndUpdateUI(QJsonObject result) {

        // Check if the result has the status code
        if (result.contains("statusCode") && result["statusCode"].isDouble()) {

            // Check if the status code is 200
            if (result["statusCode"].toInt() == 200) {

                const auto client = FairWindSK::getInstance()->getSignalKClient();
                result = client->signalkGet(autopilotUrl("state"));

                // Check if a value is returned
                if (result.contains("value")) {

                    // Check if the state is not defined
                    if (result["value"].isNull()) {

                        // Update the UI
                        ui->label_State->setText("N/A");

                    } else {

                        // Check if the value is a string
                        if (result["value"].isString()) {

                            // Get the string and update the state
                            ui->label_State->setText(result["value"].toString());
                        }
                    }
                }
            } else {

                // Check if the result as a message as string
                if (result.contains("message") && result["message"].isString()) {

                    // Update the UI with the message
                    ui->label_State->setText(result["message"].toString());
                } else {

                    // Update the UI with the status code
                    ui->label_State->setText("Error: "+ QString::number(result["statusCode"].toInt()));
                }
            }
        }

        refreshAutopilotOptions();
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
