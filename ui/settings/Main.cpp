//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Main.h" resolved

#include "Main.hpp"
#include "ui_Main.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    Main::Main(QWidget *parent) :
            QWidget(parent), ui(new Ui::Main) {
        ui->setupUi(this);

        if (fairwindsk::FairWindSK::getVirtualKeyboard()) {
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
            fairwindsk::FairWindSK::setVirtualKeyboard(true);
        } else {
            fairwindsk::FairWindSK::setVirtualKeyboard(false);
        }
    }

    Main::~Main() {
        delete ui;
    }
} // fairwindsk::ui::settings
