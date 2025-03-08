//
// Created by Raffaele Montella on 21/03/21.
//

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QMap>
#include <QDebug>
#include <QCloseEvent>
#include <QWebEngineProfile>

#include <QHotkey>

#include <FairWindSK.hpp>
#include "ui_MainWindow.h"
#include "ui/settings/Settings.hpp"
#include "ui/mydata/MyData.hpp"

#include <ui/topbar/TopBar.hpp>
#include <ui/launcher/Launcher.hpp>
#include <ui/bottombar/BottomBar.hpp>
#include <ui/about/About.hpp>

namespace Ui { class MainWindow; }

namespace fairwindsk::ui {

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

    private:

        // Close Event handler
        void closeEvent(QCloseEvent *bar) override;

        // Get Window Id using the Process Id
        static WId getWIdByPId(qint64 pId);

    public
        slots:

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

            // Invoked when the Ok button of the About widget is clicked
            void onAboutAccepted(fairwindsk::ui::about::About *aboutPage);

            // Invoked when the Save button of the Settings widget is clicked
            void onSettingsAccepted(fairwindsk::ui::settings::Settings *settingsPage) const;

            // Invoked when the Cancel button of the Settings widget is clicked
            void onSettingsRejected(fairwindsk::ui::settings::Settings *settingsPage);

            // Invoked when the Close button of the MyData widget is clicked
            void onMyDataClosed(fairwindsk::ui::mydata::MyData *myDataPage);

        // Invoked when the bottom bar button Settings is clicked
        void onHotkey();

        // Invoked when a web app have to be removed from the browser
        void onRemoveApp(const QString& name);

        // Set the windows size
        void setSize();
    private:
        // The UI pointer
        Ui::MainWindow *ui = nullptr;

        // This will be populated with the apps launched by the user for quick usage
        QMap<QString, QWidget *> m_mapHash2Widget;

        // QWidget containing useful infos
        topbar::TopBar *m_topBar = nullptr;

        // QWidget containing the launcher
        launcher::Launcher *m_launcher = nullptr;

        // QWidget containing navigation buttons
        bottombar::BottomBar *m_bottomBar = nullptr;

        // The pointer to the foreground app
        fairwindsk::AppItem *m_currentApp = nullptr;

        // The hotkey
        QHotkey *m_hotkey = nullptr;

    };

}

#endif //MAINWINDOW_HPP
