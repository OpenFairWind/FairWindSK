//
// Created by Raffaele Montella on 12/04/21.
//



#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QAbstractButton>
#include <QEasingCurve>
#include <QFrame>
#include <QScroller>
#include <QScrollerProperties>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QResizeEvent>
#include "BottomBar.hpp"

#include <QtWidgets/QLabel>

#include "FairWindSK.hpp"
#include "runtime/DiagnosticsSupport.hpp"
#include "ui/IconUtils.hpp"
#include "ui/layout/BarLayout.hpp"
#include "ui/topbar/TopBar.hpp"

namespace fairwindsk::ui::bottombar {
    namespace {
        constexpr int kBottomBarIconSize = 44;
        constexpr int kBottomBarButtonHeight = 90;
        constexpr int kPortAreaHeight = 84;
    }

    BottomBar *BottomBar::instance() {
        return s_instance;
    }

    QWidget *BottomBar::createSeparatorWidget() {
        auto *line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Plain);
        line->setLineWidth(1);
        m_dynamicLayoutWidgets.append(line);
        return line;
    }

    void BottomBar::clearConfiguredLayout() {
        if (!ui || !ui->horizontalLayoutButtons) {
            return;
        }

        while (ui->horizontalLayoutButtons->count() > 0) {
            auto *item = ui->horizontalLayoutButtons->takeAt(0);
            delete item;
        }

        for (auto &widget : m_dynamicLayoutWidgets) {
            if (widget) {
                widget->deleteLater();
            }
        }
        m_dynamicLayoutWidgets.clear();
    }

    QWidget *BottomBar::widgetForItemId(const QString &itemId) const {
        if (itemId == QStringLiteral("open_apps")) {
            return ui->scrollArea_Port;
        }
        if (itemId == QStringLiteral("mydata")) {
            return ui->toolButton_MyData;
        }
        if (itemId == QStringLiteral("pob")) {
            return ui->toolButton_POB;
        }
        if (itemId == QStringLiteral("autopilot")) {
            return ui->toolButton_Autopilot;
        }
        if (itemId == QStringLiteral("apps")) {
            return ui->toolButton_Apps;
        }
        if (itemId == QStringLiteral("anchor")) {
            return ui->toolButton_Anchor;
        }
        if (itemId == QStringLiteral("alarms")) {
            return ui->toolButton_Alarms;
        }
        if (itemId == QStringLiteral("settings")) {
            return ui->toolButton_Settings;
        }
        if (itemId == QStringLiteral("signalk_status")) {
            return m_signalKServerBox;
        }

        return nullptr;
    }

    void BottomBar::rebuildLayout() {
        if (!ui || !ui->horizontalLayoutButtons) {
            return;
        }

        clearConfiguredLayout();

        const auto entries = fairwindsk::ui::layout::entriesForBar(
            FairWindSK::getInstance()->getConfiguration()->getRoot(),
            fairwindsk::ui::layout::BarId::Bottom);
        for (const auto &entry : entries) {
            if (!entry.enabled) {
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Separator) {
                ui->horizontalLayoutButtons->addWidget(createSeparatorWidget(), 0, Qt::AlignVCenter);
                continue;
            }

            if (entry.kind == fairwindsk::ui::layout::EntryKind::Stretch) {
                ui->horizontalLayoutButtons->addStretch(1);
                continue;
            }

            QWidget *widget = widgetForItemId(entry.widgetId);
            if (!widget && fairwindsk::ui::topbar::TopBar::instance()) {
                widget = fairwindsk::ui::topbar::TopBar::instance()->widgetForItemId(entry.widgetId);
            }
            if (!widget) {
                continue;
            }

            ui->horizontalLayoutButtons->addWidget(widget, 0, Qt::AlignVCenter);
        }
    }
