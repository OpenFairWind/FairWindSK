//
// Created by Raffaele Montella on 18/03/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Settings.h" resolved

#include <iostream>

#include <QTranslator>
#include <QMessageBox>
#include "Settings.hpp"

#include "FairWindSK.hpp"

#include "Main.hpp"
#include "Connection.hpp"
#include "SignalK.hpp"
#include "Apps.hpp"


#include "ui_Settings.h"

namespace fairwindsk::ui::settings {

    /*
 * Settings
 * The class constructor
 */
    Settings::Settings(QWidget *parent, QWidget *currenWidget): QWidget(parent), ui(new Ui::Settings) {

        // Set the UI
        ui->setupUi(this);

        // Copy the pointer locally
        m_currentConfiguration = FairWindSK::getInstance()->getConfiguration();

        // Set the filename of the local configuration as the file name of FairWindSK configuration
        m_configuration.setFilename(m_currentConfiguration->getFilename());

        // Get the current configuration root as a json object
        const auto configurationAsJson = m_currentConfiguration->getRoot();

        // Set the local configuration with the json object
        m_configuration.setRoot(configurationAsJson);

        // Set the default tab
        auto currentIndex = 0;

        // Check if no server is set
        if (m_configuration.getSignalKServerUrl().isEmpty()) {

            // Set the tab index to the connection tab
            currentIndex = 1;
        }

        // Initialize the tabs
        initTabs(currentIndex);

        // Store the current widget pointer
        m_currentWidget = currenWidget;

        // Create a push button labelling it with Quit
	    m_pushButtonQuit = new QPushButton(tr("Quit"));

        // Add the push button to the button box sith ActionRole
	    ui->buttonBox->addButton(m_pushButtonQuit,QDialogButtonBox::ActionRole);

        // Connect the button box accepted signal with onAccepted method
        connect(ui->buttonBox,&QDialogButtonBox::accepted,this,&Settings::onAccepted);

        // Connect the button box rejected signal with onRejected method
        connect(ui->buttonBox,&QDialogButtonBox::rejected,this,&Settings::onRejected);

        // Connect the button box clicked signal with onClicked method
        connect(ui->buttonBox,&QDialogButtonBox::clicked,this,&Settings::onClicked);
    }

    /*
 * removeTabs
 * Remove all  tabs
 */
    void Settings::removeTabs() {
        // While there is at least a tab in the tab widget...
        while (ui->tabWidget->count() > 0) {

            // Get the reference of the tab in position 0
            const auto tab = ui->tabWidget->widget(0);

            // Remove the tab
            ui->tabWidget->removeTab(0);

            // Delete the object
            delete tab;
        }
    }

    /*
 * initTabs
 * Add tabs, then set the current index
 */
    void Settings::initTabs(const int currentIndex) {

        // Remove tabs if present
        removeTabs();

        // Add the main tab
        ui->tabWidget->addTab(new Main(this), tr("Main"));

        // Add the connection tab
        ui->tabWidget->addTab(new Connection(this), tr("Connection"));

        // Add the signal k tab
        ui->tabWidget->addTab(new SignalK(this), tr("Signal K"));

        // Add the applications tab
        ui->tabWidget->addTab(new Apps(this), tr("Applications"));

        // Set the current tab index
        ui->tabWidget->setCurrentIndex(currentIndex);
    }

    /*
     * onClicked
     * Invoked when a push button in the button bock is clicked
     */
    void Settings::onClicked(QAbstractButton *button) {

        // Check if the button is Discard
        if (ui->buttonBox->button(QDialogButtonBox::Discard) == dynamic_cast<QPushButton*>(button)) {

            // Emits a rejected signal
            emit rejected(this);
        }
        // Check if the button is Reset
        else if (ui->buttonBox->button(QDialogButtonBox::Reset) == dynamic_cast<QPushButton*>(button)) {

            // Get the json configuration object of the current configuration
            auto configurationAsJson = m_currentConfiguration->getRoot();

            // Set the local configuration as the current one
            m_configuration.setRoot(configurationAsJson);

            // Get the current tab index
            const auto currentIndex = ui->tabWidget->currentIndex();

            // Initialize all the tabs setting the currentIndex
            initTabs(currentIndex);

        }
        // Check if the button is Default
        else if (ui->buttonBox->button(QDialogButtonBox::RestoreDefaults) == dynamic_cast<QPushButton*>(button)) {

            // Set the configuration with the default values
            m_configuration.setDefault();

            // Get the current tab index
            const auto currentIndex = ui->tabWidget->currentIndex();

            // Initialize all the tabs setting the currentIndex
            initTabs(currentIndex);

        }
        // Check if the button is Quit
        else if ( m_pushButtonQuit == dynamic_cast<QPushButton*>(button)) {

            // Quit the application
            QApplication::exit(0);
        }
    }

    /*
     * onAccepted
     * Invoked when a push button connected with the accepted signal is clicked
     */
    void Settings::onAccepted() {

        // Get the configuration root element
        const auto configurationAsJson = m_configuration.getRoot();

        // Set the new configuration in the FairWindSK singleton instance
        FairWindSK::getInstance()->getConfiguration()->setRoot(configurationAsJson);

        // Save the configuration permanently
        FairWindSK::getInstance()->getConfiguration()->save();

        // Emit an accepted signal
        emit accepted(this);
    }

    /*
     * onRejected
     * Invoked when a push button connected with the rejected signal is clicked
     */
    void Settings::onRejected() {

        // Emit a reject signal
        emit rejected(this);
    }

    /*
     * getCurrentWidget
     * Return the current widget
     */
    QWidget *Settings::getCurrentWidget() {
        return m_currentWidget;
    }

    /*
     * getConfiguration
     * Get a pointer  to the local configuration
     */
    Configuration *Settings::getConfiguration() {
        return &m_configuration;
    }

    /*
     * ~Settings
     * Class destructor
     */
    Settings::~Settings() {

        // Remove the tabs
        removeTabs();

        // Check if the quit push button is allocated
	    if (m_pushButtonQuit) {

	        // Delete the button
		    delete m_pushButtonQuit;

	        // Set the pointer to null
		    m_pushButtonQuit = nullptr;
	    }

        // Check if the ui pointer is valid
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the ui pointer to null
            ui = nullptr;
        }
    }



} // fairwindsk::ui
