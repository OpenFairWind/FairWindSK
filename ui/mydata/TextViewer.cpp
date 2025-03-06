//
// Created by Raffaele Montella on 03/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_TextViewer.h" resolved

#include "TextViewer.hpp"

#include "FileInfoListModel.hpp"
#include "ui_TextViewer.h"

namespace fairwindsk::ui::mydata {
    TextViewer::TextViewer(const QString& path, QWidget *parent) : QWidget(parent), ui(new Ui::TextViewer) {
        ui->setupUi(this);



        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &TextViewer::onZoomInClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &TextViewer::onZoomOutClicked);
        connect(ui->toolButton_One2One, &QToolButton::clicked, this, &TextViewer::onOne2OneClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &TextViewer::onCloseClicked);


        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->textEdit_Text->setText(file.readAll());
        } else {
            ui->textEdit_Text->setText(":-(");
        }




    }

    void TextViewer::onZoomOutClicked() {


    }

    void TextViewer::onZoomInClicked() {


    }

    void TextViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    void TextViewer::onOne2OneClicked() {

    }



    TextViewer::~TextViewer() {
        delete ui;
    }
} // fairwindsk::ui::mydata
