//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AlarrmsBar.h" resolved

#include <QPushButton>

#include "AlarmsBar.hpp"
#include "ui_AlarmsBar.h"

namespace fairwindsk::ui::bottombar {
    AlarmsBar::AlarmsBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AlarmsBar) {
        ui->setupUi(this);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AlarmsBar::onHideClicked);

        // Not visible by default
        QWidget::setVisible(false);
    }

    void AlarmsBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    AlarmsBar::~AlarmsBar() {
        delete ui;
    }
} // fairwindsk::ui::bottombar
