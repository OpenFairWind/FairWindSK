//
// Created by Raffaele Montella on 21/03/21.
//

#include <QTimer>
#include <QToolButton>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>

#include "MainWindow.hpp"
#include "ui/topbar/TopBar.hpp"
#include "ui/bottombar/BottomBar.hpp"

#include "ui/about/About.hpp"
#include "ui/web/Web.hpp"


/*
 * MainWindow
 * Public constructor - This presents FairWind's UI
 */
fairwindsk::ui::MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    // Show the settings view
    auto fairWindSK = FairWindSK::getInstance();

    auto profileName = QString::fromLatin1("FairWindSK.%1").arg(qWebEngineChromiumVersion());

    //m_profile = new QWebEngineProfile(profileName );
    m_profile = QWebEngineProfile::defaultProfile();
    //m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    if (fairWindSK->isDebug()) {
        qDebug() << "QWenEngineProfile data store: " << m_profile->persistentStoragePath();
    }

    // Setup the UI
    ui->setupUi(this);

    // Instantiate TopBar and BottomBar object
    m_topBar = new fairwindsk::ui::topbar::TopBar(ui->widget_Top);
    m_launcher = new fairwindsk::ui::launcher::Launcher();
    ui->stackedWidget_Center->addWidget(m_launcher);
    m_bottomBar = new fairwindsk::ui::bottombar::BottomBar(ui->widget_Bottom);



    // Place the Apps object at the center of the UI
    setCentralWidget(ui->centralwidget);



    // Show the apps view when the user clicks on the Apps button inside the BottomBar object
    QObject::connect(m_bottomBar, &bottombar::BottomBar::setApps, this, &MainWindow::onApps);

    // Show the settings view when the user clicks on the Settings button inside the BottomBar object
    QObject::connect(m_bottomBar, &bottombar::BottomBar::setSettings, this, &MainWindow::onSettings);

    // Show the settings view when the user clicks on the Settings button inside the BottomBar object
    QObject::connect(m_topBar, &topbar::TopBar::clickedToolbuttonUL, this, &MainWindow::onUpperLeft);



    QObject::connect(m_launcher, &fairwindsk::ui::launcher::Launcher::foregroundAppChanged, this,
                     &MainWindow::setForegroundApp);

    //QTimer::singleShot(0, this, SLOT(showFullScreen()));


}

/*
 * ~MainWindow
 * MainWindow's destructor
 */
fairwindsk::ui::MainWindow::~MainWindow() {

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

    if (m_profile) {
        delete m_profile;
        m_profile = nullptr;
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

        auto web = new fairwindsk::ui::web::Web(nullptr,appItem);

        // Register the web widget
        //web->setApp(appItem);
        appItem->setWeb(web);


        // Get the app widget
        widgetApp = web;

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

    showFullScreen();
}

/*
 * onApps
 * Method called when the user clicks the Apps button on the BottomBar object
 */
void fairwindsk::ui::MainWindow::onApps() {
    // Set the current app
    m_currentApp = nullptr;

    // Update the UI with the new widget
    ui->stackedWidget_Center->setCurrentWidget(m_launcher);

    // Set the current app in ui components
    m_topBar->setCurrentApp(m_currentApp);

    showFullScreen();
}

/*
 * onSettings
 * Method called when the user clicks the Settings button on the BottomBar object
 */
void fairwindsk::ui::MainWindow::onSettings() {
    setForegroundApp("admin");
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

fairwindsk::ui::topbar::TopBar *fairwindsk::ui::MainWindow::getTopBar() {
    return reinterpret_cast<topbar::TopBar *>(&m_topBar);
}

fairwindsk::ui::launcher::Launcher *fairwindsk::ui::MainWindow::getLauncher() {
    return reinterpret_cast<fairwindsk::ui::launcher::Launcher *>(&m_launcher);;
}

fairwindsk::ui::bottombar::BottomBar *fairwindsk::ui::MainWindow::getBottomBar() {
    return reinterpret_cast<fairwindsk::ui::bottombar::BottomBar *>(&m_bottomBar);
}




