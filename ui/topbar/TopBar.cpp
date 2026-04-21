//
// Created by Raffaele Montella on 12/04/21.
//

#include <QTimer>
#include <QAbstractButton>
#include <QFrame>
#include <QGeoCoordinate>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QToolButton>
#include <QString>
#include <QFontMetrics>
#include <QLabel>
#include <QPixmap>
#include <nlohmann/json.hpp>

#include "TopBar.hpp"
#include "../bottombar/BottomBar.hpp"
#include "../web/Web.hpp"
#include "ui/IconUtils.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/layout/BarLayout.hpp"
#include <signalk/Waypoint.hpp>

namespace fairwindsk::ui::topbar {
    namespace {
        enum class MetricFreshnessState {
            Live,
            Stale,
            Missing
        };

        QString configuredMetricTitle(const QString &title, const bool pathConfigured) {
            return pathConfigured
                       ? title
                       : TopBar::tr("%1 (not configured)").arg(title);
        }

        QString chromeToolButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors) {
            return QStringLiteral(
                "QToolButton {"
                " background: transparent;"
                " border: none;"
                " padding: 6px;"
                " color: %1;"
                " }"
                "QToolButton:hover { background: %2; border-radius: 8px; color: %1; }"
                "QToolButton:pressed { background: %3; border-radius: 8px; color: %4; }"
                "QToolButton:disabled { color: %5; }")
                .arg(colors.transparentIcon.name(),
                     colors.transparentHoverBackground.name(QColor::HexArgb),
                     fairwindsk::ui::comfortAlpha(colors.accentBottom, 84).name(QColor::HexArgb),
                     colors.accentText.name(),
                     colors.disabledText.name());
        }