/*
 * BottomBar
 * Public constructor - This presents some navigation buttons at the bottom of the screen
 */
    fairwindsk::ui::bottombar::BottomBar::BottomBar(QWidget *parent) :
            QWidget(parent),
            ui(new Ui::BottomBar) {
        s_instance = this;

        m_iconSize = kBottomBarIconSize;
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Set the UI
        ui->setupUi(this);
        setMaximumHeight(96);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        ui->scrollArea_Port->setBorderless(true);
        ui->scrollArea_Port->setFrameShape(QFrame::NoFrame);
        ui->scrollArea_Port->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->scrollArea_Port->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->scrollArea_Port->setFixedHeight(kPortAreaHeight);
        ui->scrollArea_Port->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->scrollArea_Port->viewport()->setAutoFillBackground(false);
        ui->scrollArea_Port->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        ui->scrollArea_Port->installEventFilter(this);
        ui->widget_Starboard->installEventFilter(this);
        ui->widget_CenterButtons->installEventFilter(this);
        QScroller::grabGesture(ui->scrollArea_Port->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(ui->scrollArea_Port->viewport(), QScroller::LeftMouseButtonGesture);
        auto scrollerProperties = QScroller::scroller(ui->scrollArea_Port->viewport())->scrollerProperties();
        scrollerProperties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::AxisLockThreshold, 0.25);
        scrollerProperties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.005);
        scrollerProperties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.55);
        scrollerProperties.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve(QEasingCurve::OutCubic));
        QScroller::scroller(ui->scrollArea_Port->viewport())->setScrollerProperties(scrollerProperties);

        const QList<QToolButton *> navigationButtons = {
            ui->toolButton_MyData,
            ui->toolButton_POB,
            ui->toolButton_Autopilot,
            ui->toolButton_Apps,
            ui->toolButton_Anchor,
            ui->toolButton_Alarms,
            ui->toolButton_Settings
        };
        for (auto *button : navigationButtons) {
            if (!button) {
                continue;
            }

            button->setAutoRaise(true);
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setIconSize(QSize(m_iconSize, m_iconSize));
            button->setMinimumHeight(kBottomBarButtonHeight);
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        applyNavigationButtonIcons();

        // Create the POB bar
        m_POBBar = new POBBar(this);

        // Create the autopilot bar
        m_AutopilotBar = new AutopilotBar(this);

        // Create the Anchor bar
        m_AnchorBar = new AnchorBar(this);

        // Create the alarms bar
        m_AlarmsBar = new AlarmsBar(this);

        m_signalKServerBox = new widgets::SignalKServerBox(this);
        ui->widget_Starboard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_signalKServerBox->setMaximumHeight(kBottomBarButtonHeight);
        m_signalKServerBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto *starboardLayout = new QVBoxLayout(ui->widget_Starboard);
        starboardLayout->setContentsMargins(0, 0, 0, 0);
        starboardLayout->setSpacing(0);
        starboardLayout->addWidget(m_signalKServerBox);

        // Add the POB bar to the layout
        ui->gridLayout->addWidget(m_POBBar,0,0);

        // Add the autopilot bar to the layout
        ui->gridLayout->addWidget(m_AutopilotBar,1,0);

        // Add the POB bar to the layout
        ui->gridLayout->addWidget(m_AnchorBar,2,0);

        // Add the alarms bar to the layout
        ui->gridLayout->addWidget(m_AlarmsBar,3,0);

        // Emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_MyData, &QToolButton::released, this, &BottomBar::myData_clicked);

        // Emit a signal when the POB tool button from the UI is clicked
        connect(ui->toolButton_POB, &QToolButton::released, this, &BottomBar::pob_clicked);

        // Emit a signal when the POB tool button from the UI is clicked
        connect(ui->toolButton_Autopilot, &QToolButton::released, this, &BottomBar::autopilot_clicked);

        // Emit a signal when the Apps tool button from the UI is clicked
        connect(ui->toolButton_Apps, &QToolButton::released, this, &BottomBar::apps_clicked);

        // Emit a signal when the POB tool button from the UI is clicked
        connect(ui->toolButton_Anchor, &QToolButton::released, this, &BottomBar::anchor_clicked);

        // Emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Alarms, &QToolButton::released, this, &BottomBar::alarms_clicked);

        // Emit a signal when the Settings tool button from the UI is clicked
        connect(ui->toolButton_Settings, &QToolButton::released, this, &BottomBar::settings_clicked);

        connect(m_POBBar, &POBBar::hidden, this, [this]() { setPanelVisibility(m_POBBar, false); });
        connect(m_AutopilotBar, &AutopilotBar::hidden, this, [this]() { setPanelVisibility(m_AutopilotBar, false); });
        connect(m_AnchorBar, &AnchorBar::hidden, this, [this]() { setPanelVisibility(m_AnchorBar, false); });
            connect(m_AlarmsBar, &AlarmsBar::hidden, this, [this]() { setPanelVisibility(m_AlarmsBar, false); });

        if (fairWindSK) {
            m_runtimeHealthState = fairWindSK->runtimeHealthState();
            m_runtimeHealthSummary = fairWindSK->runtimeHealthSummary();
            connect(fairWindSK, &fairwindsk::FairWindSK::runtimeHealthChanged,
                    this, &BottomBar::onRuntimeHealthChanged);
        }

        updateHealthChrome();
        rebuildLayout();
    }

    /*
     * setAutopilotIcon
     * Set Autopilot icon visibility
     */
    void BottomBar::setAutopilotIcon(const bool value) const
    {
        ui->toolButton_Autopilot->setEnabled(value);
        if (!value) {
            setPanelVisibility(m_AutopilotBar, false);
        }
    }

    /*
     * setAnchorIcon
     * Set Anchor icon visibility
     */
    void BottomBar::setAnchorIcon(const bool value) const
    {
        ui->toolButton_Anchor->setEnabled(value);
        if (!value) {
            setPanelVisibility(m_AnchorBar, false);
        }
    }

    /*
     * setPOBIcon
     * Set POB icon visibility
     */
    void BottomBar::setPOBIcon(const bool value) const
    {
        ui->toolButton_POB->setEnabled(value);
        if (!value) {
            setPanelVisibility(m_POBBar, false);
        }
    }

    void BottomBar::refreshFromConfiguration() const {
        const_cast<BottomBar *>(this)->rebuildLayout();
        if (m_POBBar) {
            m_POBBar->refreshFromConfiguration();
        }
        if (m_AlarmsBar) {
            m_AlarmsBar->refreshFromConfiguration();
        }
        if (m_AutopilotBar) {
            m_AutopilotBar->refreshFromConfiguration();
        }
        if (m_AnchorBar) {
            m_AnchorBar->refreshFromConfiguration();
        }
        const_cast<BottomBar *>(this)->updateHealthChrome();
    }

    void BottomBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            applyNavigationButtonIcons();
        }
    }

    void BottomBar::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        rebalanceNavigationBlock();
    }

    bool BottomBar::eventFilter(QObject *watched, QEvent *event) {
        const bool needsRebalance = watched &&
                                    event &&
                                    (watched == ui->scrollArea_Port ||
                                     watched == ui->widget_Starboard ||
                                     watched == ui->widget_CenterButtons) &&
                                    (event->type() == QEvent::Resize ||
                                     event->type() == QEvent::Show ||
                                     event->type() == QEvent::LayoutRequest);
        if (needsRebalance) {
            QTimer::singleShot(0, this, [this]() { rebalanceNavigationBlock(); });
        }

        return QWidget::eventFilter(watched, event);
    }

    void BottomBar::applyNavigationButtonIcons() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QList<QToolButton *> navigationButtons = {
            ui->toolButton_MyData,
            ui->toolButton_POB,
            ui->toolButton_Autopilot,
            ui->toolButton_Apps,
            ui->toolButton_Anchor,
            ui->toolButton_Alarms,
            ui->toolButton_Settings
        };
        for (auto *button : navigationButtons) {
            if (!button) {
                continue;
            }
            fairwindsk::ui::applyBottomBarToolButtonChrome(
                button,
                colors,
                fairwindsk::ui::BottomBarButtonChrome::Transparent,
                QSize(m_iconSize, m_iconSize),
                kBottomBarButtonHeight);
        }
        const_cast<BottomBar *>(this)->updateHealthChrome();
    }

    void BottomBar::updateHealthChrome() {
        if (!ui) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::BottomBarButtonChrome appsChrome = fairwindsk::ui::BottomBarButtonChrome::Transparent;
        switch (m_runtimeHealthState) {
            case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedLive:
                appsChrome = fairwindsk::ui::BottomBarButtonChrome::Transparent;
                break;
            case fairwindsk::FairWindSK::RuntimeHealthState::AppsLoading:
            case fairwindsk::FairWindSK::RuntimeHealthState::Connecting:
            case fairwindsk::FairWindSK::RuntimeHealthState::Reconnecting:
            case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedStale:
            case fairwindsk::FairWindSK::RuntimeHealthState::AppsStale:
            case fairwindsk::FairWindSK::RuntimeHealthState::RestDegraded:
            case fairwindsk::FairWindSK::RuntimeHealthState::StreamDegraded:
                appsChrome = fairwindsk::ui::BottomBarButtonChrome::Flat;
                break;
            case fairwindsk::FairWindSK::RuntimeHealthState::ForegroundAppDegraded:
            case fairwindsk::FairWindSK::RuntimeHealthState::Disconnected:
                appsChrome = fairwindsk::ui::BottomBarButtonChrome::Accent;
                break;
        }

        fairwindsk::ui::applyBottomBarToolButtonChrome(
            ui->toolButton_Apps,
            colors,
            appsChrome,
            QSize(m_iconSize, m_iconSize),
            kBottomBarButtonHeight);
        ui->toolButton_Apps->setToolTip(
            m_runtimeHealthSummary.trimmed().isEmpty()
                ? tr("Application launcher")
                : tr("Application launcher\n%1").arg(m_runtimeHealthSummary.trimmed()));
        if (ui->toolButton_Settings) {
            ui->toolButton_Settings->setToolTip(
                m_runtimeHealthSummary.trimmed().isEmpty()
                    ? tr("Settings")
                    : tr("Settings\n%1").arg(m_runtimeHealthSummary.trimmed()));
        }
    }

    void BottomBar::onRuntimeHealthChanged(const fairwindsk::FairWindSK::RuntimeHealthState state,
                                           const QString &summary,
                                           const QString &badgeText) {
        Q_UNUSED(badgeText)
        m_runtimeHealthState = state;
        m_runtimeHealthSummary = summary.trimmed();
        updateHealthChrome();
    }

    void BottomBar::rebalanceNavigationBlock() const {
        // The bottom bar now supports arbitrary item placement, so the old
        // three-block balancing logic no longer applies.
    }

