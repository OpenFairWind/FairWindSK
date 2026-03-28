//
// Created by Codex on 28/03/26.
//

#include "PageDetailsWidget.hpp"

#include "ui_PageDetailsWidget.h"

namespace fairwindsk::ui::settings {
    PageDetailsWidget::PageDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PageDetailsWidget) {
        ui->setupUi(this);
    }

    PageDetailsWidget::~PageDetailsWidget() {
        delete ui;
        ui = nullptr;
    }
}