        QString comfortViewIconPath(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == "sunrise") {
                preset = QStringLiteral("dawn");
            }
            if (preset == "default" || preset.isEmpty()) {
                return QStringLiteral(":/resources/svg/OpenBridge/comfort-default.svg");
            }
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
            return QStringLiteral(":/resources/svg/OpenBridge/comfort-default.svg");
        }

        MetricFreshnessState signalKMetricFreshnessState(const bool hasValue, const bool pathConfigured = true) {
            if (!pathConfigured) {
                return MetricFreshnessState::Missing;
            }
            if (!hasValue) {
                return MetricFreshnessState::Missing;
            }

            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
            if (!client) {
                return MetricFreshnessState::Stale;
            }

            return client->connectionHealthState() == fairwindsk::signalk::Client::ConnectionHealthState::Live
                       ? MetricFreshnessState::Live
                       : MetricFreshnessState::Stale;
        }

        QString signalKMetricTooltip(const QString &title, const MetricFreshnessState state, const bool pathConfigured = true) {
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
            QString statusLine;
            if (!pathConfigured) {
                statusLine = TopBar::tr("Not configured");
            } else {
                switch (state) {
                    case MetricFreshnessState::Live:
                        statusLine = TopBar::tr("Live");
                        break;
                    case MetricFreshnessState::Stale:
                        statusLine = TopBar::tr("Stale");
                        break;
                    case MetricFreshnessState::Missing:
                        statusLine = TopBar::tr("Missing");
                        break;
                }
            }

            QString tooltip = TopBar::tr("%1: %2").arg(title, statusLine);
            if (client && client->lastStreamUpdate().isValid()) {
                tooltip += TopBar::tr("\nLast live update %1")
                               .arg(client->lastStreamUpdate().toLocalTime().toString(QStringLiteral("dd-MM hh:mm:ss")));
            }
            return tooltip;
        }

        void applyMetricPresentation(QWidget *widget,
                                     QLabel *valueLabel,
                                     QLabel *unitLabel,
                                     const QString &title,
                                     const QString &text,
                                     const MetricFreshnessState state,
                                     const bool showWhenMissing = true,
                                     const bool pathConfigured = true) {
            if (!widget || !valueLabel) {
                return;
            }

            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
            const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, valueLabel->palette(), false);
            const auto status = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, valueLabel->palette());

            QColor textColor = chrome.text;
            QFont font = valueLabel->font();
            font.setItalic(false);
            font.setBold(false);

            switch (state) {
                case MetricFreshnessState::Live:
                    textColor = chrome.text;
                    break;
                case MetricFreshnessState::Stale:
                    textColor = status.warningFill;
                    font.setBold(true);
                    break;
                case MetricFreshnessState::Missing:
                    textColor = chrome.disabledText;
                    font.setItalic(true);
                    break;
            }

            valueLabel->setFont(font);
            valueLabel->setText(state == MetricFreshnessState::Missing ? QStringLiteral("--") : text);
            valueLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(textColor.name()));
            valueLabel->setToolTip(signalKMetricTooltip(title, state, pathConfigured));

            if (unitLabel) {
                unitLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }")
                                             .arg((state == MetricFreshnessState::Missing ? chrome.disabledText : textColor).name()));
                unitLabel->setToolTip(valueLabel->toolTip());
            }

            widget->setToolTip(valueLabel->toolTip());
            widget->setVisible(showWhenMissing || state != MetricFreshnessState::Missing || !pathConfigured);
        }

        void applyDateMetricPresentation(QWidget *widget,
                                         QLabel *valueLabel,
                                         const QString &title,
                                         const QDateTime &value,
                                         const QString &format) {
            const bool hasValue = value.isValid() && !value.isNull();
            const QString text = hasValue ? value.toLocalTime().toString(format) : QString();
            applyMetricPresentation(widget,
                                    valueLabel,
                                    nullptr,
                                    title,
                                    text,
                                    signalKMetricFreshnessState(hasValue));
        }
    }

    TopBar *TopBar::instance() {
        return s_instance;
    }

    QWidget *TopBar::createContextWidget() {
        auto *container = new QWidget(this);
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        layout->addWidget(ui->label_ApplicationName, 0, Qt::AlignVCenter);
        layout->addWidget(ui->toolButton_UR, 0, Qt::AlignVCenter);
        m_contextLayout = layout;
        return container;
    }

    QWidget *TopBar::createSeparatorWidget() {
        auto *line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Plain);
        line->setLineWidth(1);
        m_dynamicLayoutWidgets.append(line);
        return line;
    }

    void TopBar::clearConfiguredLayout() {
        if (!ui || !ui->horizontalLayout) {
            return;
        }

        while (ui->horizontalLayout->count() > 0) {
            auto *item = ui->horizontalLayout->takeAt(0);
            delete item;
        }

        for (auto &widget : m_dynamicLayoutWidgets) {
            if (widget) {
                widget->deleteLater();
            }
        }
        m_dynamicLayoutWidgets.clear();
    }

    QWidget *TopBar::widgetForItemId(const QString &itemId) const {
        if (itemId == QStringLiteral("position")) {
            return ui->widget_POS;
        }
        if (itemId == QStringLiteral("cog")) {
            return ui->widget_COG;
        }
        if (itemId == QStringLiteral("sog")) {
            return ui->widget_SOG;
        }
        if (itemId == QStringLiteral("hdg")) {
            return ui->widget_HDG;
        }
        if (itemId == QStringLiteral("stw")) {
            return ui->widget_STW;
        }
        if (itemId == QStringLiteral("dpt")) {
            return ui->widget_DPT;
        }
        if (itemId == QStringLiteral("current_context")) {
            return m_contextWidget;
        }
        if (itemId == QStringLiteral("wpt")) {
            return ui->widget_WPT;
        }
        if (itemId == QStringLiteral("btw")) {
            return ui->widget_BTW;
        }
        if (itemId == QStringLiteral("dtg")) {
            return ui->widget_DTG;
        }
        if (itemId == QStringLiteral("ttg")) {
            return ui->widget_TTG;
        }
        if (itemId == QStringLiteral("eta")) {
            return ui->widget_ETA;
        }
        if (itemId == QStringLiteral("xte")) {
            return ui->widget_XTE;
        }
        if (itemId == QStringLiteral("vmg")) {
            return ui->widget_VMG;
        }
        if (itemId == QStringLiteral("clock_icons")) {
            return ui->widget_ClockBlock;
        }

        return nullptr;
    }

    void TopBar::rebuildLayout() {
        if (!ui || !ui->horizontalLayout) {
            return;
        }

        clearConfiguredLayout();
        ui->horizontalLayout->addWidget(ui->toolButton_UL, 0, Qt::AlignVCenter);

        const auto entries = fairwindsk::ui::layout::entriesForBar(
            FairWindSK::getInstance()->getConfiguration()->getRoot(),
            fairwindsk::ui::layout::BarId::Top);
        for (const auto &entry : entries) {
            if (!entry.enabled) {
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Separator) {
                ui->horizontalLayout->addWidget(createSeparatorWidget(), 0, Qt::AlignVCenter);
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Stretch) {
                ui->horizontalLayout->addStretch(1);
                continue;
            }

            QWidget *widget = widgetForItemId(entry.widgetId);
            if (!widget && fairwindsk::ui::bottombar::BottomBar::instance()) {
                widget = fairwindsk::ui::bottombar::BottomBar::instance()->widgetForItemId(entry.widgetId);
            }
            if (!widget) {
                continue;
            }

            ui->horizontalLayout->addWidget(widget, 0, Qt::AlignVCenter);
        }
    }

    void TopBar::renderNumericMetric(const MetricRenderTarget &target,
                                     const QString &title,
                                     const QString &path,
                                     const QJsonObject &update,
                                     const QString &sourceUnit,
                                     const QString &targetUnit) {
        const bool pathConfigured = !path.trimmed().isEmpty();
        const auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update);
        const bool hasValue = pathConfigured && !update.isEmpty() && !std::isnan(value);
        const QString text = hasValue ? m_units->formatSignalKValue(path, value, sourceUnit, targetUnit) : QString();
        applyMetricPresentation(target.container,
                                target.valueLabel,
                                target.unitLabel,
                                configuredMetricTitle(title, pathConfigured),
                                text,
                                signalKMetricFreshnessState(hasValue, pathConfigured),
                                true,
                                pathConfigured);
    }

    void TopBar::renderAngularMetric(const MetricRenderTarget &target,
                                     const QString &title,
                                     const QString &path,
                                     const QJsonObject &update) {
        renderNumericMetric(target, title, path, update, QStringLiteral("rad"), QStringLiteral("deg"));
    }

    void TopBar::renderDateMetric(const MetricRenderTarget &target,
                                  const QString &title,
                                  const QJsonObject &update,
                                  const QString &format) {
        const bool hasValue = !update.isEmpty();
        const auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(update);
        applyMetricPresentation(target.container,
                                target.valueLabel,
                                target.unitLabel,
                                title,
                                (hasValue && value.isValid() && !value.isNull()) ? value.toLocalTime().toString(format) : QString(),
                                signalKMetricFreshnessState(hasValue && value.isValid() && !value.isNull()),
                                true,
                                true);
    }

    void TopBar::renderWaypointMetric(const MetricRenderTarget &target,
                                      const QString &title,
                                      const QJsonObject &update) {
        QString text;
        bool hasValue = false;
        const auto value = fairwindsk::signalk::Client::getObjectFromUpdateByPath(update);
        if (value.contains("href") && value["href"].isString()) {
            const auto href = value["href"].toString();
            auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref(href);
            text = waypoint.getName().trimmed();
            hasValue = !text.isEmpty();
        }

        applyMetricPresentation(target.container,
                                target.valueLabel,
                                target.unitLabel,
                                title,
                                text,
                                signalKMetricFreshnessState(hasValue),
                                true,
                                true);
    }

    void TopBar::renderPositionMetric(const MetricRenderTarget &target,
                                      const QString &title,
                                      const QJsonObject &update) {
        const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(update);
        const bool hasValue = value.isValid();
        const QString text = hasValue
                                 ? fairwindsk::ui::geo::formatCoordinate(
                                       value,
                                       FairWindSK::getInstance()->getConfiguration()->getCoordinateFormat())
                                 : QString();
        applyMetricPresentation(target.container,
                                target.valueLabel,
                                target.unitLabel,
                                title,
                                text,
                                signalKMetricFreshnessState(hasValue),
                                true,
                                true);
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
        s_instance = this;

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chromeColors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        // Get units converter instance
        m_units = Units::getInstance();

        m_contextWidget = createContextWidget();
        ui->line_1->hide();
        ui->line_2->hide();
        ui->line_3->hide();

        ui->toolButton_UL->setIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png")));
        ui->toolButton_UL->setIconSize(QSize(32, 32));
        ui->toolButton_UL->setAutoRaise(true);
        ui->toolButton_UL->setStyleSheet(chromeToolButtonStyle(chromeColors));

        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(32, 32));
        ui->toolButton_UR->setAutoRaise(true);
        ui->toolButton_UR->setStyleSheet(chromeToolButtonStyle(chromeColors));

        if (ui->widget_SignalKStatusIcons) {
            auto *statusLayout = new QHBoxLayout(ui->widget_SignalKStatusIcons);
            statusLayout->setContentsMargins(0, 0, 0, 0);
            statusLayout->setSpacing(0);
            m_signalKStatusIcons = new fairwindsk::ui::widgets::SignalKStatusIconsWidget(ui->widget_SignalKStatusIcons);
            statusLayout->addWidget(m_signalKStatusIcons, 0, Qt::AlignVCenter);
        }
        
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
        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged, this,
                    [this](const fairwindsk::signalk::Client::ConnectionHealthState,
                           const QString &,
                           const QDateTime &,
                           const QString &) {
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
                    });
        }

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
        rebuildLayout();
    }

    void TopBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);

        if (event && event->type() == QEvent::FontChange) {
            refreshMetricLabelWidths();
        }

        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            updateComfortViewIcon();
            if (m_signalKStatusIcons) {
                m_signalKStatusIcons->refreshFromConfiguration();
            }
        }
    }

    void TopBar::updateComfortViewIcon() const {
        if (!ui || !ui->label_ComfortViewIcon) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset() : QStringLiteral("default");
        const QColor fallbackColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Window),
            {palette().color(QPalette::WindowText),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::Text)});
        const QColor iconColor = fairwindsk::ui::comfortIconColor(
            fairWindSK ? fairWindSK->getConfiguration() : nullptr,
            preset,
            fallbackColor);
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
        rebuildLayout();
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
        if (m_signalKStatusIcons) {
            m_signalKStatusIcons->refreshFromConfiguration();
        }
    }