/*
 * myData_clicked
 * Method called when the user wants to view the stored data
 */
    void BottomBar::myData_clicked() {
        hideTransientPanels();
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("mydata"), QStringLiteral("MyData"));
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit setMyData();
    }

/*
 * myData_clicked
 * Method called when the user click on Men Over Board (POB)
 */
    void BottomBar::pob_clicked() {
        hideTransientPanels(m_POBBar);
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("pob"), QStringLiteral("POB"));
        m_POBBar->POB();
        setPanelVisibility(m_POBBar, true);
    }

    void BottomBar::autopilot_clicked() {
        if (m_AutopilotBar) {
            fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("autopilot"), QStringLiteral("Autopilot"));
            setPanelVisibility(m_AutopilotBar, true);
        }
    }

/*
 * apps_clicked
 * Method called when the user wants to view the apps screen
 */
    void BottomBar::apps_clicked() {
        hideTransientPanels();
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("apps"), QStringLiteral("Launcher"));

        // Emit the signal to tell the MainWindow to update the UI and show the apps screen
        emit setApps();
    }


    void BottomBar::anchor_clicked() {
        if (m_AnchorBar) {
            fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("anchor"), QStringLiteral("Anchor"));
            setPanelVisibility(m_AnchorBar, true);
        }

    }

