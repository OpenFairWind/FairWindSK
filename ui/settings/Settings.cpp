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

        auto fairWindSk = FairWindSK::getInstance();

        m_configuration.setRoot(fairWindSk->getConfiguration()->getRoot());

        for (const auto& hash: fairWindSk->getAppsHashes()) {
            m_mapHash2AppItem.insert(hash, fairWindSk->getAppItemByHash(hash));
        }

        ui->tabWidget->addTab(new Main(this), tr("Main"));
        ui->tabWidget->addTab(new Connection(this), tr("Connection"));
        ui->tabWidget->addTab(new SignalK(this), tr("Signal K"));
        ui->tabWidget->addTab(new Apps(this), tr("Apps"));

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

    QList<QString> Settings::getAppsHashes() {
        return m_mapHash2AppItem.keys();
    }



    AppItem *Settings::getAppItemByHash(QString hash) {
        return m_mapHash2AppItem[hash];
    }

    Configuration *Settings::getConfiguration() {
        return &m_configuration;
    }

} // fairwindsk::ui
