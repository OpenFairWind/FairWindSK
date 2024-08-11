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

    Settings::Settings(QWidget *parent, QWidget *currenWidget, Configuration *currentConfiguration): QWidget(parent), ui(new Ui::Settings) {
        ui->setupUi(this);

        if (currentConfiguration) {
            m_currentConfiguration = currentConfiguration;
            m_configuration.setFilename(m_currentConfiguration->getFilename());
            m_configuration.setRoot(m_currentConfiguration->getRoot());
        }


        initTabs();

        m_currentWidget = currenWidget;

	m_pushButtonQuit = new QPushButton("Quit");

	ui->buttonBox->addButton(m_pushButtonQuit,QDialogButtonBox::ActionRole);

        connect(ui->buttonBox,&QDialogButtonBox::accepted,this,&Settings::onAccepted);
        connect(ui->buttonBox,&QDialogButtonBox::rejected,this,&Settings::onRejected);
        connect(ui->buttonBox,&QDialogButtonBox::clicked,this,&Settings::onClicked);
    }

    void Settings::onClicked(QAbstractButton *button) {
        if (ui->buttonBox->button(QDialogButtonBox::Discard) == (QPushButton *)button) {


            emit rejected(this);
        }
        else if (ui->buttonBox->button(QDialogButtonBox::Reset) == (QPushButton *)button) {


            auto fairWindSk = FairWindSK::getInstance();

            m_configuration.setRoot(m_currentConfiguration->getRoot());

            initTabs();


        }
        else if (ui->buttonBox->button(QDialogButtonBox::RestoreDefaults) == (QPushButton *)button) {


            m_configuration.setDefault();

            initTabs();
        } else if ((QPushButton *)button == m_pushButtonQuit) {
		QApplication::exit(0);
	}	
    }

    void Settings::initTabs() {
        while (ui->tabWidget->count() > 0) {
	    auto tab = ui->tabWidget->widget(0);
	    ui->tabWidget->removeTab(0);
	    delete tab;
	} 
        
        ui->tabWidget->addTab(new Main(this), tr("Main"));
        ui->tabWidget->addTab(new Connection(this), tr("Connection"));
        ui->tabWidget->addTab(new SignalK(this), tr("Signal K"));
        ui->tabWidget->addTab(new Apps(this), tr("Apps"));
        ui->tabWidget->setCurrentIndex(0);
    }


    void Settings::onAccepted() {



        FairWindSK::getInstance()->getConfiguration()->setRoot(m_configuration.getRoot());

        FairWindSK::getInstance()->getConfiguration()->save();

        emit accepted(this);
    }

    void Settings::onRejected() {
        emit rejected(this);
    }

    QWidget *Settings::getCurrentWidget() {
        return m_currentWidget;
    }

    Settings::~Settings() {
        while (ui->tabWidget->count() > 0) {
            auto tab = ui->tabWidget->widget(0);
            ui->tabWidget->removeTab(0);
            delete tab;
        }

	if (m_pushButtonQuit) {
		delete m_pushButtonQuit;

		m_pushButtonQuit = nullptr;
	}

        delete ui;
    }

    Configuration *Settings::getConfiguration() {
        return &m_configuration;
    }

} // fairwindsk::ui
