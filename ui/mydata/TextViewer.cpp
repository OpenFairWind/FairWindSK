//
// Created by Raffaele Montella on 03/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_TextViewer.h" resolved

#include <QWheelEvent>

#include "TextViewer.hpp"

#include <QDomDocument>
#include <QJsonDocument>
#include <QtCore/qmimedatabase.h>

#include "FileInfoListModel.hpp"
#include "ui_TextViewer.h"

namespace fairwindsk::ui::mydata {
    TextViewer::TextViewer(const QString& path, QWidget *parent) : QWidget(parent), ui(new Ui::TextViewer) {
        ui->setupUi(this);

        ui->textBrowser_Text->setFontPointSize(13);

        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &TextViewer::onZoomInClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &TextViewer::onZoomOutClicked);
        connect(ui->toolButton_One2One, &QToolButton::clicked, this, &TextViewer::onOne2OneClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &TextViewer::onCloseClicked);

        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_text = file.readAll();

            const QMimeDatabase db;
            const auto type = db.mimeTypeForFile(path);
            qDebug() << "Mime type:" << type.name();

            if (type.name() == "application/xml") {
                QDomDocument doc;
                doc.setContent(m_text);
                m_text = doc.toString();
            }


            ui->textBrowser_Text->setPlainText(m_text);

        } else {
            ui->textBrowser_Text->setPlainText(":-(");
        }

        m_fontPS = ui->textBrowser_Text->fontPointSize();
        qDebug() << m_fontPS;
    }



    void TextViewer::onZoomOutClicked() {
        if (const qreal fontPS = ui->textBrowser_Text->fontPointSize(); fontPS > 8) {
            ui->textBrowser_Text->setFontPointSize(fontPS - 2);
            ui->textBrowser_Text->setPlainText(m_text);
        }
    }

    void TextViewer::onZoomInClicked() {

        const qreal fontPS = ui->textBrowser_Text->fontPointSize();

        ui->textBrowser_Text->setFontPointSize(fontPS + 2);
        ui->textBrowser_Text->setPlainText(m_text);
    }

    void TextViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    void TextViewer::onOne2OneClicked() {
        ui->textBrowser_Text->setFontPointSize(m_fontPS);
        ui->textBrowser_Text->setPlainText(m_text);
    }



    TextViewer::~TextViewer() {
        if (ui) {
            delete ui;
            ui = nullptr;
        }

    }
} // fairwindsk::ui::mydata
