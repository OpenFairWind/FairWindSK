//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_NavigationBar.h" resolved

#include "NavigationBar.hpp"
#include "ui_NavigationBar.h"

#include <QToolButton>
#include <QtGlobal>

namespace fairwindsk::ui::web {
    NavigationBar::NavigationBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::NavigationBar) {
        ui->setupUi(this);

        connect(ui->toolButton_Home, &QToolButton::clicked, this, &NavigationBar::onHomeClicked);
        connect(ui->toolButton_Back, &QToolButton::clicked, this, &NavigationBar::onBackClicked);
        connect(ui->toolButton_Forward, &QToolButton::clicked, this, &NavigationBar::onForwardClicked);
        connect(ui->toolButton_Reload, &QToolButton::clicked, this, &NavigationBar::onReloadClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &NavigationBar::onZoomOutClicked);
        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &NavigationBar::onZoomInClicked);
        connect(ui->toolButton_Settings, &QToolButton::clicked, this, &NavigationBar::onSettingsClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &NavigationBar::onCloseClicked);

        setBackEnabled(false);
        setForwardEnabled(false);
        setReloadActive(false);
        setZoomPercent(100.0);
    }

    void NavigationBar::setBackEnabled(const bool enabled) const {
        ui->toolButton_Back->setEnabled(enabled);
    }

    void NavigationBar::setForwardEnabled(const bool enabled) const {
        ui->toolButton_Forward->setEnabled(enabled);
    }

    void NavigationBar::setHomeEnabled(const bool enabled) const {
        ui->toolButton_Home->setEnabled(enabled);
    }

    void NavigationBar::setSettingsEnabled(const bool enabled) const {
        ui->toolButton_Settings->setEnabled(enabled);
    }

    void NavigationBar::setReloadActive(const bool loading) const {
        ui->toolButton_Reload->setText(loading ? tr("Stop") : tr("Reload"));
        ui->toolButton_Reload->setToolTip(loading ? tr("Stop loading") : tr("Reload page"));
    }

    void NavigationBar::setZoomPercent(const double zoomPercent) const {
        ui->label_ZoomPercent->setText(tr("%1%").arg(qRound(zoomPercent)));
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
    void NavigationBar::onZoomOutClicked() {
        emit zoomOut();
    }
    void NavigationBar::onZoomInClicked() {
        emit zoomIn();
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
