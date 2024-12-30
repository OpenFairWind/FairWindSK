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

        // Not visible by default
        QWidget::setVisible(false);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AnchorBar::onHideClicked);

        connect(ui->toolButton_Reset, &QPushButton::clicked, this, &AnchorBar::onResetClicked);
        connect(ui->toolButton_Up, &QPushButton::clicked, this, &AnchorBar::onUpClicked);
        connect(ui->toolButton_Raise, &QPushButton::clicked, this, &AnchorBar::onRaiseClicked);
        connect(ui->toolButton_RadiusDec, &QPushButton::clicked, this, &AnchorBar::onRadiusDecClicked);
        connect(ui->toolButton_RadiusInc, &QPushButton::clicked, this, &AnchorBar::onRadiusIncClicked);
        connect(ui->toolButton_Drop, &QPushButton::clicked, this, &AnchorBar::onDropClicked);
        connect(ui->toolButton_Down, &QPushButton::clicked, this, &AnchorBar::onDownClicked);
        connect(ui->toolButton_Release, &QPushButton::clicked, this, &AnchorBar::onReleaseClicked);

    }

    void AnchorBar::onHideClicked() {
        setVisible(false);
        emit hidden();
    }

    void AnchorBar::onResetClicked() {

        emit resetCounter();
    }

    void AnchorBar::onUpClicked() {

        emit chainUp();
    }

    void AnchorBar::onRaiseClicked() {

        emit raiseAnchor();
    }

    void AnchorBar::onRadiusDecClicked() {

        emit radiusDec();
    }

    void AnchorBar::onRadiusIncClicked() {

        emit radiusInc();
    }

    void AnchorBar::onDropClicked() {

        emit dropAnchor();
    }

    void AnchorBar::onDownClicked() {

        emit chainDown();
    }

    void AnchorBar::onReleaseClicked() {

        emit chainRelease();
    }

    AnchorBar::~AnchorBar() {
        delete ui;
    }
} // fairwindsk::ui::bottombar
