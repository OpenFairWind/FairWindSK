#include <QApplication>
#include <QPushButton>
#include <QSplashScreen>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QLoggingCategory>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"
#include "ui/web/Browser.hpp"
#include "ui/web/BrowserWindow.hpp"
#include "ui/web/TabWidget.hpp"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {



    // Set the organization name
    QCoreApplication::setOrganizationName("uniparthenope.it");

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );

    // Enable DPI scaling
    //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Enable high DPI support
    //QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // The QT application
    QApplication a(argc, argv);

    // Get the splash screen logo
    QPixmap pixmap(":/resources/images/other/splash_logo.png");

    // Create a splash screen containing the logo
    QSplashScreen splash(pixmap);

    // Show the logo
    splash.show();

    // Get the FairWind singleton
    auto fairWindSK = fairwindsk::FairWindSK::getInstance();

    // Check if the virtual keyboard have to be activated
    if (fairWindSK->useVirtualKeyboard()) {

        // Activate the virtual keyboard
        qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    }

    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/icons/fairwind_icon.png"))));

    // Create a new MainWindow object
    fairwindsk::ui::MainWindow w;

    // Close the splash screen presenting the MainWindow UI
    splash.finish((QWidget *)&w);

    QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);

    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);

    /*
    QUrl url = QUrl("http://172.24.1.1:3000/");
    fairwindsk::ui::web::Browser browser;
    fairwindsk::ui::web::BrowserWindow *window = browser.createHiddenWindow();
    window->tabWidget()->setUrl(url);
    window->show();
    splash.finish((QWidget *)window);
     */

    return QApplication::exec();
}
