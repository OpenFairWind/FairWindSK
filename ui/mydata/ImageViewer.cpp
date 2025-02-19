//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ImageViewer.h" resolved

#include "ImageViewer.hpp"

#include <QMessageBox>

#include "DirItemInfo.hpp"
#include "ui_ImageViewer.h"

#include <QImageReader>
#include <QColorSpace>

namespace fairwindsk::ui::mydata {
    ImageViewer::ImageViewer(const QString& path, QWidget *parent) :
        QWidget(parent), ui(new Ui::ImageViewer) {
        ui->setupUi(this);

        m_imageViewerWidget = new ImageViewerWidget(this);

        ui->verticalLayout_Image->addWidget(m_imageViewerWidget);

        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &ImageViewer::onZoomInClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &ImageViewer::onZoomOutClicked);
        connect(ui->toolButton_One2One, &QToolButton::clicked, this, &ImageViewer::onOne2OneClicked);
        connect(ui->toolButton_Adapt, &QToolButton::clicked, this, &ImageViewer::onAdaptClicked);

        loadFile(path);
    }

    void ImageViewer::onZoomOutClicked() {

        m_imageViewerWidget->zoomOut();
    }

    void ImageViewer::onZoomInClicked() {

        m_imageViewerWidget->zoomIn();
    }

    void ImageViewer::onAdaptClicked() {
        const bool fitToWindow = ui->toolButton_Adapt->isChecked();
        ui->scrollArea->setWidgetResizable(fitToWindow);
        if (!fitToWindow)
            onOne2OneClicked();
    }

    void ImageViewer::onOne2OneClicked() {
        /*
        ui->label_Image->adjustSize();
        m_scaleFactor = 1.0;
        */

    }

    bool ImageViewer::loadFile(const QString &path) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (const QImage newImage = reader.read(); !newImage.isNull())
        {
            setImage(newImage);

            return true;
        }
        return false;
    }

    void ImageViewer::setImage(const QImage &newImage) {
        m_imageViewerWidget->setPixmap(QPixmap::fromImage(newImage),newImage.width(),newImage.height());
    }





    ImageViewer::~ImageViewer() {
        delete m_imageViewerWidget;
        delete ui;
    }
} // fairwindsk::ui::mydata
