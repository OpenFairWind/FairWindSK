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

        // Create the MOB bar
        m_MOBBar = new MOBBar();

        // Create the autopilot bar
        m_AutopilotBar = new AutopilotBar();

        // Create the Anchor bar
        m_AnchorBar = new AnchorBar();

        // Create the alarms bar
        m_AlarmsBar = new AlarmsBar();

        // Add the MOB bar to the layout
        ui->gridLayout->addWidget(m_MOBBar,0,0);

        // Add the autopilot bar to the layout
        ui->gridLayout->addWidget(m_AutopilotBar,1,0);

        // Add the MOB bar to the layout
        ui->gridLayout->addWidget(m_AnchorBar,2,0);

        // Add the alarms bar to the layout
        ui->gridLayout->addWidget(m_AlarmsBar,3,0);

        // Emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_MyData, &QToolButton::released, this, &BottomBar::myData_clicked);

        // Emit a signal when the MOB tool button from the UI is clicked
        connect(ui->toolButton_MOB, &QToolButton::released, this, &BottomBar::mob_clicked);

        // Emit a signal when the MOB tool button from the UI is clicked
        connect(ui->toolButton_Autopilot, &QToolButton::released, this, &BottomBar::autopilot_clicked);

        // Emit a signal when the Apps tool button from the UI is clicked
        connect(ui->toolButton_Apps, &QToolButton::released, this, &BottomBar::apps_clicked);

        // Emit a signal when the MOB tool button from the UI is clicked
        connect(ui->toolButton_Anchor, &QToolButton::released, this, &BottomBar::anchor_clicked);

        // Emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Alarms, &QToolButton::released, this, &BottomBar::alarms_clicked);

        // Emit a signal when the Settings tool button from the UI is clicked
        connect(ui->toolButton_Settings, &QToolButton::released, this, &BottomBar::settings_clicked);
    }

    /*
     * setAutopilotIcon
     * Set Autopilot icon visibility
     */
    void BottomBar::setAutopilotIcon(const bool value) const
    {

        // Set button icon visibility
        ui->toolButton_Autopilot->setEnabled(value);
    }

    /*
     * setAnchorIcon
     * Set Anchor icon visibility
     */
    void BottomBar::setAnchorIcon(const bool value) const
    {

        // Set button icon visibility
        ui->toolButton_Anchor->setEnabled(value);
    }

    /*
     * setMOBIcon
     * Set MOB icon visibility
     */
    void BottomBar::setMOBIcon(const bool value) const
    {

        // Set button icon visibility
        ui->toolButton_MOB->setEnabled(value);
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

    void BottomBar::autopilot_clicked() {
        // Check if the autopilot bar is available
        if (m_AutopilotBar) {

            // Toggle the Autopilot bar
            m_AutopilotBar->setVisible(!m_AutopilotBar->isVisible());
        }
    }

/*
 * apps_clicked
 * Method called when the user wants to view the apps screen
 */
    void BottomBar::apps_clicked() {

        // Emit the signal to tell the MainWindow to update the UI and show the apps screen
        emit setApps();
    }


    void BottomBar::anchor_clicked() {
        // Check if the autopilot bar is available
        if (m_AnchorBar) {

            // Toggle the Anchor bar
            m_AnchorBar->setVisible(!m_AnchorBar->isVisible());
        }

    }

/*
 * settings_clicked
 * Method called when the user wants to view the alarms screen
 */
    void BottomBar::alarms_clicked() {

        // Check if the alarms bar is available
        if (m_AlarmsBar) {

            // Toggle the alarms bar
            m_AlarmsBar->setVisible(!m_AlarmsBar->isVisible());
        }
    }

/*
 * settings_clicked
 * Method called when the user wants to view the settings screen
 */
    void BottomBar::settings_clicked() {

        // Emit the signal to tell the MainWindow to update the UI and show the settings screen
        emit setSettings();
    }

/*
 * ~BottomBar
 * BottomBar's destructor
 */
    BottomBar::~BottomBar() {

        // Check if the autopilot bar is instanced
        if (m_AutopilotBar) {
            // Delete the autopilot bar
            delete m_AutopilotBar;

            // Set the autopilot bar pointer to null
            m_AutopilotBar = nullptr;
        }

        // Check if the alarms bar is instanced
        if (m_AlarmsBar) {
            // Delete the alarms bar
            delete m_AlarmsBar;

            // Set the alarms bar pointer to null
            m_AlarmsBar = nullptr;
        }

        // Check if the MOB bar is instanced
        if (m_MOBBar) {
            // Delete the MOB bar
            delete m_MOBBar;

            // Set the MOB bar pointer to null
            m_MOBBar = nullptr;
        }

        // Check if the anchor bar is instanced
        if (m_AnchorBar) {
            // Delete the anchor bar
            delete m_AnchorBar;

            // Set the anchor bar pointer to null
            m_AnchorBar = nullptr;
        }

        // Check if the UI is instanced
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the UI pointer to null
            ui = nullptr;
        }
    }
}