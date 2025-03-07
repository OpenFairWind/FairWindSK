//
// Created by Raffaele Montella on 06/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_JsonViewer.h" resolved

#include "JsonViewer.hpp"

#include "FileInfoListModel.hpp"
#include "JsonItemModel.hpp"
#include "ui_JsonViewer.h"

namespace fairwindsk::ui::mydata {

    JsonViewer::JsonViewer(const QString& path, QWidget *parent): QWidget(parent), ui(new Ui::JsonViewer) {
        ui->setupUi(this);

        connect(ui->toolButton_Close, &QToolButton::clicked, this, &JsonViewer::onCloseClicked);

        QJsonParseError err;

        QFile file(path);
        file.open(QIODevice::ReadOnly);
        const auto root = QJsonDocument::fromJson(file.readAll(), &err);

        if (err.error == QJsonParseError::NoError) {
            file.close();

            auto *model = new JsonItemModel(root, this);
            ui->treeView_Text->setModel(model);
        }
    }

    void JsonViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    JsonViewer::~JsonViewer() {
        delete ui;
    }
}