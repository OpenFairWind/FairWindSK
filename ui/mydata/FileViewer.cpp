//
// Created by Raffaele Montella on 06/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_JsonViewer.h" resolved

#include "FileViewer.hpp"

#include "FileInfoListModel.hpp"
#include "ui_FileViewer.h"

namespace fairwindsk::ui::mydata {

    FileViewer::FileViewer(const QString& path, QWidget *parent): QWidget(parent), ui(new Ui::FileViewer) {
        ui->setupUi(this);

        connect(ui->toolButton_Close, &QToolButton::clicked, this, &FileViewer::onCloseClicked);

        ui->label_filePath->setText(path);
        ui->webEngineView->load(QUrl("file://"+path));

    }

    void FileViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    FileViewer::~FileViewer() {
        delete ui;
    }
}