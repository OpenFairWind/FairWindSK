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
    ImageViewer::ImageViewer(QString path, QWidget *parent) :
        QWidget(parent), ui(new Ui::ImageViewer) {
        ui->setupUi(this);


        loadFile(path);
    }

    bool ImageViewer::loadFile(const QString &fileName)
    {
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        if (const QImage newImage = reader.read(); !newImage.isNull())
        {
            setImage(newImage);

            /*
            setWindowFilePath(fileName);

            const QString description = m_image.colorSpace().isValid()
                ? m_image.colorSpace().description() : tr("unknown");
            const QString message = tr("Opened \"%1\", %2x%3, Depth: %4 (%5)")
                .arg(QDir::toNativeSeparators(fileName)).arg(m_image.width()).arg(m_image.height())
                .arg(m_image.depth()).arg(description);
            */
            return true;
        }
        return false;
    }

    void ImageViewer::setImage(const QImage &newImage)
    {
        m_image = newImage;
        if (m_image.colorSpace().isValid())
            m_image.convertToColorSpace(QColorSpace::SRgb);
        ui->label_Image->setPixmap(QPixmap::fromImage(m_image));

        m_scaleFactor = 1.0;



        //fitToWindowAct->setEnabled(true);

        //if (!fitToWindowAct->isChecked())
        //    m_imageLabel->adjustSize();
    }


    void ImageViewer::scaleImage(const double factor) {
        m_scaleFactor *= factor;
        ui->label_Image->resize(m_scaleFactor * ui->label_Image->pixmap(Qt::ReturnByValue).size());

        adjustScrollBar(ui->scrollArea->horizontalScrollBar(), factor);
        adjustScrollBar(ui->scrollArea->verticalScrollBar(), factor);

        //zoomInAct->setEnabled(m_scaleFactor < 3.0);
        //zoomOutAct->setEnabled(m_scaleFactor > 0.333);
    }

    void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, const double factor) {
        scrollBar->setValue(static_cast<int>(factor * scrollBar->value()
            + ((factor - 1) * scrollBar->pageStep() / 2)));
    }


    ImageViewer::~ImageViewer() {
        delete ui;
    }
} // fairwindsk::ui::mydata
