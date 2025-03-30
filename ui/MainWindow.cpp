//
// Created by Raffaele Montella on 21/03/21.
//

#include <QTimer>
#include <QToolButton>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QMessageBox>
#include <QProcess>
#include <QWindow>

#include "MainWindow.hpp"
#include "ui/topbar/TopBar.hpp"
#include "ui/bottombar/BottomBar.hpp"

#include "ui/about/About.hpp"
#include "ui/web/Web.hpp"
#include "ui/settings/Settings.hpp"
#include "ui/mydata/MyData.hpp"

namespace fairwindsk::ui {
    /*
     * MainWindow
     * Public constructor - This presents FairWind's UI
     */
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

        // Set up the UI
        ui->setupUi(this);

        // Get the singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Create the TopBar object
        m_topBar = new topbar::TopBar(ui->widget_Top);

        // Create the BottomBar object
        m_bottomBar = new bottombar::BottomBar(ui->widget_Bottom);

        // Set the Autopilot icon visible only if the autopilot application is defined
        m_bottomBar->setAutopilotIcon(fairWindSK->checkAutopilotApp());

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
        m_hotkey = new QHotkey(Qt::Key_Tab, Qt::ShiftModifier, true, this);

        // Connect the hotkey
        connect(m_hotkey, &QHotkey::activated, this, &MainWindow::onHotkey);

        // Check if no signal k server is defined
        if (fairWindSK->getConfiguration()->getSignalKServerUrl().isEmpty()) {

            // Open the settings window
            onSettings();
        };

        // Set the window size
        setSize();
    }

    // Hot Key handler
    void MainWindow::onHotkey(){

        // Show the main window fullscreen
        showFullScreen();
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

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Check if the debug is active
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "MainWindow hash:" << hash;
        }

        // Get the map containing all the loaded apps and pick the one that matches the provided hash
        const auto appItem = fairWindSK->getAppItemByHash(hash);

        // The QT widget implementing the app
        QWidget *widgetApp = nullptr;

        // Check if the requested app has been already launched by the user
        if (m_mapHash2Widget.contains(hash)) {

            // If yes, get its widget from mapWidgets
            widgetApp = m_mapHash2Widget[hash];
        } else {
            // Check if the app is an executable
            if (appItem->getName().startsWith("file://")) {

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
                m_bottomBar->addApp(appItem->getName());

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
            setSize();
        }
    }

    /*
     * onRemoveApp
     * Close a web app
     */

    void MainWindow::onRemoveApp(const QString& name) {
        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Check if the debug is active
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "MainWindow::onRemoveApp hash:" << name;
        }

        // Get the widget
        const auto widgetApp = m_mapHash2Widget[name];

        // Close the widget
        widgetApp->close();

        // Delete the widget
        delete widgetApp;

        // Remove the widget from m_mapHash2Widget
        m_mapHash2Widget.remove(name);

        // Remove the icon from the bottom bar
        m_bottomBar->removeApp(name);

        // Set the launcher as current application
        onApps();
    }
/*
 * onApps
 * Method called when the user clicks the Apps button on the BottomBar object
 */
    void MainWindow::onApps() {
        // Set the current app
        m_currentApp = nullptr;

        // Update the UI with the new widget
        ui->stackedWidget_Center->setCurrentWidget(m_launcher);

        // Set the current app in ui components
        m_topBar->setCurrentApp(m_currentApp);
    }

/*
 * onMyData
 * Method called when the user clicks the Settings button on the BottomBar object
 */
    void MainWindow::onMyData() {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        const auto myDataPage = new mydata::MyData(this, ui->stackedWidget_Center->currentWidget());
        ui->widget_Top->setDisabled(true);
        ui->widget_Bottom->setDisabled(true);
        ui->stackedWidget_Center->addWidget(myDataPage);
        ui->stackedWidget_Center->setCurrentWidget(myDataPage);

        connect(myDataPage, &mydata::MyData::closed, this, &MainWindow::onMyDataClosed);

        // Set the window size
        setSize();
    }


