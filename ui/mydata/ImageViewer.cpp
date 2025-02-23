//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ImageViewer.h" resolved

#include "ImageViewer.hpp"

#include <QMessageBox>


#include "ui_ImageViewer.h"

#include <QImageReader>
#include <QColorSpace>
#include <QTimer>

namespace fairwindsk::ui::mydata {
    ImageViewer::ImageViewer(const QString& path, QWidget *parent) :
        QWidget(parent), ui(new Ui::ImageViewer) {
        ui->setupUi(this);

        m_imageWidget = new ImageWidget(this);
        m_imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


        ui->verticalLayout_Image->addWidget(m_imageWidget);

        connect(ui->toolButton_ZoomIn, &QToolButton::clicked, this, &ImageViewer::onZoomInClicked);
        connect(ui->toolButton_ZoomOut, &QToolButton::clicked, this, &ImageViewer::onZoomOutClicked);
        connect(ui->toolButton_One2One, &QToolButton::clicked, this, &ImageViewer::onOne2OneClicked);
        connect(ui->toolButton_Close, &QToolButton::clicked, this, &ImageViewer::onCloseClicked);

        loadFile(path);
    }

    void ImageViewer::onZoomOutClicked() {

        m_imageWidget->zoomOut();
    }

    void ImageViewer::onZoomInClicked() {

        m_imageWidget->zoomIn();
    }

    void ImageViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    void ImageViewer::onOne2OneClicked() {

        m_imageWidget->resize(ui->scrollAreaWidgetContents->width(), ui->scrollAreaWidgetContents->height());


    }

    bool ImageViewer::loadFile(const QString &path) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (const QImage newImage = reader.read(); !newImage.isNull()) {
            setImage(newImage);

            return true;
        }
        return false;
    }

    void ImageViewer::setImage(const QImage &newImage)  {

        m_imageWidget->setPixmap(QPixmap::fromImage(newImage),ui->scrollAreaWidgetContents->width(),ui->scrollAreaWidgetContents->height());

        // Show the window fullscreen
        QTimer::singleShot(0, this, SLOT(onOne2OneClicked()));

    }





    ImageViewer::~ImageViewer() {

        // Check if the image widget has been allocated
        if (m_imageWidget) {

            // Delete the widget
            delete m_imageWidget;

            // Set the pointer to null
            m_imageWidget = nullptr;
        }

        // Check if the UI has been allocated
        if (ui) {
            // Delete the ui
            delete ui;

            // Set the pointer to null
            ui = nullptr;
        }

    }
} // fairwindsk::ui::mydata