/*
 * settings_clicked
 * Method called when the user wants to view the alarms screen
 */
    void BottomBar::alarms_clicked() {
        if (m_AlarmsBar) {
            fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("alarms"), QStringLiteral("Alarms"));
            setPanelVisibility(m_AlarmsBar, true);
        }
    }

/*
 * settings_clicked
 * Method called when the user wants to view the settings screen
 */
    void BottomBar::settings_clicked() {
        hideTransientPanels();
        fairwindsk::runtime::recordUserInteraction(QStringLiteral("bottom_bar"), QStringLiteral("settings"), QStringLiteral("Settings"));

        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit setSettings();
    }

    /*
 * addApp
 * Add the application icon in the shortcut area
 */
    void BottomBar::addApp(const QString& name) {
            if (m_buttons.contains(name)) {
                return;
            }

        //if (name != "http:///") {

            // Get the FairWind singleton
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();


            QString resolvedHash = name;
            auto appItem = fairWindSK->getAppItemByHash(resolvedHash);
            if (!appItem) {
                const QString candidateHash = fairWindSK->getAppHashById(name);
                if (!candidateHash.isEmpty()) {
                    resolvedHash = candidateHash;
                    appItem = fairWindSK->getAppItemByHash(resolvedHash);
                }
            }

            QString displayName;
            QPixmap pixmap;
            if (appItem) {
                displayName = appItem->getDisplayName();
                pixmap = appItem->getIcon();
            } else {
                const int idx = fairWindSK->getConfiguration()->findApp(name);
                if (idx != -1) {
                    fairwindsk::AppItem configApp(fairWindSK->getConfiguration()->getRoot()["apps"].at(idx));
                    displayName = configApp.getDisplayName();
                    pixmap = configApp.getIcon();
                    resolvedHash = configApp.getName();
                }
            }

            if (!displayName.isEmpty()) {

                // Create a new button
                auto *button = new QToolButton(this);

                // Set the app's hash value as the 's object name
                button->setObjectName("toolbutton_" + resolvedHash);

                // Set the app's name as the button's text
                //button->setText(appItem->getDisplayName());

                // Set the tool tip
                button->setToolTip(displayName);
                button->setAutoRaise(true);
                auto *styleFairWind = fairwindsk::FairWindSK::getInstance();
                auto *styleConfiguration = styleFairWind ? styleFairWind->getConfiguration() : nullptr;
                const QString stylePreset = styleFairWind ? styleFairWind->getActiveComfortViewPreset(styleConfiguration) : QStringLiteral("day");
                fairwindsk::ui::applyBottomBarToolButtonChrome(
                    button,
                    fairwindsk::ui::resolveComfortChromeColors(styleConfiguration, stylePreset, palette(), false),
                    fairwindsk::ui::BottomBarButtonChrome::Transparent,
                    QSize(m_iconSize, m_iconSize),
                    kBottomBarButtonHeight);

                // Check if the icon is available
                if (!pixmap.isNull()) {
                    // Scale the icon keeping the original aspect ratio.
                    pixmap = pixmap.scaled(m_iconSize, m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                    // Set the app's icon as the button's icon
                    button->setIcon(pixmap);

                    // Give the button's icon a fixed square
                    button->setIconSize(QSize(m_iconSize, m_iconSize));
                }

                // Launch the app when the button is clicked
                connect(button, &QToolButton::released, this, &BottomBar::app_clicked);

                // Add the newly created button to the horizontal layout as a widget
                ui->horizontalLayoutApps->addWidget(button);

                // Store in buttons in a map
                m_buttons[resolvedHash] = button;


            }
        //}
    }

    /*
 * removeApp
 * Remove the application icon frpm the shortcut area
 */
    void BottomBar::removeApp(const QString& name) {
            if (!m_buttons.contains(name)) {
                return;
            }

        // Check for the setting application
        //if (name != "http:///") {

            // Get the button
            const auto button = m_buttons[name];

            // Add the newly created button to the horizontal layout as a widget
            ui->horizontalLayoutApps->removeWidget(button);

            // Remove the button from the array
            m_buttons.remove(name);

            // Delete the button
            button->deleteLater();

        //}
    }

    /*
 * app_clicked
 * Method called when the user wants to launch an app
 */
    void BottomBar::app_clicked() {

        // Get the sender button
        auto *buttonWidget = qobject_cast<QToolButton *>(sender());

        // Check if the sender is valid
        if (!buttonWidget) {

            // The sender is not a widget
            return;
        }

        // Get the app's hash value from the button's object name
        const QString hash = buttonWidget->objectName().mid(QStringLiteral("toolbutton_").size());

        if (hash.isEmpty()) {
            return;
        }

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "Apps - hash:" << hash;
        }

        // Emit the signal to tell the MainWindow to update the UI and show the app with that particular hash value
        emit foregroundAppChanged(hash);
    }

    void BottomBar::setRegularBarVisible(const bool visible) const {
        const QList<QWidget *> regularWidgets = {
            ui->scrollArea_Port,
            ui->widget_CenterButtons,
            ui->widget_Starboard
        };

        for (auto *widget : regularWidgets) {
            if (widget) {
                widget->setVisible(visible);
            }
        }
    }

    void BottomBar::setPanelVisibility(QWidget *panel, const bool visible) const {
        if (!panel) {
            return;
        }

        if (visible) {
            hideTransientPanels(panel);
            setRegularBarVisible(false);
            panel->raise();
        } else {
            const QList<QWidget *> panels = {m_POBBar, m_AutopilotBar, m_AnchorBar, m_AlarmsBar};
            const bool anotherPanelVisible = std::any_of(
                panels.cbegin(),
                panels.cend(),
                [panel](const QWidget *candidate) {
                    return candidate && candidate != panel && candidate->isVisible();
                });
            if (!anotherPanelVisible) {
                setRegularBarVisible(true);
            }
        }

        panel->setVisible(visible);
    }

    void BottomBar::hideTransientPanels(QWidget *except) const {
        const QList<QWidget *> panels = {m_POBBar, m_AutopilotBar, m_AnchorBar, m_AlarmsBar};
        for (auto *panel : panels) {
            if (panel && panel != except) {
                panel->setVisible(false);
            }
        }

        if (!except) {
            setRegularBarVisible(true);
        }
    }

