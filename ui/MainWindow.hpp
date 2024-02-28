//
// Created by Raffaele Montella on 21/03/21.
//

#ifndef FAIRWINDSK_MAINWINDOW_HPP
#define FAIRWINDSK_MAINWINDOW_HPP

#include <QMainWindow>
#include <QMap>
#include <QDebug>

#include <ui/topbar/TopBar.hpp>
#include <ui/bottombar/BottomBar.hpp>
#include <ui/about/About.hpp>

class BottomBar;

class TopBar;

namespace Ui {
    class MainWindow;
}

namespace fairwindsk::ui {
    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);

        ~MainWindow();

        Ui::MainWindow *getUi();

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

        // This will be populated with the apps launched by the user for quick usage
        QMap<QString, QWidget *> m_mapHash2Widget;

        // QWidget containing the loaded apps
        //apps::Apps *m_apps = nullptr;

        // QWidget containing useful infos
        topbar::TopBar *m_topBar = nullptr;
        // QWidget containing navigation buttons
        bottombar::BottomBar *m_bottonBar = nullptr;

        // The pointer to the foreground app
        //fairwind::apps::IFairWindApp *m_fairWindApp = nullptr;


    };
}

#endif //FAIRWINDS_MAINWINDOW_HPP