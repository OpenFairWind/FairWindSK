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
        connect(ui->toolButton_Settings, &QToolButton::clicked, this, &NavigationBar::onSettingsCicked);

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
    void NavigationBar::onSettingsCicked() {
        emit settings();
    }

    NavigationBar::~NavigationBar() {
        delete ui;
    }
} // fairwindsk::ui::web
