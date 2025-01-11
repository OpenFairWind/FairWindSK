//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_NavigationBar.h" resolved

#include "NavigationBar.hpp"
#include "ui_NavigationBar.h"

#include <QToolButton>

namespace fairwindsk::ui::web {
    NavigationBar::NavigationBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::NavigationBar) {
        ui->setupUi(this);

        connect(ui->toolButton_Home, &QToolButton::clicked, this, &NavigationBar::onHomeClicked);
        connect(ui->toolButton_Back, &QToolButton::clicked, this, &NavigationBar::onBackClicked);
        connect(ui->toolButton_Forward, &QToolButton::clicked, this, &NavigationBar::onForwardClicked);
        connect(ui->toolButton_Reload, &QToolButton::clicked, this, &NavigationBar::onReloadClicked);
        connect(ui->toolButton_Settings, &QToolButton::clicked, this, &NavigationBar::onSettingsClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &NavigationBar::onCloseClicked);
    }

    void NavigationBar::onHomeClicked() {
        emit home();
    }
    void NavigationBar::onBackClicked() {
        emit back();
    }
    void NavigationBar::onForwardClicked() {
        emit forward();
    }
    void NavigationBar::onReloadClicked() {
        emit reload();
    }
    void NavigationBar::onSettingsClicked() {
        emit settings();
    }

    void NavigationBar::onCloseClicked() {
        emit close();
    }

    NavigationBar::~NavigationBar() {
        delete ui;
    }
} // fairwindsk::ui::web
