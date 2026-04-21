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
#include "ui_AnchorBar.h"


namespace fairwindsk::ui::bottombar {
    namespace {
        enum class MetricFreshnessState {
            Live,
            Stale,
            Missing
        };

        MetricFreshnessState signalKMetricFreshnessState(const bool hasValue) {
            if (!hasValue) {
                return MetricFreshnessState::Missing;
            }

            const auto *client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
            if (!client) {
                return MetricFreshnessState::Stale;
            }

            return client->connectionHealthState() == fairwindsk::signalk::Client::ConnectionHealthState::Live
                       ? MetricFreshnessState::Live
                       : MetricFreshnessState::Stale;
        }

        QString metricTooltip(const QString &title, const MetricFreshnessState state) {
            QString stateText = AnchorBar::tr("Missing");
            if (state == MetricFreshnessState::Live) {
                stateText = AnchorBar::tr("Live");
            } else if (state == MetricFreshnessState::Stale) {
                stateText = AnchorBar::tr("Stale");
            }

            return AnchorBar::tr("%1: %2").arg(title, stateText);
        }

        void applyMetricPresentation(QLabel *label,
                                     QLabel *unitLabel,
                                     QWidget *container,
                                     const QString &title,
                                     const QString &text,
                                     const MetricFreshnessState state) {
            if (!label || !container) {
                return;
            }

            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
            const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, label->palette(), false);
            const auto status = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, label->palette());

            QColor color = chrome.text;
            QFont font = label->font();
            font.setItalic(false);
            font.setBold(false);

            switch (state) {
                case MetricFreshnessState::Live:
                    break;
                case MetricFreshnessState::Stale:
                    color = status.warningFill;
                    font.setBold(true);
                    break;
                case MetricFreshnessState::Missing:
                    color = chrome.disabledText;
                    font.setItalic(true);
                    break;
            }

            label->setFont(font);
            label->setText(state == MetricFreshnessState::Missing ? QStringLiteral("--") : text);
            label->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(color.name()));
            const QString tooltip = metricTooltip(title, state);
            label->setToolTip(tooltip);
            if (unitLabel) {
                unitLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }")
                                             .arg((state == MetricFreshnessState::Missing ? chrome.disabledText : color).name()));
                unitLabel->setToolTip(tooltip);
            }
            container->setToolTip(tooltip);
            container->setVisible(true);
        }
    }

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
            const auto configuration = FairWindSK::getInstance()->getConfiguration();
            ui->label_Latitude->setText(
                fairwindsk::ui::geo::formatSingleCoordinate(
                    value.latitude(),
                    true,
                    configuration->getCoordinateFormat()));
            ui->label_Longitude->setText(
                fairwindsk::ui::geo::formatSingleCoordinate(
                    value.longitude(),
                    false,
                    configuration->getCoordinateFormat()));

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
        applyMetricPresentation(ui->label_Depth, ui->label_unitDepth, ui->widget_Depth,
                                tr("Anchor depth"), text, signalKMetricFreshnessState(hasValue));
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
        applyMetricPresentation(ui->label_Bearing, ui->label_unitBearing, ui->widget_Bearing,
                                tr("Anchor bearing"), text, signalKMetricFreshnessState(hasValue));
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
        applyMetricPresentation(ui->label_Distance, ui->label_unitDistance, ui->widget_Distance,
                                tr("Anchor distance"), text, signalKMetricFreshnessState(hasValue));
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
        applyMetricPresentation(ui->label_Rode, ui->label_unitRode, ui->widget_Rode,
                                tr("Anchor rode"), text, signalKMetricFreshnessState(hasValue));
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
        applyMetricPresentation(ui->label_CurrentRadius, ui->label_unitCurrentRadius, ui->widget_Radius,
                                tr("Anchor radius"), text, signalKMetricFreshnessState(hasValue));
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
        applyMetricPresentation(ui->label_MaxRadius, ui->label_unitMaxRadius, ui->widget_Radius,
                                tr("Anchor max radius"), text, signalKMetricFreshnessState(hasValue));
    }



    AnchorBar::~AnchorBar() {
        if (ui) {
            delete ui;
            ui = nullptr;
        }

    }
} // fairwindsk::ui::bottombar
