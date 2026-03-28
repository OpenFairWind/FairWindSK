//
// Created by Raffaele Montella on 21/03/21.
//

#include <QTimer>
#include <QToolButton>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>
#include <QProcess>
#include <QWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "MainWindow.hpp"
#include "ui/topbar/TopBar.hpp"
#include "ui/bottombar/BottomBar.hpp"

#include "ui/about/About.hpp"
#include "ui/web/Web.hpp"
#include "ui/settings/Settings.hpp"
#include "ui/mydata/MyData.hpp"
#include "ui/DrawerDialogHost.hpp"

namespace fairwindsk::ui {
    MainWindow *MainWindow::s_instance = nullptr;

    /*
     * MainWindow
     * Public constructor - This presents FairWind's UI
     */
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

        // Set up the UI
        ui->setupUi(this);

        // Get the singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        s_instance = this;

        // Create the TopBar object
        m_topBar = new topbar::TopBar(ui->widget_Top);

        // Create the BottomBar object
        m_bottomBar = new bottombar::BottomBar(ui->widget_Bottom);

        auto *topLayout = new QVBoxLayout(ui->widget_Top);
        topLayout->setContentsMargins(0, 0, 0, 0);
        topLayout->setSpacing(0);
        topLayout->addWidget(m_topBar);

        auto *bottomLayout = new QVBoxLayout(ui->widget_Bottom);
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayout->setSpacing(0);
        bottomLayout->addWidget(m_bottomBar);

        m_dialogDrawer = ui->frameDialogDrawer;
        m_dialogDrawerTitle = ui->labelDialogDrawerTitle;
        m_dialogDrawerContentHost = ui->widgetDialogDrawerContentHost;
        m_dialogDrawerContentLayout = ui->verticalLayoutDialogDrawerContent;
        m_dialogDrawerButtonsLayout = ui->horizontalLayoutDialogDrawerButtons;
        if (m_dialogDrawerButtonsLayout) {
            m_dialogDrawerButtonsLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }

        // The Autopilot panel is wired directly to the Signal K autopilot APIs.
        m_bottomBar->setAutopilotIcon(true);

        // Set the Anchor icon visible only if the anchor alarm application is defined
        m_bottomBar->setAnchorIcon(fairWindSK->checkAnchorApp());

        // Place the Apps object at the center of the UI
        setCentralWidget(ui->centralwidget);

        // Show the apps view when the user clicks on the Apps button inside the BottomBar object
        connect(m_bottomBar, &bottombar::BottomBar::setMyData, this, &MainWindow::onMyData);

        // Show the apps view when the user clicks on the Apps button inside the BottomBar object
        connect(m_bottomBar, &bottombar::BottomBar::setApps, this, &MainWindow::onApps);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(m_bottomBar, &bottombar::BottomBar::setSettings, this, &MainWindow::onSettings);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(m_topBar, &topbar::TopBar::clickedToolbuttonUL, this, &MainWindow::onUpperLeft);

        // Create the launcher
        m_launcher = new launcher::Launcher();

        // Add the launcher to the stacked widget
        ui->stackedWidget_Center->addWidget(m_launcher);

        // Connect the foreground app changed signal to the setForegroundApp method (launcher)
        connect(m_launcher, &launcher::Launcher::foregroundAppChanged,this, &MainWindow::setForegroundApp);

        // Connect the foreground app changed signal to the setForegroundApp method (bottom bar)
        connect(m_bottomBar, &bottombar::BottomBar::foregroundAppChanged,this, &MainWindow::setForegroundApp);


        // Preload a web application
        setForegroundApp("http:///");

        // Show the launcher
        onRemoveApp("http:///");

        // Create the hot key to popup this window
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        m_hotkey = new QHotkey(Qt::Key_Tab, Qt::ShiftModifier, true, this);

        // Connect the hotkey
        connect(m_hotkey, &QHotkey::activated, this, &MainWindow::onHotkey);
#endif

        // Check if no signal k server is defined
        if (fairWindSK->getConfiguration()->getSignalKServerUrl().isEmpty()) {

            // Open the settings window
            onSettings();
        };

        // Set the window size
        setSize();

