//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MOBBar.h" resolved

#include "MOBBar.hpp"
#include "ui_MOBBar.h"

namespace fairwindsk::ui::bottombar {
    MOBBar::MOBBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::MOBBar) {
        ui->setupUi(this);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->pushButton_MOB_Cancel, &QPushButton::clicked, this, &MOBBar::onCancelClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &MOBBar::onHideClicked);
    }

    void MOBBar::MOB() {
        setVisible(true);
    }

    void MOBBar::onCancelClicked() {
        setVisible(false);
        emit cancelMOB();
    }

    void MOBBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    MOBBar::~MOBBar() {
        delete ui;
    }
} // fairwindsk::ui::bottombar