/*
 * updateNavigationPosition
 * Method called in accordance to signalk to update the navigation position
 */
    void TopBar::updatePOS(const QJsonObject &update) {
        m_lastPosUpdate = update;
        renderPositionMetric({ui->widget_POS, ui->label_POS, nullptr}, tr("Position"), update);
    }


    /*
 * updateNavigationCourseOverGroundTrue
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void TopBar::updateCOG(const QJsonObject &update) {
        m_lastCogUpdate = update;
        renderAngularMetric({ui->widget_COG, ui->label_COG, ui->label_unitCOG},
                            tr("Course over ground"),
                            m_pathCOG,
                            update);
    }

/*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateSOG(const QJsonObject &update) {
        m_lastSogUpdate = update;
        renderNumericMetric({ui->widget_SOG, ui->label_SOG, ui->label_unitSOG},
                            tr("Speed over ground"),
                            m_pathSOG,
                            update,
                            QStringLiteral("ms-1"),
                            FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits());
    }

    /*
 * updateNavigationCourseOverGroundTrue
 * Method called in accordance to signalk to update the navigation course over ground
 */
    void TopBar::updateHDG(const QJsonObject &update) {
        m_lastHdgUpdate = update;
        renderAngularMetric({ui->widget_HDG, ui->label_HDG, ui->label_unitHDG},
                            tr("Heading"),
                            m_pathHDG,
                            update);
    }

