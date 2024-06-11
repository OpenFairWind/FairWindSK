//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Autopilot.h" resolved

#include <QtWidgets/QAbstractButton>
#include "AutopilotBar.hpp"
#include "ui_AutopilotBar.h"

namespace fairwindsk::ui::bottombar {
    AutopilotBar::AutopilotBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AutopilotBar) {
        ui->setupUi(this);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &AutopilotBar::onHideClicked);
    }

    void AutopilotBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    AutopilotBar::~AutopilotBar() {
        delete ui;
    }
} // fairwindsk::ui::autopilot
