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
        auto fairWindSk = fairwindsk::FairWindSK::getInstance();

        // Create the TopBar object
        m_topBar = new topbar::TopBar(ui->widget_Top);

        // Create the BottomBar object
        m_bottomBar = new bottombar::BottomBar(ui->widget_Bottom);

        // Get the autopilot application
        auto autopilotApp = fairWindSk->getConfiguration()->getAutopilotApp();

        // Check if the autopilot application is defined
        if (!autopilotApp.isEmpty()) {

            // Set the Autopilot icon visible only if the autopilot application is defined
            m_bottomBar->setAutopilotIcon(fairWindSk->getAppsHashes().contains(autopilotApp));
        } else {
            // Hide the Autopilot icon
            m_bottomBar->setAutopilotIcon(false);
        }

        // Get the anchor application
        auto anchorApp = fairWindSk->getConfiguration()->getAnchorApp();

        // Check if the autopilot application is defined
        if (!anchorApp.isEmpty()) {

            // Set the Anchor icon visible only if the anchor alarm application is defined
            m_bottomBar->setAnchorIcon(fairWindSk->getAppsHashes().contains(anchorApp));
        } else {
            // Hide the Anchor icon
            m_bottomBar->setAnchorIcon(false);
        }



        // Place the Apps object at the center of the UI
        setCentralWidget(ui->centralwidget);

        // Show the apps view when the user clicks on the Apps button inside the BottomBar object
        QObject::connect(m_bottomBar, &bottombar::BottomBar::setMyData, this, &MainWindow::onMyData);


        // Show the apps view when the user clicks on the Apps button inside the BottomBar object
        QObject::connect(m_bottomBar, &bottombar::BottomBar::setApps, this, &MainWindow::onApps);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        QObject::connect(m_bottomBar, &bottombar::BottomBar::setSettings, this, &MainWindow::onSettings);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        QObject::connect(m_topBar, &topbar::TopBar::clickedToolbuttonUL, this, &MainWindow::onUpperLeft);


        // Create the launcher
        m_launcher = new launcher::Launcher();

        // Add the launcher to the stacked widget
        ui->stackedWidget_Center->addWidget(m_launcher);

        // Connect the foreground app changed signa to the setForegroundApp method
        QObject::connect(m_launcher, &launcher::Launcher::foregroundAppChanged,
                         this, &MainWindow::setForegroundApp);

        // Preload a web application
        setForegroundApp("http:///");

        // Show the launcher
        onApps();


        // Show the window fullscreen
        QTimer::singleShot(0, this, SLOT(showFullScreen()));

    }

/*
 * ~MainWindow
 * MainWindow's destructor
 */
    MainWindow::~MainWindow() {

        if (m_bottomBar) {
            delete m_bottomBar;
            m_bottomBar = nullptr;
        }

        if (m_launcher) {
            delete m_launcher;
            m_launcher = nullptr;
        }

        if (m_topBar) {
            delete m_topBar;
            m_topBar = nullptr;
        }

        if (ui) {
            delete ui;
            ui = nullptr;
        }

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
    void MainWindow::setForegroundApp(QString hash) {
        qDebug() << "MainWindow hash:" << hash;

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get the map containing all the loaded apps and pick the one that matches the provided hash
        auto appItem = fairWindSK->getAppItemByHash(hash);


        // The QT widget implementing the app
        QWidget *widgetApp = nullptr;

        // Check if the requested app has been already launched by the user
        if (m_mapHash2Widget.contains(hash)) {

            // If yes, get its widget from mapWidgets
            widgetApp = m_mapHash2Widget[hash];
        } else {
            if (appItem->getName() == "__SETTINGS__") {

                widgetApp = new settings::Settings(nullptr, nullptr, FairWindSK::getInstance()->getConfiguration());

            } else if (appItem->getName().startsWith("file://")) {
                //https://forum.qt.io/topic/44091/embed-an-application-inside-a-qt-window-solved/16
                //https://forum.qt.io/topic/101510/calling-a-process-in-the-main-app-and-return-the-process-s-window-id
                qDebug() << appItem->getName() << " is a native app!";


                auto process = new QProcess(this);
                QString program = appItem->getName().replace("file://", "");
                auto arguments = appItem->getArguments();
                process->setProgram(program);
                WId winid = this->winId();

                //arguments << "-wid" << QString::number(winid);
                qDebug() << arguments;
                process->setArguments(arguments);
                process->start();


                /*
                QWindow *window = QWindow::fromWinId(1130);
                window->setFlags(Qt::FramelessWindowHint);
    
                widgetApp = QWidget::createWindowContainer(window);
                 */

            } else {
                // Create a new web instance
                auto web = new web::Web(nullptr, appItem, fairWindSK->getWebEngineProfile());

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
        }
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

        auto fairWindSK = FairWindSK::getInstance();
        auto myDataPage = new mydata::MyData(this, ui->stackedWidget_Center->currentWidget());
        ui->widget_Top->setDisabled(true);
        ui->widget_Bottom->setDisabled(true);
        ui->stackedWidget_Center->addWidget(myDataPage);
        ui->stackedWidget_Center->setCurrentWidget(myDataPage);

        connect(myDataPage, &mydata::MyData::closed, this, &MainWindow::onMyDataClosed);
    }


/*
 * onSettings
 * Method called when the user clicks the Settings button on the BottomBar object
 */
    void MainWindow::onSettings() {

        auto settingsPage = new settings::Settings(this, ui->stackedWidget_Center->currentWidget(),
                                                   FairWindSK::getInstance()->getConfiguration());
        ui->widget_Top->setDisabled(true);
        ui->widget_Bottom->setDisabled(true);
        ui->stackedWidget_Center->addWidget(settingsPage);
        ui->stackedWidget_Center->setCurrentWidget(settingsPage);

        connect(settingsPage, &settings::Settings::accepted, this, &MainWindow::onSettingsAccepted);
        connect(settingsPage, &settings::Settings::rejected, this, &MainWindow::onSettingsRejected);
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

    void MainWindow::onSettingsAccepted(settings::Settings *settingsPage) {
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

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Quit FairWindSK", "Are you sure you want to exit theFairWindSK?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            event->accept();

            QApplication::quit();
        } else
            event->ignore();
    }
}