/*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateSTW(const QJsonObject &update) {
        m_lastStwUpdate = update;
        renderNumericMetric({ui->widget_STW, ui->label_STW, ui->label_unitSTW},
                            tr("Speed through water"),
                            m_pathSTW,
                            update,
                            QStringLiteral("ms-1"),
                            FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits());
    }

    /*
 * updateNavigationSpeedOverGround
 * Method called in accordance to signalk to update the navigation speed over ground
 */
    void TopBar::updateDPT(const QJsonObject &update) {
        m_lastDptUpdate = update;
        renderNumericMetric({ui->widget_DPT, ui->label_DPT, ui->label_unitDPT},
                            tr("Depth"),
                            m_pathDPT,
                            update,
                            QStringLiteral("mt"),
                            FairWindSK::getInstance()->getConfiguration()->getDepthUnits());
    }

    void TopBar::updateWPT(const QJsonObject &update) {
        m_lastWptUpdate = update;
        renderWaypointMetric({ui->widget_WPT, ui->label_WPT, nullptr}, tr("Waypoint"), update);
    }

    void TopBar::updateBTW(const QJsonObject &update) {
        m_lastBtwUpdate = update;
        renderAngularMetric({ui->widget_BTW, ui->label_BTW, ui->label_unitBTW},
                            tr("Bearing to waypoint"),
                            m_pathBTW,
                            update);
    }

    void TopBar::updateDTG(const QJsonObject &update) {
        m_lastDtgUpdate = update;
        renderNumericMetric({ui->widget_DTG, ui->label_DTG, ui->label_unitDTG},
                            tr("Distance to go"),
                            m_pathDTG,
                            update,
                            QStringLiteral("m"),
                            FairWindSK::getInstance()->getConfiguration()->getDistanceUnits());
    }

    void TopBar::updateTTG(const QJsonObject &update) {
        m_lastTtgUpdate = update;
        renderDateMetric({ui->widget_TTG, ui->label_TTG, nullptr},
                         tr("Time to go"),
                         update,
                         QStringLiteral("hh:mm"));
    }

    void TopBar::updateETA(const QJsonObject &update) {
        m_lastEtaUpdate = update;
        renderDateMetric({ui->widget_ETA, ui->label_ETA, nullptr},
                         tr("Estimated time of arrival"),
                         update,
                         QStringLiteral("dd-MM-yyyy hh:mm"));
    }

    void TopBar::updateXTE(const QJsonObject &update) {
        m_lastXteUpdate = update;
        renderNumericMetric({ui->widget_XTE, ui->label_XTE, ui->label_unitXTE},
                            tr("Cross track error"),
                            m_pathXTE,
                            update,
                            QStringLiteral("m"),
                            FairWindSK::getInstance()->getConfiguration()->getDistanceUnits());
    }

    void TopBar::updateVMG(const QJsonObject &update) {
        m_lastVmgUpdate = update;
        renderNumericMetric({ui->widget_VMG, ui->label_VMG, ui->label_unitVMG},
                            tr("Velocity made good"),
                            m_pathVMG,
                            update,
                            QStringLiteral("ms-1"),
                            FairWindSK::getInstance()->getConfiguration()->getVesselSpeedUnits());
    }

    void TopBar::setCurrentApp(AppItem *appItem) {
        m_currentApp = appItem;
        if (m_currentApp) {
            auto *widget = m_currentApp->getWidget();
            ui->toolButton_UR->setIcon(m_currentApp->getIcon());
            ui->toolButton_UR->setIconSize(QSize(32, 32));
            ui->label_ApplicationName->setText(m_currentApp->getDisplayName());
            ui->label_ApplicationName->setToolTip(m_currentApp->getDescription());
            ui->toolButton_UR->setEnabled(qobject_cast<fairwindsk::ui::web::Web *>(widget) != nullptr);
        } else {
            resetCurrentAppPresentation();
        }
    }

    void TopBar::setCurrentAppStatusSummary(const QString &summary) {
        if (!m_currentApp) {
            return;
        }

        const QString baseTooltip = m_currentApp->getDescription();
        ui->label_ApplicationName->setToolTip(
            summary.trimmed().isEmpty() ? baseTooltip : tr("%1\n%2").arg(baseTooltip, summary.trimmed()));
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
        if (s_instance == this) {
            s_instance = nullptr;
        }

        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        delete ui;
    }
}
