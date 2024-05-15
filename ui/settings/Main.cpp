//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Main.h" resolved

#include "Main.hpp"
#include "ui_Main.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    Main::Main(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Main) {

        m_settings = settings;

        ui->setupUi(this);

        if (m_settings->getConfiguration()->getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->checkBox_virtualkeboard, &QCheckBox::stateChanged, this, &Main::onVirtualKeyboard);
    }

    void Main::onVirtualKeyboard(int state) {

        auto fairWindSK = FairWindSK::getInstance();

        if (state == Qt::Checked) {
            m_settings->getConfiguration()->setVirtualKeyboard(true);
        } else {
            m_settings->getConfiguration()->setVirtualKeyboard(false);
        }
    }

    Main::~Main() {
        delete ui;
    }
} // fairwindsk::ui::settings
