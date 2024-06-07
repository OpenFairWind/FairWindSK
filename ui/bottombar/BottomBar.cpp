//
// Created by Raffaele Montella on 12/04/21.
//



#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QAbstractButton>
#include "BottomBar.hpp"

namespace fairwindsk::ui::bottombar {
/*
 * BottomBar
 * Public constructor - This presents some navigation buttons at the bottom of the screen
 */
    fairwindsk::ui::bottombar::BottomBar::BottomBar(QWidget *parent) :
            QWidget(parent),
            ui(new Ui::BottomBar) {

        // Setup the UI
        ui->setupUi(parent);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        m_AlarmsBar = new AlarmsBar();
        m_AlarmsBar->setVisible(false);
        ui->gridLayout->addWidget(m_AlarmsBar,0,0);

        m_MOBBar = new MOBBar();
        m_MOBBar->setVisible(false);
        ui->gridLayout->addWidget(m_MOBBar,1,0);

        showMOB(false);
        showAlarms(false);





        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_MyData, &QToolButton::released, this, &BottomBar::myData_clicked);

        // emit a signal when the MOB tool button from the UI is clicked
        connect(ui->toolButton_MOB, &QToolButton::released, this, &BottomBar::mob_clicked);

        // emit a signal when the Apps tool button from the UI is clicked
        connect(ui->toolButton_Apps, &QToolButton::released, this, &BottomBar::apps_clicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Alarms, &QToolButton::released, this, &BottomBar::alarms_clicked);

        // emit a signal when the Settings tool button from the UI is clicked
        connect(ui->toolButton_Settings, &QToolButton::released, this, &BottomBar::settings_clicked);
    }


/*
 * myData_clicked
 * Method called when the user wants to view the stored data
 */
    void BottomBar::myData_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit setMyData();
    }

/*
 * myData_clicked
 * Method called when the user click on Men Over Board (MOB)
 */
    void BottomBar::mob_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        m_MOBBar->MOB();
    }

/*
 * apps_clicked
 * Method called when the user wants to view the apps screen
 */
    void BottomBar::apps_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the apps screen
        emit setApps();
        /*
        auto fairWind = fairwind::FairWind::getInstance();
        auto signalKDocument = fairWind->getSignalKDocument();

        signalk::Note note1("Note1", "This is my note1", QGeoCoordinate(40,14));
        signalk::Note note2("Note2", "This is my note2", QGeoCoordinate(40.56,14.28));

        signalKDocument->set("resources.notes", note1);
        signalKDocument->set("resources.notes", note2);
        signalKDocument->save("signalkmodel.json");
         */
    }

/*
 * settings_clicked
 * Method called when the user wants to view the alarms screen
 */
    void BottomBar::alarms_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        m_AlarmsBar->setVisible(true);
    }

/*
 * settings_clicked
 * Method called when the user wants to view the settings screen
 */
    void BottomBar::settings_clicked() {
        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit setSettings();
    }

    void BottomBar::showMOB(bool show) {
        /*
        for (auto widget: IterableLayoutAdapter<>(ui->horizontalLayoutMOB)) {
            if (show) {
                widget->show();
            } else {
                widget->hide();
            }

        }
         */


    }

    void BottomBar::showAlarms(bool show) {
        /*
        for (auto widget: IterableLayoutAdapter<>(ui->horizontalLayoutAlarms)) {
            if (show) {
                widget->show();
            } else {
                widget->hide();
            }

        }
         */
    }


/*
 * ~BottomBar
 * BottomBar's destructor
 */
    BottomBar::~BottomBar() {
        delete m_MOBBar;
        delete m_AlarmsBar;
        delete ui;
    }
}