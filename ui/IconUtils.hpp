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
#include <QPalette>
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

    inline QColor comfortThemeColor(const fairwindsk::Configuration *configuration,
                                    const QString &preset,
                                    const QString &key,
                                    const QColor &fallback) {
        if (!configuration) {
            return fallback;
        }

        const QColor configured = configuration->getComfortThemeColor(
            preset.trimmed().toLower(),
            key,
            QColor());
        return configured.isValid() ? configured : fallback;
    }

    inline QColor comfortAlpha(const QColor &color, const int alpha) {
        QColor result = color;
        result.setAlpha(std::clamp(alpha, 0, 255));
        return result;
    }

    struct ComfortChromeColors {
        QColor window;
        QColor text;
        QColor buttonBackground;
        QColor buttonText;
        QColor border;
        QColor accentTop;
        QColor accentBottom;
        QColor accentText;
        QColor disabledText;
        QColor hoverBackground;
        QColor pressedBackground;
        QColor transparentHoverBackground;
        QColor icon;
        QColor transparentIcon;
    };

    inline ComfortChromeColors resolveComfortChromeColors(const fairwindsk::Configuration *configuration,
                                                          const QString &preset,
                                                          const QPalette &palette,
                                                          const bool accentButtons = false) {
        ComfortChromeColors colors;
        colors.window = comfortThemeColor(configuration, preset, QStringLiteral("window"), palette.color(QPalette::Window));
        colors.text = comfortThemeColor(configuration, preset, QStringLiteral("text"), palette.color(QPalette::WindowText));
        colors.buttonBackground = comfortThemeColor(
            configuration,
            preset,
            accentButtons ? QStringLiteral("accentTop") : QStringLiteral("buttonBackground"),
            palette.color(accentButtons ? QPalette::Highlight : QPalette::Button));
        colors.buttonText = comfortThemeColor(
            configuration,
            preset,
            accentButtons ? QStringLiteral("accentText") : QStringLiteral("buttonText"),
            palette.color(accentButtons ? QPalette::HighlightedText : QPalette::ButtonText));
        colors.border = comfortThemeColor(configuration, preset, QStringLiteral("border"), palette.color(QPalette::Mid));
        colors.accentTop = comfortThemeColor(configuration, preset, QStringLiteral("accentTop"), palette.color(QPalette::Highlight));
        colors.accentBottom = comfortThemeColor(configuration, preset, QStringLiteral("accentBottom"), colors.accentTop.darker(122));
        colors.accentText = comfortThemeColor(configuration, preset, QStringLiteral("accentText"), palette.color(QPalette::HighlightedText));
        colors.disabledText = comfortThemeColor(
            configuration,
            preset,
            QStringLiteral("text"),
            palette.color(QPalette::Disabled, QPalette::WindowText)).darker(150);
        colors.hoverBackground = accentButtons ? colors.accentTop.lighter(108) : colors.buttonBackground.lighter(108);
        colors.pressedBackground = accentButtons ? colors.accentBottom : colors.buttonBackground.darker(118);
        colors.transparentHoverBackground = comfortAlpha(colors.accentTop, 56);
        colors.icon = comfortIconColor(
            configuration,
            preset,
            bestContrastingColor(
                colors.buttonBackground,
                {colors.buttonText, colors.text, colors.accentText, QColor(QStringLiteral("#f8f8f8")), QColor(QStringLiteral("#111111"))}));
        colors.transparentIcon = comfortIconColor(
            configuration,
            preset,
            bestContrastingColor(
                colors.window,
                {colors.text, colors.buttonText, colors.accentText, QColor(QStringLiteral("#f8f8f8")), QColor(QStringLiteral("#111111"))}));
        return colors;
    }
}

#endif // FAIRWINDSK_UI_ICONUTILS_HPP