        QTimer::singleShot(0, this, &MainWindow::prewarmPersistentPages);
        QTimer::singleShot(750, this, [this]() {
            ensureSettingsPage(m_launcher);
        });
    }

    bool MainWindow::isOverlayOpen() const {
        return m_activeOverlay != nullptr;
    }

    MainWindow *MainWindow::instance(QWidget *context) {
        if (context) {
            if (auto *window = qobject_cast<MainWindow *>(context->window())) {
                return window;
            }
        }

        if (s_instance) {
            return s_instance;
        }

        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *window = qobject_cast<MainWindow *>(widget)) {
                return window;
            }
        }

        return nullptr;
    }

    void MainWindow::setChromeEnabled(const bool enabled) const {
        ui->widget_Top->setEnabled(enabled);
        ui->widget_Bottom->setEnabled(enabled);
    }

    void MainWindow::setDrawerEnabled(const bool enabled) const {
        ui->widget_Top->setEnabled(enabled);
        ui->stackedWidget_Center->setEnabled(enabled);
        ui->widget_Bottom->setEnabled(enabled);
        if (m_dialogDrawer) {
            m_dialogDrawer->setEnabled(true);
        }
    }

    void MainWindow::clearDrawer() {
        while (m_dialogDrawerButtonsLayout && m_dialogDrawerButtonsLayout->count() > 1) {
            auto *item = m_dialogDrawerButtonsLayout->takeAt(0);
            if (auto *widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }
        if (m_dialogDrawerButtonsLayout && m_dialogDrawerButtonsLayout->count() == 1 && m_dialogDrawerButtonsLayout->itemAt(0)->spacerItem()) {
            m_dialogDrawerButtonsLayout->takeAt(0);
            m_dialogDrawerButtonsLayout->addStretch(1);
        }
        while (m_dialogDrawerContentLayout && m_dialogDrawerContentLayout->count() > 0) {
            auto *item = m_dialogDrawerContentLayout->takeAt(0);
            if (auto *widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }
    }

    bool MainWindow::isDrawerOpen() const {
        return m_dialogDrawer && m_dialogDrawer->isVisible() && m_activeDrawerLoop != nullptr;
    }

    void MainWindow::finishActiveDrawer(const int result) {
        if (m_activeDrawerResult) {
            *m_activeDrawerResult = result;
        }
        if (m_activeDrawerLoop) {
            m_activeDrawerLoop->quit();
        }
    }

    int MainWindow::execDrawer(const QString &title, QWidget *content, const QList<DrawerButtonSpec> &buttons, const int defaultResult) {
        if (!m_dialogDrawer || !content) {
            return defaultResult;
        }

        clearDrawer();
        m_dialogDrawerTitle->setText(title);
        content->setParent(m_dialogDrawerContentHost);
        m_dialogDrawerContentLayout->addWidget(content);
        ui->widgetDialogDrawerButtonRow->setVisible(!buttons.isEmpty());

        QEventLoop loop;
        int result = defaultResult;
        m_activeDrawerLoop = &loop;
        m_activeDrawerResult = &result;

        for (const auto &buttonSpec : buttons) {
            auto *button = new QPushButton(buttonSpec.text, m_dialogDrawer);
            button->setDefault(buttonSpec.isDefault);
            connect(button, &QPushButton::clicked, this, [&loop, &result, buttonSpec]() {
                result = buttonSpec.result;
                loop.quit();
            });
            m_dialogDrawerButtonsLayout->insertWidget(m_dialogDrawerButtonsLayout->count() - 1, button);
        }

        setDrawerEnabled(false);
        m_dialogDrawer->show();
        m_dialogDrawer->raise();
        loop.exec();
        m_activeDrawerLoop = nullptr;
        m_activeDrawerResult = nullptr;
        m_dialogDrawer->hide();
        clearDrawer();
        setDrawerEnabled(true);
        return result;
    }

    void MainWindow::showOverlay(QWidget *page) {
        if (!page || isOverlayOpen()) {
            if (page && page != m_activeOverlay) {
                page->deleteLater();
            }
            return;
        }

        m_activeOverlay = page;
        setChromeEnabled(false);
        ui->stackedWidget_Center->addWidget(page);
        ui->stackedWidget_Center->setCurrentWidget(page);
    }

    void MainWindow::closeOverlay(QWidget *page, QWidget *fallbackWidget) {
        if (!page) {
            return;
        }

        setChromeEnabled(true);
        ui->stackedWidget_Center->removeWidget(page);

        if (fallbackWidget && ui->stackedWidget_Center->indexOf(fallbackWidget) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(fallbackWidget);
        } else {
            showLauncher();
        }

        if (m_activeOverlay == page) {
            m_activeOverlay = nullptr;
        }

        page->close();
        delete page;
    }

    void MainWindow::showLauncher() {
        m_currentApp = nullptr;
        if (m_launcher && ui->stackedWidget_Center->indexOf(m_launcher) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(m_launcher);
        }
        syncTopBarToCurrentPage();
    }

    void MainWindow::syncTopBarToCurrentPage() {
        if (m_topBar) {
            const auto *currentWidget = ui->stackedWidget_Center->currentWidget();
            if (currentWidget == m_launcher) {
                m_topBar->setCurrentContext(
                    tr("Apps"),
                    tr("Application launcher"),
                    QIcon(":/resources/images/icons/apps_icon.png"),
                    false);
            } else if (currentWidget == m_myDataPage) {
                m_topBar->setCurrentContext(
                    tr("MyData"),
                    tr("Signal K resources and files"),
                    QIcon(":/resources/svg/OpenBridge/database.svg"),
                    false);
            } else if (currentWidget == m_settingsPage) {
                m_topBar->setCurrentContext(
                    tr("Settings"),
                    tr("Application settings"),
                    QIcon(":/resources/svg/OpenBridge/settings-iec.svg"),
                    false);
            } else if (currentWidget == m_aboutPage) {
                m_topBar->setCurrentContext(
                    tr("About"),
                    tr("About FairWindSK"),
                    QIcon(":/resources/images/mainwindow/fairwind_icon.png"),
                    false);
            } else if (m_currentApp) {
                m_topBar->setCurrentApp(m_currentApp);
            } else {
                m_topBar->setCurrentContext(
                    tr("Apps"),
                    tr("Application launcher"),
                    QIcon(":/resources/images/icons/apps_icon.png"),
                    false);
            }
        }
    }

    // Hot Key handler
    void MainWindow::onHotkey(){

        // Show the FairWindSK window in foreground
        setSize();
    }



    /*
     * getUi
     * Returns the widget's UI
     */
    Ui::MainWindow *MainWindow::getUi() {
        return ui;
    }

    /*
     * setForegroundApp
     * Method called when the user clicks on the Apps widget: show a new foreground app with the provided hash value
     */
    void MainWindow::setForegroundApp(const QString& hash) {
        if (!closeSettingsPage()) {
            return;
        }
        closeAboutPage();

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Check if the debug is active
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "MainWindow hash:" << hash;
        }

        // Get the map containing all the loaded apps and pick the one that matches the provided hash
        const auto appItem = fairWindSK->getAppItemByHash(hash);

        if (!appItem) {
            showLauncher();
            return;
        }

        // The QT widget implementing the app
        QWidget *widgetApp = nullptr;

        // Check if the requested app has been already launched by the user
        if (m_mapHash2Widget.contains(hash)) {

            // If yes, get its widget from mapWidgets
            widgetApp = m_mapHash2Widget[hash];
        } else {
            // Check if the app is an executable
            if (appItem->getName().startsWith("file://")) {

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
                qWarning() << "Native file:// applications are not supported on mobile builds:" << appItem->getName();
                showLauncher();
                return;
#else

                //https://forum.qt.io/topic/44091/embed-an-application-inside-a-qt-window-solved/16
                //https://forum.qt.io/topic/101510/calling-a-process-in-the-main-app-and-return-the-process-s-window-id

                // Get the path to the executable
                const auto executable = appItem->getName().replace("file://", "");

                // Get the executable arguments
                const auto arguments = appItem->getArguments();

                // Check if the debug is active
                //if (fairWindSK->isDebug()) {
                    // Write a message
                    qDebug() << appItem->getName() << " Native APP:  " << executable << " " << arguments;
                //}

                // Create a process
                const auto process = new QProcess(this);

                // Set th executable
                process->setProgram(executable);

                // Set the parameters
                process->setArguments(arguments);

                // Start the process
                process->start();

                appItem->setProcess(process);
                showLauncher();
                return;
#endif

            } else {
                // Create a new web instance
                const auto web = new web::Web(nullptr, appItem, fairWindSK->getWebEngineProfile());

                // Connect the remove app signal to the onRemoveApp member
                connect(web, &web::Web::removeApp, this, &MainWindow::onRemoveApp);


                // Get the app widget
                widgetApp = web;
            }

            // Register the web widget
            appItem->setWidget(widgetApp);

            // Check if the widget is valid
            if (widgetApp) {

                // Add it to the UI
                ui->stackedWidget_Center->addWidget(widgetApp);

                // Store it in mapWidgets for future usage
                m_mapHash2Widget.insert(hash, widgetApp);

                // Add icon to the bottom bar
                m_bottomBar->addApp(hash);

            }
        }

        // Check if the widget is valid
        if (widgetApp) {

            // Set the current app
            m_currentApp = appItem;

            // Update the UI with the new widget
            ui->stackedWidget_Center->setCurrentWidget(widgetApp);

            // Set the current app in ui components
            m_topBar->setCurrentApp(m_currentApp);

            // Set the window size
            //setSize();
        }
    }

    /*
     * onRemoveApp
     * Close a web app
     */

    void MainWindow::onRemoveApp(const QString& name) {
        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        QString hash = name;
        if (!m_mapHash2Widget.contains(hash)) {
            const auto candidateHash = fairWindSK->getAppHashById(name);
            if (!candidateHash.isEmpty()) {
                hash = candidateHash;
            }
        }

        // Check if the debug is active
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "MainWindow::onRemoveApp hash:" << hash;
        }

        if (!m_mapHash2Widget.contains(hash)) {
            return;
        }

        // Get the widget
        const auto widgetApp = m_mapHash2Widget[hash];

        if (m_currentApp == fairWindSK->getAppItemByHash(hash)) {
            m_currentApp = nullptr;
            m_topBar->setCurrentApp(nullptr);
        }

        ui->stackedWidget_Center->removeWidget(widgetApp);

        // Close the widget
        widgetApp->close();

        // Delete the widget
        delete widgetApp;

        // Remove the widget from m_mapHash2Widget
        m_mapHash2Widget.remove(hash);

        // Remove the icon from the bottom bar
        m_bottomBar->removeApp(hash);

        // Set the launcher as current application
        showLauncher();
    }
