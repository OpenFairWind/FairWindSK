//
// Created by Raffaele Montella on 08/12/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AnchorBar.h" resolved


#include <QPushButton>

#include "AnchorBar.hpp"
#include "ui_AnchorBar.h"


namespace fairwindsk::ui::bottombar {
    AnchorBar::AnchorBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AnchorBar) {
        ui->setupUi(this);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AnchorBar::onHideClicked);
    }

    void AnchorBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    AnchorBar::~AnchorBar() {
        delete ui;
    }
} // fairwindsk::ui::bottombar
