//
// Created by Raffaele Montella on 18/03/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Settings.h" resolved


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

    Settings::Settings(QWidget *parent, QWidget *currenWidget): QWidget(parent), ui(new Ui::Settings) {
        ui->setupUi(this);

        ui->tabWidget->addTab(new Main(), tr("Main"));
        ui->tabWidget->addTab(new Connection(), tr("Connection"));
        ui->tabWidget->addTab(new SignalK(), tr("Signal K"));
        ui->tabWidget->addTab(new Apps(), tr("Apps"));

        m_currentWidget = currenWidget;
        connect(ui->buttonBox,&QDialogButtonBox::accepted,this,&Settings::onAccepted);
    }

    void Settings::onAccepted() {
        emit accepted(this);
    }

    QWidget *Settings::getCurrentWidget() {
        return m_currentWidget;
    }

    Settings::~Settings() {
        delete ui;
    }

} // fairwindsk::ui