/*
 * onApps
 * Method called when the user clicks the Apps button on the BottomBar object
 */
    void MainWindow::onApps() {
        if (isOverlayOpen()) {
            return;
        }
        if (!closeSettingsPage(m_launcher)) {
            return;
        }
        closeAboutPage(m_launcher);
        showLauncher();
    }

/*
 * onMyData
 * Method called when the user clicks the Settings button on the BottomBar object
 */
    void MainWindow::onMyData() {
        if (!closeSettingsPage(m_myDataPage ? m_myDataPage : ui->stackedWidget_Center->currentWidget())) {
            return;
        }
        closeAboutPage(m_myDataPage ? m_myDataPage : ui->stackedWidget_Center->currentWidget());

        ensureMyDataPage(ui->stackedWidget_Center->currentWidget());
        ui->stackedWidget_Center->setCurrentWidget(m_myDataPage);
        syncTopBarToCurrentPage();
    }


/*
 * onSettings
 * Method called when the user clicks the Settings button on the BottomBar object
 */
    void MainWindow::onSettings() {
        if (isOverlayOpen()) {
            return;
        }
        if (m_settingsPage && ui->stackedWidget_Center->currentWidget() == m_settingsPage) {
            return;
        }

        closeAboutPage(ui->stackedWidget_Center->currentWidget());

        const auto fallbackWidget = ui->stackedWidget_Center->currentWidget();
        ensureSettingsPage(fallbackWidget);
        ui->stackedWidget_Center->setCurrentWidget(m_settingsPage);
        syncTopBarToCurrentPage();
    }

    void MainWindow::ensureMyDataPage(QWidget *fallbackWidget) {
        if (!m_myDataPage) {
            m_myDataPage = new mydata::MyData(this, fallbackWidget ? fallbackWidget : m_launcher);
            ui->stackedWidget_Center->addWidget(m_myDataPage);
            connect(m_myDataPage, &mydata::MyData::closed, this, &MainWindow::onMyDataClosed);
        }
    }

    void MainWindow::ensureSettingsPage(QWidget *fallbackWidget) {
        if (!m_settingsPage) {
            m_settingsPage = new settings::Settings(this, fallbackWidget ? fallbackWidget : m_launcher);
            ui->stackedWidget_Center->addWidget(m_settingsPage);
        } else {
            m_settingsPage->setCurrentWidget(fallbackWidget ? fallbackWidget : m_launcher);
        }
    }

    void MainWindow::prewarmPersistentPages() {
        QWidget *currentWidget = ui->stackedWidget_Center->currentWidget();
        ensureAboutPage(m_launcher);
        ensureMyDataPage(m_launcher);
        if (currentWidget && ui->stackedWidget_Center->indexOf(currentWidget) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(currentWidget);
        }
    }

    void MainWindow::setSize() {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        const auto unlockWindowSize = [this]() {
            setMinimumSize(QSize(0, 0));
            setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        };

        const auto lockWindowSize = [this](const int width, const int height) {
            setMinimumSize(width, height);
            setMaximumSize(width, height);
        };

        if (fairWindSK->getConfiguration()->getWindowMode()=="centered") {

            const auto width = fairWindSK->getConfiguration()->getWindowWidth();
            const auto height = fairWindSK->getConfiguration()->getWindowHeight();

            const QScreen *screen = QGuiApplication::primaryScreen();
            const auto  screenGeometry = screen->geometry();

            const auto left = (screenGeometry.width() - width) / 2;
            const auto top = (screenGeometry.height() - height) / 2;

            move(left,top);
            resize(width, height);
            lockWindowSize(width, height);

            // Show windowed
            show();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            raise();  // for MacOS
            activateWindow(); // for Windows

        } else if (fairWindSK->getConfiguration()->getWindowMode()=="maximized") {
            unlockWindowSize();

            // Set the window maximized
            showMaximized();

        } else if (fairWindSK->getConfiguration()->getWindowMode()=="fullscreen") {
            unlockWindowSize();

            // Show the window full screen
            showFullScreen();

        } else {
            const auto left = fairWindSK->getConfiguration()->getWindowLeft();
            const auto top = fairWindSK->getConfiguration()->getWindowTop();
            const auto width = fairWindSK->getConfiguration()->getWindowWidth();
            const auto height = fairWindSK->getConfiguration()->getWindowHeight();
            move(left,top);
            resize(width, height);
            lockWindowSize(width, height);

            // Show windowed
            show();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            raise();  // for MacOS
            activateWindow(); // for Windows
        }
    }

    void MainWindow::applyRuntimeConfiguration() {
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        if (!fairWindSK) {
            return;
        }

        m_bottomBar->setAnchorIcon(fairWindSK->checkAnchorApp());
        if (m_topBar) {
            m_topBar->refreshFromConfiguration();
        }
        if (m_bottomBar) {
            m_bottomBar->refreshFromConfiguration();
        }
        setSize();

        if (m_launcher) {
            m_launcher->refreshFromConfiguration(true);
        }
        if (m_myDataPage) {
            m_myDataPage->refreshFromConfiguration();
        }

        syncTopBarToCurrentPage();
    }
