//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>


#include "ImageViewerWidget.hpp"


namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class ImageViewer; }
QT_END_NAMESPACE

class ImageViewer : public QWidget {
Q_OBJECT

public:
    explicit ImageViewer(const QString& path, QWidget *parent = nullptr);
    ~ImageViewer() override;

    bool loadFile(const QString &path);
    void setImage(const QImage &newImage);

    public slots:
        void onZoomInClicked();
        void onZoomOutClicked();
        void onOne2OneClicked();
        void onAdaptClicked();

private:
    Ui::ImageViewer *ui;

    ImageViewerWidget *m_imageViewerWidget;
    double m_scaleFactor = 1;
};
} // fairwindsk::ui::mydata

#endif //IMAGEVIEWER_H
