//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Autopilot.h" resolved

#include "Autopilot.hpp"
#include "ui_Autopilot.h"

namespace fairwindsk::ui::autopilot {
    Autopilot::Autopilot(QWidget *parent) :
            QWidget(parent), ui(new Ui::Autopilot) {
        ui->setupUi(this);
    }

    Autopilot::~Autopilot() {
        delete ui;
    }
} // fairwindsk::ui::autopilot
