//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_ICONUTILS_HPP
#define FAIRWINDSK_UI_ICONUTILS_HPP

#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace fairwindsk::ui {
    inline QPixmap tintedPixmap(const QPixmap &source, const QColor &color) {
        if (source.isNull()) {
            return {};
        }

        QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(image.rect(), color);
        painter.end();
        return QPixmap::fromImage(image);
    }

    inline QIcon tintedIcon(const QIcon &source, const QColor &color, const QSize &size) {
        if (source.isNull()) {
            return {};
        }

        const QSize iconSize = size.isValid() ? size : QSize(24, 24);
        QIcon icon;
        for (const auto mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled}) {
            const QColor modeColor = mode == QIcon::Disabled ? color.darker(150) : color;
            const QPixmap pixmap = source.pixmap(iconSize, mode, QIcon::Off);
            if (!pixmap.isNull()) {
                icon.addPixmap(tintedPixmap(pixmap, modeColor), mode, QIcon::Off);
            }
        }
        return icon;
    }

    inline void applyTintedButtonIcon(QAbstractButton *button, const QColor &color, const QSize &fallbackSize = QSize(24, 24)) {
        if (!button || button->icon().isNull()) {
            return;
        }

        const QSize iconSize = button->iconSize().isValid() ? button->iconSize() : fallbackSize;
        button->setIcon(tintedIcon(button->icon(), color, iconSize));
    }
}

#endif // FAIRWINDSK_UI_ICONUTILS_HPP