/*
 * onUpperLeft
 * Method called when the user clicks the upper left icon
 */
    void MainWindow::onUpperLeft() {
        if (isOverlayOpen()) {
            return;
        }
        QWidget *fallbackWidget = ui->stackedWidget_Center->currentWidget();
        if (fallbackWidget == m_settingsPage && m_settingsPage) {
            fallbackWidget = m_settingsPage->getCurrentWidget();
        }
        if (!closeSettingsPage(fallbackWidget, false)) {
            return;
        }

        ensureAboutPage(fallbackWidget ? fallbackWidget : ui->stackedWidget_Center->currentWidget());
        ui->stackedWidget_Center->setCurrentWidget(m_aboutPage);
        syncTopBarToCurrentPage();
    }

    void MainWindow::onMyDataClosed(mydata::MyData *myDataPage) {
        if (!myDataPage) {
            showLauncher();
            return;
        }

        QWidget *fallbackWidget = myDataPage->getCurrentWidget();
        ui->stackedWidget_Center->removeWidget(myDataPage);
        myDataPage->close();
        delete myDataPage;

        if (m_myDataPage == myDataPage) {
            m_myDataPage = nullptr;
        }

        if (fallbackWidget && ui->stackedWidget_Center->indexOf(fallbackWidget) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(fallbackWidget);
        } else {
            showLauncher();
            return;
        }
        syncTopBarToCurrentPage();
    }

    void MainWindow::closeAboutPage(QWidget *fallbackWidget) {
        if (!m_aboutPage) {
            return;
        }

        if (ui->stackedWidget_Center->currentWidget() != m_aboutPage) {
            return;
        }

        QWidget *targetFallback = fallbackWidget ? fallbackWidget : m_aboutPage->getCurrentWidget();
        m_aboutPage->setCurrentWidget(targetFallback);

        if (targetFallback && ui->stackedWidget_Center->indexOf(targetFallback) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(targetFallback);
            syncTopBarToCurrentPage();
        } else {
            showLauncher();
        }
    }

    void MainWindow::onAboutClosed(about::About *aboutPage) {
        if (aboutPage != m_aboutPage) {
            return;
        }
        closeAboutPage(aboutPage->getCurrentWidget());
    }

    void MainWindow::ensureAboutPage(QWidget *fallbackWidget) {
        if (!m_aboutPage) {
            m_aboutPage = new about::About(this, fallbackWidget ? fallbackWidget : m_launcher);
            ui->stackedWidget_Center->addWidget(m_aboutPage);
            connect(m_aboutPage, &about::About::closed, this, &MainWindow::onAboutClosed);
        } else {
            m_aboutPage->setCurrentWidget(fallbackWidget ? fallbackWidget : m_launcher);
        }
    }

    bool MainWindow::closeSettingsPage(QWidget *fallbackWidget, const bool showFallback, const bool exitAfterSave) {
        if (!m_settingsPage) {
            return true;
        }

        if (ui->stackedWidget_Center->currentWidget() != m_settingsPage) {
            return true;
        }

        QWidget *targetFallback = fallbackWidget ? fallbackWidget : m_settingsPage->getCurrentWidget();
        if (m_settingsPage->hasPendingChanges()) {
            m_settingsPage->saveChanges();
        }
        m_settingsPage->setCurrentWidget(targetFallback);

        if (showFallback && targetFallback && ui->stackedWidget_Center->indexOf(targetFallback) >= 0) {
            ui->stackedWidget_Center->setCurrentWidget(targetFallback);
            syncTopBarToCurrentPage();
        } else if (showFallback) {
            showLauncher();
        }

        if (exitAfterSave) {
            QApplication::exit(1);
        }

        return true;
    }

    topbar::TopBar *MainWindow::getTopBar() {
        return m_topBar;
    }

    launcher::Launcher *MainWindow::getLauncher() {
        return m_launcher;
    }

    bottombar::BottomBar *MainWindow::getBottomBar() {
        return m_bottomBar;
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        if (m_quitConfirmed) {
            event->accept();
            return;
        }

        if (isDrawerOpen()) {
            finishActiveDrawer(int(QMessageBox::Cancel));
            event->ignore();
            QTimer::singleShot(0, this, [this]() {
                if (isVisible()) {
                    close();
                }
            });
            return;
        }

        event->ignore();

        if (ui->stackedWidget_Center->currentWidget() == m_settingsPage && m_settingsPage && m_settingsPage->hasPendingChanges()) {
            m_settingsPage->saveChanges();
        }

        QTimer::singleShot(0, this, [this]() {
            const QMessageBox::StandardButton reply = drawer::question(this,
                                                                       tr("Quit FairWindSK"),
                                                                       tr("Are you sure you want to exit FairWindSK?"),
                                                                       QMessageBox::Yes | QMessageBox::No,
                                                                       QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                m_quitConfirmed = true;
                close();
            }
        });
    }

    /*
     * getWIdByPId
     * Return the window id given a process id.
     * This method is API dependant.
     * Actually now it is just a placeholder
     */
    WId MainWindow::getWIdByPId(qint64 pid) {
        WId result = 0;

        // ToDo: Implement the actual code

        return result;
    }

    /*
     * ~MainWindow
     * MainWindow's destructor
     */
    MainWindow::~MainWindow() {
        if (s_instance == this) {
            s_instance = nullptr;
        }

        // Check if the hotkey is allocated
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_hotkey)
        {
            // Delete the hotkey
            delete m_hotkey;

            // Set the hotkey pointer to null
            m_hotkey = nullptr;
        }
#endif
        // Check if the UI is allocated
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the UI pointer to null
            ui = nullptr;
        }

        m_topBar = nullptr;
        m_bottomBar = nullptr;
        m_launcher = nullptr;
        m_activeOverlay = nullptr;

    }
}
