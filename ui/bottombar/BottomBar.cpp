//
// Created by Raffaele Montella on 12/04/21.
//



#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QToolButton>
#include <QAbstractButton>
#include "BottomBar.hpp"

#include <QtWidgets/QLabel>

namespace fairwindsk::ui::bottombar {
/*
 * BottomBar
 * Public constructor - This presents some navigation buttons at the bottom of the screen
 */
    fairwindsk::ui::bottombar::BottomBar::BottomBar(QWidget *parent) :
            QWidget(parent),
            ui(new Ui::BottomBar) {

        m_iconSize = 32;

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
 * addApp
 * Add the application icon in the shortcut area
 */
    void BottomBar::addApp(const QString& name) {

        //if (name != "http:///") {

            // Get the FairWind singleton
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();


            auto appItem = fairWindSK->getAppItemByHash(name);

            if (appItem) {

                // Create a new button
                auto *button = new QToolButton();

                // Set the app's hash value as the 's object name
                button->setObjectName("toolbutton_" + name);

                // Set the app's name as the button's text
                //button->setText(appItem->getDisplayName());

                // Set the tool tip
                button->setToolTip(appItem->getDisplayName());

                // Get the application icon
                QPixmap pixmap = appItem->getIcon();

                // Check if the icon is available
                if (!pixmap.isNull()) {
                    // Scale the icon
                    pixmap = pixmap.scaled(m_iconSize, m_iconSize);

                    // Set the app's icon as the button's icon
                    button->setIcon(pixmap);

                    // Give the button's icon a fixed square
                    button->setIconSize(QSize(m_iconSize, m_iconSize));
                }

                // Set the button's style to have an icon and some text beneath it
                //button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

                // Launch the app when the button is clicked
                connect(button, &QToolButton::released, this, &BottomBar::app_clicked);

                // Add the newly created button to the horizontal layout as a widget
                ui->horizontalLayoutApps->addWidget(button);

                // Store in buttons in a map
                m_buttons[name] = button;


            }
        //}
    }

    /*
 * removeApp
 * Remove the application icon frpm the shortcut area
 */
    void BottomBar::removeApp(const QString& name) {

        // Check for the setting application
        //if (name != "http:///") {

            // Get the button
            const auto button = m_buttons[name];

            // Add the newly created button to the horizontal layout as a widget
            ui->horizontalLayoutApps->removeWidget(button);

            // Remove the button from the array
            m_buttons.remove(name);

            // Delete the button
            delete button;

        //}
    }

    /*
 * app_clicked
 * Method called when the user wants to launch an app
 */
    void BottomBar::app_clicked() {

        // Get the sender button
        QWidget *buttonWidget = qobject_cast<QWidget *>(sender());

        // Check if the sender is valid
        if (!buttonWidget) {

            // The sender is not a widget
            return;
        }

        // Get the app's hash value from the button's object name
        QString hash = buttonWidget->objectName().replace("toolbutton_", "");

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "Apps - hash:" << hash;
        }

        // Emit the signal to tell the MainWindow to update the UI and show the app with that particular hash value
        emit foregroundAppChanged(hash);
    }

/*
 * ~BottomBar
 * BottomBar's destructor
 */
    BottomBar::~BottomBar() {

        // Delete the application icons
        for (const auto button: m_buttons) {

            // Delete the button
            delete button;
        }

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
