//
// Created by Codex on 28/03/26.
//

#include "AppDetailsWidget.hpp"

#include "ui_AppDetailsWidget.h"

namespace fairwindsk::ui::settings {
    AppDetailsWidget::AppDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::AppDetailsWidget) {
        ui->setupUi(this);
    }

    AppDetailsWidget::~AppDetailsWidget() {
        delete ui;
        ui = nullptr;
    }
}
