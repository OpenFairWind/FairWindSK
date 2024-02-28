//
// Created by Raffaele Montella on 12/04/21.
//

#include <ui_BottomBar.h>
#include <FairWindSK.hpp>
#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QAbstractButton>
#include "BottomBar.hpp"

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

    /*

        ui->toolButton_MyData->setIcon(QPixmap::fromImage(myDataFairWindApp->getIcon()));



        ui->toolButton_MOB->setIcon(QPixmap::fromImage(mobFairWindApp->getIcon()));



        ui->toolButton_Apps->setIcon(QPixmap::fromImage(appsFairWindApp->getIcon()));



        ui->toolButton_Alarms->setIcon(QPixmap::fromImage(alarmsFairWindApp->getIcon()));



        ui->toolButton_Settings->setIcon(QPixmap::fromImage(settingsFairWindApp->getIcon()));
*/

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
void fairwindsk::ui::bottombar::BottomBar::myData_clicked() {
    // Emit the signal to tell the MainWindow to update the UI and show the settings screen
    emit setMyData();
}

/*
 * myData_clicked
 * Method called when the user click on Men Over Board (MOB)
 */
void fairwindsk::ui::bottombar::BottomBar::mob_clicked() {
    // Emit the signal to tell the MainWindow to update the UI and show the settings screen
    emit setMOB();
}

/*
 * apps_clicked
 * Method called when the user wants to view the apps screen
 */
void fairwindsk::ui::bottombar::BottomBar::apps_clicked() {
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
void fairwindsk::ui::bottombar::BottomBar::alarms_clicked() {
    // Emit the signal to tell the MainWindow to update the UI and show the settings screen
    emit setAlarms();
}

/*
 * settings_clicked
 * Method called when the user wants to view the settings screen
 */
void fairwindsk::ui::bottombar::BottomBar::settings_clicked() {
    // Emit the signal to tell the MainWindow to update the UI and show the settings screen
    emit setSettings();
}



/*
 * ~BottomBar
 * BottomBar's destructor
 */
fairwindsk::ui::bottombar::BottomBar::~BottomBar() {
    delete ui;
}