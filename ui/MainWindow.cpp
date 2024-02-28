//
// Created by Raffaele Montella on 21/03/21.
//

#include <QTimer>
#include <QToolButton>



#include "MainWindow.hpp"
#include "ui/topbar/TopBar.hpp"
#include "ui/bottombar/BottomBar.hpp"

#include "ui/about/About.hpp"
#include "ui_MainWindow.h"

/*
 * MainWindow
 * Public constructor - This presents FairWind's UI
 */
fairwindsk::ui::MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    // Setup the UI
    ui->setupUi(this);

    // Instantiate TopBar and BottomBar object
    m_topBar = new fairwindsk::ui::topbar::TopBar(ui->widget_Top);
    m_bottonBar = new fairwindsk::ui::bottombar::BottomBar(ui->widget_Bottom);

    // Place the Apps object at the center of the UI
    setCentralWidget(ui->centralwidget);


    // Show the settings view
    auto fairWindSK = FairWindSK::getInstance();



    // Show the apps view when the user clicks on the Apps button inside the BottomBar object
    QObject::connect(m_bottonBar, &bottombar::BottomBar::setApps, this, &MainWindow::onApps);

    // Show the settings view when the user clicks on the Settings button inside the BottomBar object
    QObject::connect(m_bottonBar, &bottombar::BottomBar::setSettings, this, &MainWindow::onSettings);

    // Show the settings view when the user clicks on the Settings button inside the BottomBar object
    QObject::connect(m_topBar, &topbar::TopBar::clickedToolbuttonUL, this, &MainWindow::onUpperLeft);

    // Show the settings view when the user clicks on the Settings button inside the BottomBar object
    QObject::connect(m_topBar, &topbar::TopBar::clickedToolbuttonUR, this, &MainWindow::onUpperRight);

    QTimer::singleShot(0, this, SLOT(showFullScreen()));
}

/*
 * ~MainWindow
 * MainWindow's destructor
 */
fairwindsk::ui::MainWindow::~MainWindow() {

    if (m_bottonBar) {
        delete m_bottonBar;
        m_bottonBar = nullptr;
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
Ui::MainWindow *fairwindsk::ui::MainWindow::getUi() {
    return ui;
}

/*
 * setForegroundApp
 * Method called when the user clicks on the Apps widget: show a new foreground app with the provided hash value
 */
void fairwindsk::ui::MainWindow::setForegroundApp(QString hash) {
    qDebug() << "MainWindow hash:" << hash;
    /*
    // Get the FairWind singleton
    auto fairWind = fairWindSk::FairWindSk::getInstance();

    // Get the map containing all the loaded apps and pick the one that matches the provided hash
    auto appItem = fairWind->getAppItemByHash(hash);

    // Get the fairwind app
    fairWindSk::apps::IFairWindApp *fairWindApp = fairWind->getAppByExtensionId(appItem->getExtension());

    // The QT widget implementing the app
    QWidget *widgetApp = nullptr;

    // Check if the requested app has been already launched by the user
    if (m_mapHash2Widget.contains(hash)) {

        // If yes, get its widget from mapWidgets
        widgetApp = m_mapHash2Widget[hash];
    } else {
        // Set the route
        fairWindApp->setRoute(appItem->getRoute());

        // Set the args
        fairWindApp->setArgs(appItem->getArgs());

        // invoke the app onStart method
        fairWindApp->onStart();

        // Get the app widget
        widgetApp = ((fairwind::apps::FairWindApp *)fairWindApp)->getWidget();

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

        // Check if there is an app on foreground
        if (m_fairWindApp) {

            // Call the foreground app onPause method
            m_fairWindApp->onPause();
        }

        // Set the current app
        m_fairWindApp = fairWindApp;

        // Update the UI with the new widget
        ui->stackedWidget_Center->setCurrentWidget(widgetApp);

        // Call the new foreground app onResume method
        m_fairWindApp->onResume();

        // Set the current app in ui components
        m_topBar->setFairWindApp(appItem);
    }
     */
}

/*
 * onApps
 * Method called when the user clicks the Apps button on the BottomBar object
 */
void fairwindsk::ui::MainWindow::onApps() {
    /*
    // Show the settings view
    auto fairWindSk = FairWindSk::getInstance();

    // Show the launcher
    setForegroundApp(fairWind->getAppHashById(fairWind->getLauncherFairWindAppId()));
    */
}

/*
 * onSettings
 * Method called when the user clicks the Settings button on the BottomBar object
 */
void fairwindsk::ui::MainWindow::onSettings() {
    /*
    // Show the settings view
    auto fairWindSk = FairWindSk::getInstance();

    auto settingsFairWindAppId = fairWind->getSettingsFairWindAppId();
    if (!settingsFairWindAppId.isEmpty()) {
        setForegroundApp(fairWindSk->getAppHashById(settingsFairWindAppId));
    }
     */
}

/*
 * onUpperLeft
 * Method called when the user clicks the upper left icon
 */
void fairwindsk::ui::MainWindow::onUpperLeft() {
    // Show the settings view
    auto aboutPage = new about::About(this, ui->stackedWidget_Center->currentWidget());
    ui->widget_Top->setDisabled(true);
    ui->widget_Bottom->setDisabled(true);
    ui->stackedWidget_Center->addWidget(aboutPage);
    ui->stackedWidget_Center->setCurrentWidget(aboutPage);

    connect(aboutPage,&about::About::accepted,this, &MainWindow::onAboutAccepted);
}

void fairwindsk::ui::MainWindow::onAboutAccepted(fairwindsk::ui::about::About *aboutPage) {
    ui->widget_Top->setDisabled(false);
    ui->widget_Bottom->setDisabled(false);
    ui->stackedWidget_Center->removeWidget(aboutPage);
    ui->stackedWidget_Center->setCurrentWidget(aboutPage->getCurrentWidget());
}

/*
 * onUpperRight
 * Method called when the user clicks the upper right icon
 */
void fairwindsk::ui::MainWindow::onUpperRight() {
    /*
    // Show the settings view
    if (m_fairWindApp) {
        m_fairWindApp->colophon();
    }
     */
}