/*
 * onSettings
 * Method called when the user clicks the Settings button on the BottomBar object
 */
    void MainWindow::onSettings() {

        const auto settingsPage = new settings::Settings(this, ui->stackedWidget_Center->currentWidget());
        ui->widget_Top->setDisabled(true);
        ui->widget_Bottom->setDisabled(true);
        ui->stackedWidget_Center->addWidget(settingsPage);
        ui->stackedWidget_Center->setCurrentWidget(settingsPage);

        connect(settingsPage, &settings::Settings::accepted, this, &MainWindow::onSettingsAccepted);
        connect(settingsPage, &settings::Settings::rejected, this, &MainWindow::onSettingsRejected);

        // Set the window size
        setSize();
    }

    void MainWindow::setSize() {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        if (fairWindSK->getConfiguration()->getFullScreen()) {

            // Show the window fullscreen
            QTimer::singleShot(0, this, SLOT(showFullScreen()));
        } else {
            const auto left = fairWindSK->getConfiguration()->getWindowLeft();
            const auto top = fairWindSK->getConfiguration()->getWindowTop();
            const auto width = fairWindSK->getConfiguration()->getWindowWidth();
            const auto height = fairWindSK->getConfiguration()->getWindowHeight();
            move(left,top);
            resize(width, height);

            // Show windowed
            show();
            setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            raise();  // for MacOS
            activateWindow(); // for Windows
        }
    }
/*
 * onUpperLeft
 * Method called when the user clicks the upper left icon
 */
    void MainWindow::onUpperLeft() {
        // Show the settings view
        auto aboutPage = new about::About(this, ui->stackedWidget_Center->currentWidget());
        ui->widget_Top->setDisabled(true);
        ui->widget_Bottom->setDisabled(true);
        ui->stackedWidget_Center->addWidget(aboutPage);
        ui->stackedWidget_Center->setCurrentWidget(aboutPage);

        connect(aboutPage, &about::About::accepted, this, &MainWindow::onAboutAccepted);
    }

    void MainWindow::onMyDataClosed(mydata::MyData *myDataPage) {
        ui->widget_Top->setDisabled(false);
        ui->widget_Bottom->setDisabled(false);
        ui->stackedWidget_Center->removeWidget(myDataPage);
        ui->stackedWidget_Center->setCurrentWidget(myDataPage->getCurrentWidget());

        myDataPage->close();
        delete myDataPage;
    }

    void MainWindow::onAboutAccepted(about::About *aboutPage) {
        ui->widget_Top->setDisabled(false);
        ui->widget_Bottom->setDisabled(false);
        ui->stackedWidget_Center->removeWidget(aboutPage);
        ui->stackedWidget_Center->setCurrentWidget(aboutPage->getCurrentWidget());

        aboutPage->close();
        delete aboutPage;
    }

    void MainWindow::onSettingsRejected(settings::Settings *settingsPage) {
        ui->widget_Top->setDisabled(false);
        ui->widget_Bottom->setDisabled(false);
        ui->stackedWidget_Center->removeWidget(settingsPage);
        ui->stackedWidget_Center->setCurrentWidget(settingsPage->getCurrentWidget());
	
        settingsPage->close();
	    delete settingsPage;
    }

    void MainWindow::onSettingsAccepted(settings::Settings *settingsPage) const
    {
        ui->widget_Top->setDisabled(false);
        ui->widget_Bottom->setDisabled(false);
        ui->stackedWidget_Center->removeWidget(settingsPage);
        ui->stackedWidget_Center->setCurrentWidget(settingsPage->getCurrentWidget());

        settingsPage->close();
	    delete settingsPage;

	    QApplication::exit(1);
    }

    topbar::TopBar *MainWindow::getTopBar() {
        return reinterpret_cast<topbar::TopBar *>(&m_topBar);
    }

    launcher::Launcher *MainWindow::getLauncher() {
        return reinterpret_cast<launcher::Launcher *>(&m_launcher);;
    }

    bottombar::BottomBar *MainWindow::getBottomBar() {
        return reinterpret_cast<bottombar::BottomBar *>(&m_bottomBar);
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Quit FairWindSK",
                                                                  "Are you sure you want to exit theFairWindSK?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {

            // Accept the event
            event->accept();

            // Quit the application
            QApplication::quit();
        } else
            event->ignore();
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

        // Check if the hotkey is allocated
        if (m_hotkey)
        {
            // Delete the hotkey
            delete m_hotkey;

            // Set the hotkey pointer to null
            m_hotkey = nullptr;
        }


        // Check if the bottom bar is allocated
        if (m_bottomBar) {

            // Delete the bottom bar
            delete m_bottomBar;

            // Set the bottom bar pointer to null
            m_bottomBar = nullptr;
        }

        // Check uf the launcher is allocated
        if (m_launcher) {

            // Delete the launcher
            delete m_launcher;

            // Set the launcher pointer to null
            m_launcher = nullptr;
        }

        // Check if the top bar is allocated
        if (m_topBar) {

            // Delete the top bar
            delete m_topBar;

            // Set the top bar pointer to null
            m_topBar = nullptr;
        }

        // Check if the UI is allocated
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the UI pointer to null
            ui = nullptr;
        }

    }
}




