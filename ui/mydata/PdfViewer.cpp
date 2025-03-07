//
// Created by Raffaele Montella on 06/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_PdfViewer.h" resolved

#include "PdfViewer.hpp"
#include "ui_PdfViewer.h"

#include <QScroller>
#include <QUrl>

#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfView>


#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
#include <QPdfPageSelector>
#endif

namespace fairwindsk::ui::mydata {
    PdfViewer::PdfViewer(const QString& path, QWidget *parent) : QWidget(parent), ui(new Ui::PdfViewer) {
        ui->setupUi(this);

        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &PdfViewer::onZoomInClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &PdfViewer::onZoomOutClicked);
        connect(ui->toolButton_One2One, &QToolButton::clicked, this, &PdfViewer::onOne2OneClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &PdfViewer::onCloseClicked);


        m_document = new QPdfDocument(this);

        ui->pdfView_Text->setDocument(m_document);
        ui->pdfView_Text->setPageMode(QPdfView::PageMode::MultiPage);

        m_document->load(QUrl("file://"+path).toLocalFile());
        ui->listView_Pages->setModel(m_document->pageModel());
        ui->listView_Pages->setMaximumWidth(150);


        QScroller::grabGesture(ui->pdfView_Text->viewport(), QScroller::ScrollerGestureType::LeftMouseButtonGesture);
        //HoverWatcher::watcher(m_pdfView->viewport());


    }

    void PdfViewer::onZoomOutClicked() {
        ui->pdfView_Text->setZoomFactor(ui->pdfView_Text->zoomFactor() / m_zoomMultiplier);
    }

    void PdfViewer::onZoomInClicked() {
        ui->pdfView_Text->setZoomFactor(ui->pdfView_Text->zoomFactor() * m_zoomMultiplier);

    }

    void PdfViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    void PdfViewer::onOne2OneClicked() {
        ui->pdfView_Text->setZoomFactor(1);
    }

PdfViewer::~PdfViewer() {
    delete ui;
}
} // fairwindsk::ui::mydata
