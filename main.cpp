#include <QApplication>
#include <QPushButton>
#include <QSplashScreen>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"

int main(int argc, char *argv[]) {
    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );

    // Enable DPI scaling
    //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Enable high DPI support
    //QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    // Get the splash screen logo
    QPixmap pixmap(":/resources/images/other/splash_logo.png");

    // Create a splash screen containing the logo
    QSplashScreen splash(pixmap);

    // Show the logo
    splash.show();

    // Get the FairWind singleton
    auto fairWindSK = fairwindsk::FairWindSK::getInstance();

    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/icons/fairwind_icon.png"))));

    // Create a new MainWindow object
    fairwindsk::ui::MainWindow w;

    // Register the main window
    fairWindSK->setMainWindow(&w);

    // Close the splash screen presenting the MainWindow UI
    splash.finish((QWidget *)&w);
    return QApplication::exec();
}
