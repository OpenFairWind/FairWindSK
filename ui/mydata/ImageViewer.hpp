//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>


#include "ImageWidget.hpp"


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
    void setImage(const QImage &newImage) ;

    signals:
        void askedToBeClosed();

    public slots:
        void onZoomInClicked();
        void onZoomOutClicked();
        void onOne2OneClicked() ;
        void onCloseClicked();

private:
    Ui::ImageViewer *ui;

    ImageWidget *m_imageWidget = nullptr;
    double m_scaleFactor = 1;
};
} // fairwindsk::ui::mydata

#endif //IMAGEVIEWER_H
