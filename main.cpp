#include <QApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QTranslator>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QSettings>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"


using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {

    // The translator
    QTranslator translator;

    // Set the organization name
    QCoreApplication::setOrganizationName("uniparthenope.it");

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );

    // The QT application
    QApplication app(argc, argv);

    // Install the translator
    QApplication::installTranslator(&translator);

    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png"))));

    // Get the splash screen logo
    const QPixmap pixmap(":/resources/images/other/splash_logo.png");

    // Create a splash screen containing the logo
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint);

    // Show the logo
    splash.show();

    // Show message
    splash.showMessage(QObject::tr("Welcome to FairWindSK a GUI for the Signal K server!"), 500, Qt::white);

    // Get the FairWind singleton
    const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

    // Load the configuration inside the FairWind singleton itself
    fairWindSK->loadConfig();

    // Show message
    splash.showMessage(QObject::tr("Connecting to the Signal K Server..."), 500, Qt::white);

    // Connect to the Signal K server...
    fairWindSK->startSignalK();

    // Show message
    splash.showMessage(QObject::tr("Loading applications..."), 500, Qt::white);

    // Load the apps inside the FairWind singleton itself
    fairWindSK->loadApps();

    // Set web profile options
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,true);

    // Create a new MainWindow object
    fairwindsk::ui::MainWindow w;

    // Close the splash screen presenting the MainWindow UI
    splash.finish((QWidget *) &w);

    // Run the application
    const auto result = QApplication::exec();

    // Delete the FairWindSK singleton
    delete fairWindSK;

    // Return the result
    return result;
}
