//
// Created by Raffaele Montella on 20/02/25.
//

#ifndef IMAGEVIEWVERWIDGET_H
#define IMAGEVIEWVERWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QPinchGesture>
#include <QMenu>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#include <iostream>
#include <optional>

// Original source code https://github.com/p-ranav/ImageViewer-Qt6

namespace fairwindsk::ui::mydata {

    class ImageViewerWidget : public QGraphicsView {
        Q_OBJECT

        QGraphicsScene m_scene;
        QGraphicsPixmapItem m_item;

        static constexpr inline qreal ZOOM_IN_SCALE = 1.04;
        static constexpr inline qreal ZOOM_OUT_SCALE = 0.96;

    public:
        explicit ImageViewerWidget(QWidget *parent = nullptr);
        void setPixmap(const QPixmap &pixmap, int desiredWidth, int desiredHeight);
        QPixmap pixmap() const;
        void scale(qreal s);
        void resize(int desiredWidth, int desiredHeight);
        void zoomIn();
        void zoomOut();

    protected:
        bool event(QEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        bool nativeGestureEvent(QNativeGestureEvent *event);
        void keyPressEvent(QKeyEvent *event) override;
    };

} // fairwindsk::ui::mydata

#endif //IMAGEVIEWVERWIDGET_H
