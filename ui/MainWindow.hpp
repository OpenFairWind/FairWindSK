//
// Created by Raffaele Montella on 21/03/21.
//

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QMap>
#include <QDebug>
#include <QCloseEvent>
#include <QEventLoop>
#include <QWebEngineProfile>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QHotkey>
#endif

#include <FairWindSK.hpp>
#include "ui_MainWindow.h"
#include "ui/settings/Settings.hpp"
#include "ui/mydata/MyData.hpp"

#include <ui/topbar/TopBar.hpp>
#include <ui/launcher/Launcher.hpp>
#include <ui/bottombar/BottomBar.hpp>
#include <ui/about/About.hpp>

namespace Ui { class MainWindow; }
class QLabel;
class QHBoxLayout;
class QEventLoop;
class QFrame;
class QVBoxLayout;

namespace fairwindsk::ui {

    struct DrawerButtonSpec {
        QString text;
        int result = 0;
        bool isDefault = false;
    };

    namespace topbar {class TopBar; }
    namespace launcher {class Launcher;}
    namespace bottombar {class BottomBar;}
    namespace settings {class Settings;}

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:

        // Explicit constructor
        explicit MainWindow(QWidget *parent = nullptr);

        // Destructor
        ~MainWindow() override;

        // Get the pointer to UI
        Ui::MainWindow *getUi();

        // Get the pointer to the top bar
        topbar::TopBar *getTopBar();

        // Get the pointer to the launcher
        launcher::Launcher *getLauncher();

        // Get the pointer to the bottom bar
        bottombar::BottomBar *getBottomBar();

        static MainWindow *instance(QWidget *context = nullptr);
        int execDrawer(const QString &title, QWidget *content, const QList<DrawerButtonSpec> &buttons, int defaultResult = 0);
        void finishActiveDrawer(int result = 0);

    private:
        bool isOverlayOpen() const;
        void setChromeEnabled(bool enabled) const;
        void setDrawerEnabled(bool enabled) const;
        void clearDrawer();
        bool isDrawerOpen() const;
        void showOverlay(QWidget *page);
        void closeOverlay(QWidget *page, QWidget *fallbackWidget);
        bool closeSettingsPage(QWidget *fallbackWidget = nullptr, bool showFallback = true, bool exitAfterSave = false);
        void closeAboutPage(QWidget *fallbackWidget = nullptr);
        void syncTopBarToCurrentPage();
        void showLauncher();
        void ensureAboutPage(QWidget *fallbackWidget = nullptr);
        void ensureMyDataPage(QWidget *fallbackWidget = nullptr);
        void ensureSettingsPage(QWidget *fallbackWidget = nullptr);
        void prewarmPersistentPages();

        // Close Event handler
        void closeEvent(QCloseEvent *bar) override;

        // Get Window Id using the Process Id
        static WId getWIdByPId(qint64 pId);

    public slots:

        // Set foreground application by hash
        void setForegroundApp(const QString& hash);

        // Invoked when the bottom bar button Apps (Home) is clicked
        void onApps();

        // Invoked when the bottom bar button Settings is clicked
        void onSettings();

        // Invoked when the bottom bar button MyData is clicked
        void onMyData();

        // Invoked when the top bar upper left button is clicked
        void onUpperLeft();

        // Invoked when the Close button of the About widget is clicked
        void onAboutClosed(fairwindsk::ui::about::About *aboutPage);

        // Invoked when the Close button of the MyData widget is clicked
        void onMyDataClosed(fairwindsk::ui::mydata::MyData *myDataPage);

        // Invoked when the bottom bar button Settings is clicked
        void onHotkey();

        // Invoked when a web app have to be removed from the browser
        void onRemoveApp(const QString& name);

        // Set the windows size
        void setSize();
        void applyRuntimeConfiguration();
    private:
        // The UI pointer
        Ui::MainWindow *ui = nullptr;

        // This will be populated with the apps launched by the user for quick usage
        QMap<QString, QWidget *> m_mapHash2Widget;

        QWidget *m_activeOverlay = nullptr;
        fairwindsk::ui::about::About *m_aboutPage = nullptr;
        fairwindsk::ui::mydata::MyData *m_myDataPage = nullptr;
        fairwindsk::ui::settings::Settings *m_settingsPage = nullptr;
        QWidget *m_dialogDrawer = nullptr;
        QLabel *m_dialogDrawerTitle = nullptr;
        QWidget *m_dialogDrawerContentHost = nullptr;
        QVBoxLayout *m_dialogDrawerContentLayout = nullptr;
        QHBoxLayout *m_dialogDrawerButtonsLayout = nullptr;
        QEventLoop *m_activeDrawerLoop = nullptr;
        int *m_activeDrawerResult = nullptr;
        bool m_quitConfirmed = false;

        // QWidget containing useful infos
        topbar::TopBar *m_topBar = nullptr;

        // QWidget containing the launcher
        launcher::Launcher *m_launcher = nullptr;

        // QWidget containing navigation buttons
        bottombar::BottomBar *m_bottomBar = nullptr;

        // The pointer to the foreground app
        fairwindsk::AppItem *m_currentApp = nullptr;

        // The hotkey
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QHotkey *m_hotkey = nullptr;
#endif

        static MainWindow *s_instance;

    };

}

#endif //MAINWINDOW_HPP
