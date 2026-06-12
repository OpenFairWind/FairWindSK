//
// Created by Raffaele Montella on 12/04/21.
//

#include <algorithm>

#include <QTimer>
#include <QHBoxLayout>
#include <QToolButton>
#include <QString>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>

#include "TopBar.hpp"
#include "../bottombar/BottomBar.hpp"
#include "../web/Web.hpp"
#include "ui/IconUtils.hpp"
#include "ui/layout/BarLayout.hpp"
#include "ui/layout/BarRuntime.hpp"
#include "ui/widgets/DataWidget.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"
#include "ui/widgets/SignalKServerBox.hpp"

namespace fairwindsk::ui::topbar {
    namespace {
        QString chromeToolButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors) {
            return QStringLiteral(
                "QToolButton {"
                " background: transparent;"
                " border: none;"
                " padding: 0px;"
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

        bool isFramelessExpandedWidget(const fairwindsk::ui::layout::LayoutEntry &entry) {
            return entry.widgetId == QStringLiteral("position") ||
                   entry.widgetId == QStringLiteral("current_context") ||
                   entry.widgetId == QStringLiteral("clock_icons") ||
                   entry.widgetId == QStringLiteral("status");
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
        m_contextLayout = layout;
        return container;
    }

    QWidget *TopBar::createStatusWidget() {
        auto *container = new QWidget(this);
        container->setObjectName(QStringLiteral("widget_StatusBlock"));
        container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_compactStatusIcons = new fairwindsk::ui::widgets::SignalKStatusIconsWidget(container);
        m_compactStatusIcons->setDetailIndicatorsVisible(false);
        layout->addWidget(m_compactStatusIcons, 0, Qt::AlignCenter);

        return container;
    }

    QWidget *TopBar::widgetForItemId(const QString &itemId) const {
        if (itemId == QStringLiteral("current_context")) {
            return m_contextWidget;
        }
        if (itemId == QStringLiteral("clock_icons")) {
            return ui->widget_ClockBlock;
        }
        if (itemId == QStringLiteral("status")) {
            return m_statusWidget;
        }

        return nullptr;
    }

    void TopBar::applyEntryPresentation(const fairwindsk::ui::layout::LayoutEntry &entry,
                                        QWidget *widget) const {
        if (!widget) {
            return;
        }

        if (auto *button = qobject_cast<QToolButton *>(widget)) {
            fairwindsk::ui::layout::runtime::applyToolButtonDisplayOptions(button, entry);
        }
        if (entry.widgetId == QStringLiteral("signalk_status")) {
            if (auto *signalKServerBox = qobject_cast<fairwindsk::ui::widgets::SignalKServerBox *>(widget)) {
                signalKServerBox->setDisplayOptions(entry.showText);
            }
            return;
        }

        if (entry.widgetId == QStringLiteral("current_context")) {
            if (ui->label_ApplicationName) {
                ui->label_ApplicationName->setVisible(entry.showText);
            }
            return;
        }

        if (entry.widgetId == QStringLiteral("clock_icons")) {
            if (ui->label_DateTime) {
                ui->label_DateTime->setVisible(entry.showText);
            }
            if (ui->widget_SignalKStatusIcons) {
                ui->widget_SignalKStatusIcons->setVisible(true);
            }
            if (ui->label_ComfortViewIcon) {
                ui->label_ComfortViewIcon->setVisible(entry.showIcon);
            }
            if (m_signalKStatusIcons) {
                m_signalKStatusIcons->setVisible(true);
            }
            return;
        }

        if (entry.widgetId == QStringLiteral("status")) {
            if (m_compactStatusIcons) {
                m_compactStatusIcons->setVisible(entry.showIcon || entry.showText);
            }
        }
    }

    void TopBar::rebuildLayout() {
        if (!ui || !ui->horizontalLayout) {
            return;
        }

        m_deferredExternalWidgetResolution = false;
        clearLayoutEditHints();
        fairwindsk::ui::layout::runtime::clearConfiguredLayout(
            ui->horizontalLayout,
            m_dynamicLayoutWidgets,
            m_dataWidgets);
        ui->toolButton_UL->setVisible(true);
        ui->horizontalLayout->addWidget(ui->toolButton_UL, 0, Qt::AlignVCenter);

        const auto configRoot = FairWindSK::getInstance()->getConfiguration()->getRoot();
        m_layoutSignature = fairwindsk::ui::layout::layoutSignature(
            configRoot,
            fairwindsk::ui::layout::BarId::Top);
        const auto entries = fairwindsk::ui::layout::entriesForBar(
            configRoot,
            fairwindsk::ui::layout::BarId::Top);
        for (const auto &entry : entries) {
            if (!entry.enabled) {
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Separator) {
                fairwindsk::ui::layout::runtime::addSeparatorToLayout(
                    ui->horizontalLayout,
                    this,
                    m_dynamicLayoutWidgets,
                    entry);
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Stretch) {
                fairwindsk::ui::layout::runtime::addStretchToLayout(ui->horizontalLayout, entry);
                continue;
            }

            QString itemId = entry.widgetId;
            QWidget *widget = widgetForItemId(entry.widgetId);
            const bool dataWidgetEntry = fairwindsk::ui::widgets::isDataWidgetId(configRoot, entry.widgetId);
            if (!widget && !dataWidgetEntry && fairwindsk::ui::bottombar::BottomBar::instance()) {
                widget = fairwindsk::ui::bottombar::BottomBar::instance()->widgetForItemId(entry.widgetId);
            }
            if (!widget && !dataWidgetEntry) {
                m_deferredExternalWidgetResolution = true;
            }
            if (!widget) {
                const auto definition = fairwindsk::ui::widgets::dataWidgetDefinition(configRoot, entry.widgetId);
                widget = fairwindsk::ui::layout::runtime::createDataWidget(
                    definition,
                    entry,
                    this,
                    m_dataWidgets);
            }
            if (!widget) {
                continue;
            }

            applyEntryPresentation(entry, widget);
            fairwindsk::ui::layout::runtime::addWidgetToLayout(
                ui->horizontalLayout,
                entry,
                itemId,
                widget,
                m_baseSizePolicies);
        }

        ui->toolButton_UR->setVisible(true);
        ui->horizontalLayout->addWidget(ui->toolButton_UR, 0, Qt::AlignVCenter);
        ui->toolButton_UL->raise();
        ui->toolButton_UR->raise();
        applyLayoutEditHints(entries);
    }

    void TopBar::clearLayoutEditHints() {
        for (auto it = m_layoutHintEffects.begin(); it != m_layoutHintEffects.end(); ++it) {
            if (!it.key()) {
                continue;
            }
            if (it.key()->graphicsEffect() == it.value()) {
                it.key()->setGraphicsEffect(nullptr);
            }
            if (it.value()) {
                it.value()->deleteLater();
            }
        }
        m_layoutHintEffects.clear();
    }

    void TopBar::applyLayoutEditHints(const QList<fairwindsk::ui::layout::LayoutEntry> &entries) {
        clearLayoutEditHints();
        if (!m_layoutEditHighlightEnabled) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chromeColors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        for (const auto &entry : entries) {
            if (!entry.enabled || entry.kind != fairwindsk::ui::layout::EntryKind::Widget || !entry.expandHorizontally) {
                continue;
            }
            if (isFramelessExpandedWidget(entry)) {
                continue;
            }

            QWidget *widget = widgetForItemId(entry.widgetId);
            if (!widget) {
                widget = m_dataWidgets.value(entry.widgetId).data();
            }
            if (!widget) {
                continue;
            }

            auto *effect = new QGraphicsDropShadowEffect(widget);
            effect->setOffset(0, 0);
            effect->setBlurRadius(18);
            effect->setColor(fairwindsk::ui::comfortAlpha(chromeColors.accentTop.lighter(112), 180));
            widget->setGraphicsEffect(effect);
            m_layoutHintEffects.insert(widget, effect);
        }
    }

    void TopBar::setLayoutEditHighlightEnabled(const bool enabled) {
        if (m_layoutEditHighlightEnabled == enabled) {
            return;
        }

        m_layoutEditHighlightEnabled = enabled;
        const auto configRoot = FairWindSK::getInstance()->getConfiguration()->getRoot();
        applyLayoutEditHints(fairwindsk::ui::layout::entriesForBar(
            configRoot,
            fairwindsk::ui::layout::BarId::Top));
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

        m_contextWidget = createContextWidget();
        m_statusWidget = createStatusWidget();
        ui->horizontalLayout->setContentsMargins(4, 0, 4, 0);
        applyFramelessRuntimeChrome();

        ui->toolButton_UL->setIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png")));
        ui->toolButton_UL->setIconSize(QSize(36, 36));
        ui->toolButton_UL->setFixedSize(QSize(56, 56));
        ui->toolButton_UL->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->toolButton_UL->setAutoRaise(true);
        ui->toolButton_UL->setStyleSheet(chromeToolButtonStyle(chromeColors));

        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(36, 36));
        ui->toolButton_UR->setFixedSize(QSize(56, 56));
        ui->toolButton_UR->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->toolButton_UR->setAutoRaise(true);
        ui->toolButton_UR->setStyleSheet(chromeToolButtonStyle(chromeColors));

        if (ui->widget_SignalKStatusIcons) {
            auto *statusLayout = new QHBoxLayout(ui->widget_SignalKStatusIcons);
            statusLayout->setContentsMargins(0, 0, 0, 0);
            statusLayout->setSpacing(0);
            m_signalKStatusIcons = new fairwindsk::ui::widgets::SignalKStatusIconsWidget(ui->widget_SignalKStatusIcons);
            m_signalKStatusIcons->setDetailIndicatorsVisible(true);
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
        rebuildLayout();
        QTimer::singleShot(0, this, [this]() {
            refreshFromConfiguration();
        });
    }

    void TopBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);

        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            applyFramelessRuntimeChrome();
            updateComfortViewIcon();
            if (m_signalKStatusIcons) {
                m_signalKStatusIcons->refreshFromConfiguration();
            }
            if (m_compactStatusIcons) {
                m_compactStatusIcons->refreshFromConfiguration();
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

    void TopBar::applyFramelessRuntimeChrome() const {
        if (!ui) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        const QString widgetStyle = QStringLiteral("QWidget { background: transparent; border: none; }");
        if (m_contextWidget) {
            m_contextWidget->setStyleSheet(widgetStyle);
        }
        if (ui->widget_ClockBlock) {
            ui->widget_ClockBlock->setStyleSheet(widgetStyle);
        }
        if (m_statusWidget) {
            m_statusWidget->setStyleSheet(widgetStyle);
        }

        const QString labelStyle = QStringLiteral("QLabel { background: transparent; border: none; color: %1; }")
                                       .arg(chrome.text.name());
        if (ui->label_ApplicationName) {
            ui->label_ApplicationName->setStyleSheet(labelStyle);
        }
        if (ui->label_DateTime) {
            ui->label_DateTime->setStyleSheet(labelStyle);
        }
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
        const auto configRoot = FairWindSK::getInstance()->getConfiguration()->getRoot();
        const QString newLayoutSignature = fairwindsk::ui::layout::layoutSignature(
            configRoot,
            fairwindsk::ui::layout::BarId::Top);

        if (m_layoutSignature != newLayoutSignature || m_deferredExternalWidgetResolution) {
            rebuildLayout();
        }
        if (m_signalKStatusIcons) {
            m_signalKStatusIcons->refreshFromConfiguration();
        }
        if (m_compactStatusIcons) {
            m_compactStatusIcons->refreshFromConfiguration();
        }
    }

    void TopBar::setCurrentApp(AppItem *appItem) {
        m_currentApp = appItem;
        if (m_currentApp) {
            auto *widget = m_currentApp->getWidget();
            ui->toolButton_UR->setIcon(m_currentApp->getIcon(true));
            ui->toolButton_UR->setIconSize(QSize(36, 36));
            ui->label_ApplicationName->setText(m_currentApp->getDisplayName(true));
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
        ui->toolButton_UR->setIconSize(QSize(36, 36));
        ui->toolButton_UR->setEnabled(enableButton);
        ui->label_ApplicationName->setText(name);
        ui->label_ApplicationName->setToolTip(tooltip);
    }

    void TopBar::resetCurrentAppPresentation() const {
        ui->toolButton_UR->setIcon(QPixmap::fromImage(QImage(":/resources/images/icons/apps_icon.png")));
        ui->toolButton_UR->setIconSize(QSize(36, 36));
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

        clearLayoutEditHints();

        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        delete ui;
    }
}