/*
 * ~BottomBar
 * BottomBar's destructor
 */
    BottomBar::~BottomBar() {
        if (s_instance == this) {
            s_instance = nullptr;
        }

        // Delete the application icons
        for (const auto button: m_buttons) {

            // Delete the button
            delete button;
        }

        // Check if the autopilot bar is instanced
        if (m_AutopilotBar) {
            // Delete the autopilot bar
            delete m_AutopilotBar;

            // Set the autopilot bar pointer to null
            m_AutopilotBar = nullptr;
        }

        // Check if the alarms bar is instanced
        if (m_AlarmsBar) {
            // Delete the alarms bar
            delete m_AlarmsBar;

            // Set the alarms bar pointer to null
            m_AlarmsBar = nullptr;
        }

        // Check if the POB bar is instanced
        if (m_POBBar) {
            // Delete the POB bar
            delete m_POBBar;

            // Set the POB bar pointer to null
            m_POBBar = nullptr;
        }

        // Check if the anchor bar is instanced
        if (m_AnchorBar) {
            // Delete the anchor bar
            delete m_AnchorBar;

            // Set the anchor bar pointer to null
            m_AnchorBar = nullptr;
        }

        if (m_signalKServerBox) {
            delete m_signalKServerBox;
            m_signalKServerBox = nullptr;
        }

        // Check if the UI is instanced
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the UI pointer to null
            ui = nullptr;
        }
    }
}
