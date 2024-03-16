#include <QApplication>
#include <QSplashScreen>
#include <QTimer>


#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QSettings>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"



using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {


    // Initialize the QT managed settings
    QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

    // Get the name of the FairWind++ configuration file
    bool useVirtualKeyboard = settings.value("virtualKeyboard", false).toBool();

    // Store the configuration in the settings
    settings.setValue("virtualKeyboard", useVirtualKeyboard);

    if (useVirtualKeyboard) {
        qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    }

    // Set the organization name
    QCoreApplication::setOrganizationName("uniparthenope.it");

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );

    // The QT application
    QApplication app(argc, argv);


    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/icons/fairwind_icon.png"))));

    // Get the splash screen logo
    QPixmap pixmap(":/resources/images/other/splash_logo.png");

    // Create a splash screen containing the logo
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint);

    // Show the logo
    splash.show();

    // Show message
    splash.showMessage("Welcome to FairWindSK a GUI for the Signal K server!", 500, Qt::white);

    // Get the FairWind singleton
    auto fairWindSK = fairwindsk::FairWindSK::getInstance();

    // Load the configuration inside the FairWind singleton itself
    fairWindSK->loadConfig();

    // Show message
    splash.showMessage("Connecting to the Signal K Server...", 500, Qt::white);

    // Connect to the Signal K server...
    if (fairWindSK->startSignalK()) {
        // Show message
        splash.showMessage("Loading applications...", 500, Qt::white);

        // Load the apps inside the FairWind singleton itself
        if (fairWindSK->loadApps()) {

            QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
            QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
            QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,true);

            // Create a new MainWindow object
            fairwindsk::ui::MainWindow w;

            // Show the window fullscreen
            w.showFullScreen();

            // Close the splash screen presenting the MainWindow UI
            splash.finish((QWidget *) &w);

            return QApplication::exec();
        }
    }

    splash.close();
    QApplication::exit(-1);

}
