//
// Created by Raffaele Montella on 08/12/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AnchorBar.h" resolved


#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QToolButton>

#include "AnchorBar.hpp"

#include "FairWindSK.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/SignalKMetricState.hpp"
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

        updateUnitLabels();

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

        applyComfortStyle();
        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged, this,
                    [this](const fairwindsk::signalk::Client::ConnectionHealthState,
                           const QString &,
                           const QDateTime &,
                           const QString &) {
                        updatePosition(m_lastPositionUpdate);
                        updateDepth(m_lastDepthUpdate);
                        updateBearing(m_lastBearingUpdate);
                        updateDistance(m_lastDistanceUpdate);
                        updateRode(m_lastRodeUpdate);
                        updateCurrentRadius(m_lastCurrentRadiusUpdate);
                        updateMaxRadius(m_lastMaxRadiusUpdate);
                    });
        }
    }

    void AnchorBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            applyComfortStyle();
        }
    }

    void AnchorBar::applyComfortStyle() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        for (auto *button : findChildren<QToolButton *>()) {
            const bool isTransparentIconButton =
                button == ui->toolButton_Anchor || button == ui->toolButton_Hide;
            button->setToolTip(button->text());
            fairwindsk::ui::applyBottomBarToolButtonChrome(
                button,
                colors,
                isTransparentIconButton
                    ? fairwindsk::ui::BottomBarButtonChrome::Transparent
                    : fairwindsk::ui::BottomBarButtonChrome::Flat,
                QSize(40, 40),
                88);
        }

        fairwindsk::ui::applyBottomBarPushButtonChrome(ui->pushButton_SetRadius, colors, true, 52);
    }

    void AnchorBar::updateUnitLabels() const {
        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        ui->label_unitDepth->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.depth") && m_signalkPaths["anchor.depth"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.depth"].get<std::string>())
                    : QString(),
                configuration->getDepthUnits()));
        ui->label_unitBearing->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.bearing") && m_signalkPaths["anchor.bearing"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.bearing"].get<std::string>())
                    : QString(),
                "deg"));
        ui->label_unitDistance->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.distance") && m_signalkPaths["anchor.distance"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.distance"].get<std::string>())
                    : QString(),
                configuration->getDistanceUnits()));
        ui->label_unitRode->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.rode") && m_signalkPaths["anchor.rode"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.rode"].get<std::string>())
                    : QString(),
                configuration->getRangeUnits()));
        ui->label_unitCurrentRadius->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.radius") && m_signalkPaths["anchor.radius"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.radius"].get<std::string>())
                    : QString(),
                configuration->getRangeUnits()));
        ui->label_unitMaxRadius->setText(
            m_units->getSignalKUnitLabel(
                m_signalkPaths.contains("anchor.max") && m_signalkPaths["anchor.max"].is_string()
                    ? QString::fromStdString(m_signalkPaths["anchor.max"].get<std::string>())
                    : QString(),
                configuration->getRangeUnits()));
    }

    void AnchorBar::refreshFromConfiguration() {
        applyComfortStyle();
        updateUnitLabels();
        updatePosition(m_lastPositionUpdate);
        updateDepth(m_lastDepthUpdate);
        updateBearing(m_lastBearingUpdate);
        updateDistance(m_lastDistanceUpdate);
        updateRode(m_lastRodeUpdate);
        updateCurrentRadius(m_lastCurrentRadiusUpdate);
        updateMaxRadius(m_lastMaxRadiusUpdate);
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
        m_lastPositionUpdate = update;
        const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && value.isValid();

        QString latitudeText;
        QString longitudeText;
        if (hasValue) {
            const auto configuration = FairWindSK::getInstance()->getConfiguration();
            latitudeText = fairwindsk::ui::geo::formatSingleCoordinate(
                value.latitude(),
                true,
                configuration->getCoordinateFormat());
            longitudeText = fairwindsk::ui::geo::formatSingleCoordinate(
                value.longitude(),
                false,
                configuration->getCoordinateFormat());
        }

        fairwindsk::ui::widgets::applySignalKDualMetricPresentation(
            ui->label_Latitude,
            ui->label_Longitude,
            ui->widget_Position,
            tr("Anchor position"),
            latitudeText,
            longitudeText,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateDepth(const QJsonObject &update) {
        m_lastDepthUpdate = update;
        const auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        const QString text = hasValue
                                 ? m_units->formatSignalKValue(
                                       QString::fromStdString(m_signalkPaths["anchor.depth"].get<std::string>()),
                                       value,
                                       "m",
                                       FairWindSK::getInstance()->getConfiguration()->getDepthUnits())
                                 : QString();
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_Depth,
            ui->label_unitDepth,
            ui->widget_Depth,
            tr("Anchor depth"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
     * updateBearing
     * Method called in accordance to signalk to update the bearing
     */
    void AnchorBar::updateBearing(const QJsonObject &update) {
        m_lastBearingUpdate = update;
        const auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        const QString text = hasValue
                                 ? m_units->formatSignalKValue(
                                       QString::fromStdString(m_signalkPaths["anchor.bearing"].get<std::string>()),
                                       value,
                                       "rad",
                                       "deg")
                                 : QString();
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_Bearing,
            ui->label_unitBearing,
            ui->widget_Bearing,
            tr("Anchor bearing"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateDistance(const QJsonObject &update) {
        m_lastDistanceUpdate = update;
        const auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        const QString text = hasValue
                                 ? m_units->formatSignalKValue(
                                       QString::fromStdString(m_signalkPaths["anchor.distance"].get<std::string>()),
                                       value,
                                       "m",
                                       FairWindSK::getInstance()->getConfiguration()->getDistanceUnits())
                                 : QString();
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_Distance,
            ui->label_unitDistance,
            ui->widget_Distance,
            tr("Anchor distance"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateRode(const QJsonObject &update) {
        m_lastRodeUpdate = update;
        const auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        const QString text = hasValue
                                 ? m_units->formatSignalKValue(
                                       QString::fromStdString(m_signalkPaths["anchor.rode"].get<std::string>()),
                                       value,
                                       "m",
                                       FairWindSK::getInstance()->getConfiguration()->getRangeUnits())
                                 : QString();
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_Rode,
            ui->label_unitRode,
            ui->widget_Rode,
            tr("Anchor rode"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateCurrentRadius(const QJsonObject &update) {
        m_lastCurrentRadiusUpdate = update;
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        QString text;
        if (hasValue) {
            value = m_units->convertSignalKValue(
                QString::fromStdString(m_signalkPaths["anchor.radius"].get<std::string>()),
                value,
                "m",
                FairWindSK::getInstance()->getConfiguration()->getRangeUnits());
            ui->horizontalSlider_CurrentRadius->setValue(static_cast<int>(value));
            text = m_units->formatSignalKValue(
                QString::fromStdString(m_signalkPaths["anchor.radius"].get<std::string>()),
                fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update),
                "m",
                FairWindSK::getInstance()->getConfiguration()->getRangeUnits());
        }
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_CurrentRadius,
            ui->label_unitCurrentRadius,
            ui->widget_Radius,
            tr("Anchor radius"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void AnchorBar::updateMaxRadius(const QJsonObject &update) {
        m_lastMaxRadiusUpdate = update;
        auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = !update.isEmpty() && !std::isnan(value);
        QString text;
        if (hasValue) {
            value = m_units->convertSignalKValue(
                QString::fromStdString(m_signalkPaths["anchor.max"].get<std::string>()),
                value,
                "m",
                FairWindSK::getInstance()->getConfiguration()->getRangeUnits());
            ui->horizontalSlider_CurrentRadius->setMaximum(static_cast<int>(value));
            text = m_units->formatSignalKValue(
                QString::fromStdString(m_signalkPaths["anchor.max"].get<std::string>()),
                fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update),
                "m",
                FairWindSK::getInstance()->getConfiguration()->getRangeUnits());
        }
        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            ui->label_MaxRadius,
            ui->label_unitMaxRadius,
            ui->widget_Radius,
            tr("Anchor max radius"),
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue));
    }



    AnchorBar::~AnchorBar() {
        if (ui) {
            delete ui;
            ui = nullptr;
        }

    }
} // fairwindsk::ui::bottombar
