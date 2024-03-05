//
// Created by Raffaele Montella on 21/03/21.
//

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QMap>
#include <QDebug>

#include <FairWindSK.hpp>
#include "ui_MainWindow.h"

#include <ui/topbar/TopBar.hpp>
#include <ui/launcher/Launcher.hpp>
#include <ui/bottombar/BottomBar.hpp>
#include <ui/about/About.hpp>

class TopBar;
class Launcher;
class BottomBar;

namespace Ui { class MainWindow; }

namespace fairwindsk::ui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

        ~MainWindow() override;

        Ui::MainWindow *getUi();

        TopBar *getTopBar();
        Launcher *getLauncher();
        BottomBar *getBottomBar();


    public
        slots:
                void setForegroundApp(QString hash);

        void onApps();

        void onSettings();
        void onUpperLeft();
        void onUpperRight();

        void onAboutAccepted(about::About *aboutPage);



    private:
        Ui::MainWindow *ui;

        //fairwindsk::ui::web::Browser *m_browser;

        // This will be populated with the apps launched by the user for quick usage
        QMap<QString, QWidget *> m_mapHash2Widget;

        // QWidget containing the loaded apps
        //apps::Apps *m_apps = nullptr;

        // QWidget containing useful infos
        topbar::TopBar *m_topBar = nullptr;

        // QWidget containing the launcher
        launcher::Launcher *m_launcher = nullptr;

        // QWidget containing navigation buttons
        bottombar::BottomBar *m_bottomBar = nullptr;

        // The pointer to the foreground app
        fairwindsk::AppItem *m_currentApp = nullptr;


    };
}

#endif //MAINWINDOW_HPP