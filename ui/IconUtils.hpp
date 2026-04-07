//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_ICONUTILS_HPP
#define FAIRWINDSK_UI_ICONUTILS_HPP

#include <algorithm>
#include <cmath>
#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QVector>

#include "Configuration.hpp"

namespace fairwindsk::ui {
    inline double relativeLuminance(const QColor &color) {
        const auto channel = [](double value) {
            value /= 255.0;
            return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };

        return (0.2126 * channel(color.red()))
            + (0.7152 * channel(color.green()))
            + (0.0722 * channel(color.blue()));
    }

    inline QColor bestContrastingColor(const QColor &background, const std::initializer_list<QColor> &candidates) {
        QColor best = background;
        double bestContrast = -1.0;

        const double backgroundLuminance = relativeLuminance(background);
        for (const QColor &candidate : candidates) {
            const double candidateLuminance = relativeLuminance(candidate);
            const double contrast =
                (std::max(backgroundLuminance, candidateLuminance) + 0.05)
                / (std::min(backgroundLuminance, candidateLuminance) + 0.05);
            if (contrast > bestContrast) {
                bestContrast = contrast;
                best = candidate;
            }
        }

        return best;
    }

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

    inline QColor comfortIconColor(const fairwindsk::Configuration *configuration,
                                   const QString &preset,
                                   const QColor &fallback) {
        if (!configuration) {
            return fallback;
        }

        const QColor configured = configuration->getComfortThemeColor(
            preset.trimmed().toLower(),
            QStringLiteral("iconDefault"),
            QColor());
        return configured.isValid() ? configured : fallback;
    }
}

#endif // FAIRWINDSK_UI_ICONUTILS_HPP
