//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_ICONUTILS_HPP
#define FAIRWINDSK_UI_ICONUTILS_HPP

#include <algorithm>
#include <cmath>
#include <QAbstractButton>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QToolButton>
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
            for (const auto state : {QIcon::Off, QIcon::On}) {
                const QPixmap pixmap = source.pixmap(iconSize, mode, state);
                if (!pixmap.isNull()) {
                    icon.addPixmap(tintedPixmap(pixmap, modeColor), mode, state);
                }
            }
        }
        return icon;
    }

    inline void applyTintedButtonIcon(QAbstractButton *button, const QColor &color, const QSize &fallbackSize = QSize(24, 24)) {
        if (!button || button->icon().isNull()) {
            return;
        }

        const QSize iconSize = button->iconSize().isValid() ? button->iconSize() : fallbackSize;
        const QVariant baseIconVariant = button->property("_fw_base_icon");
        const QIcon baseIcon = baseIconVariant.canConvert<QIcon>() ? qvariant_cast<QIcon>(baseIconVariant) : button->icon();
        if (!baseIconVariant.isValid()) {
            button->setProperty("_fw_base_icon", baseIcon);
        }

        const QString tintKey = QStringLiteral("%1|%2x%3")
            .arg(color.rgba(), 0, 16)
            .arg(iconSize.width())
            .arg(iconSize.height());
        if (button->property("_fw_icon_tint_key").toString() == tintKey) {
            return;
        }

        button->setIcon(tintedIcon(baseIcon, color, iconSize));
        button->setProperty("_fw_icon_tint_key", tintKey);
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

    struct ComfortStatusColors {
        QColor healthyFill;
        QColor healthyText;
        QColor warningFill;
        QColor warningText;
        QColor errorFill;
        QColor errorText;
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
                {colors.buttonText, colors.text, colors.accentText, palette.color(QPalette::Base), palette.color(QPalette::ButtonText)}));
        colors.transparentIcon = comfortIconColor(
            configuration,
            preset,
            bestContrastingColor(
                colors.window,
                {colors.text, colors.buttonText, colors.accentText, palette.color(QPalette::Base), palette.color(QPalette::WindowText)}));
        return colors;
    }

    inline ComfortStatusColors resolveComfortStatusColors(const fairwindsk::Configuration *configuration,
                                                          const QString &preset,
                                                          const QPalette &palette) {
        const auto chrome = resolveComfortChromeColors(configuration, preset, palette, false);

        ComfortStatusColors colors;
        colors.healthyFill = chrome.accentTop;
        colors.healthyText = bestContrastingColor(
            colors.healthyFill,
            {chrome.accentText, chrome.text, chrome.buttonText, palette.color(QPalette::HighlightedText), palette.color(QPalette::WindowText)});
        colors.warningFill = chrome.buttonBackground;
        colors.warningText = bestContrastingColor(
            colors.warningFill,
            {chrome.buttonText, chrome.text, chrome.accentText, palette.color(QPalette::ButtonText), palette.color(QPalette::WindowText)});
        colors.errorFill = chrome.accentBottom;
        colors.errorText = bestContrastingColor(
            colors.errorFill,
            {chrome.accentText, chrome.text, chrome.buttonText, palette.color(QPalette::HighlightedText), palette.color(QPalette::WindowText)});
        return colors;
    }

    enum class BottomBarButtonChrome {
        Transparent,
        Flat,
        Accent
    };

    inline QString bottomBarButtonStyleSheet(const ComfortChromeColors &colors,
                                             const BottomBarButtonChrome chrome) {
        switch (chrome) {
            case BottomBarButtonChrome::Transparent:
                return QStringLiteral(
                    "QToolButton {"
                    " background: transparent;"
                    " border: none;"
                    " border-radius: 10px;"
                    " padding: 4px 6px;"
                    " color: %1;"
                    " }"
                    "QToolButton:hover {"
                    " background: %2;"
                    " color: %1;"
                    " }"
                    "QToolButton:pressed, QToolButton:checked {"
                    " background: %3;"
                    " color: %4;"
                    " }"
                    "QToolButton:disabled {"
                    " color: %5;"
                    " }")
                    .arg(colors.transparentIcon.name(),
                         colors.transparentHoverBackground.name(QColor::HexArgb),
                         comfortAlpha(colors.accentBottom, 92).name(QColor::HexArgb),
                         colors.accentText.name(),
                         colors.disabledText.name());
            case BottomBarButtonChrome::Accent:
                return QStringLiteral(
                    "QToolButton {"
                    " background: %1;"
                    " border: none;"
                    " border-radius: 10px;"
                    " padding: 6px 8px;"
                    " color: %2;"
                    " }"
                    "QToolButton:hover {"
                    " background: %3;"
                    " color: %2;"
                    " }"
                    "QToolButton:pressed, QToolButton:checked {"
                    " background: %4;"
                    " color: %5;"
                    " }"
                    "QToolButton:disabled {"
                    " color: %6;"
                    " }")
                    .arg(comfortAlpha(colors.accentTop, 68).name(QColor::HexArgb),
                         colors.icon.name(),
                         comfortAlpha(colors.accentTop, 102).name(QColor::HexArgb),
                         colors.accentBottom.name(),
                         colors.accentText.name(),
                         colors.disabledText.name());
            case BottomBarButtonChrome::Flat:
            default:
                return QStringLiteral(
                    "QToolButton {"
                    " background: transparent;"
                    " border: none;"
                    " border-radius: 10px;"
                    " padding: 6px 8px;"
                    " color: %1;"
                    " }"
                    "QToolButton:hover {"
                    " background: %2;"
                    " color: %1;"
                    " }"
                    "QToolButton:pressed, QToolButton:checked {"
                    " background: %3;"
                    " color: %4;"
                    " }"
                    "QToolButton:disabled {"
                    " color: %5;"
                    " }")
                    .arg(colors.icon.name(),
                         colors.transparentHoverBackground.name(QColor::HexArgb),
                         comfortAlpha(colors.accentBottom, 92).name(QColor::HexArgb),
                         colors.accentText.name(),
                         colors.disabledText.name());
        }
    }

    inline void applyBottomBarToolButtonChrome(QToolButton *button,
                                               const ComfortChromeColors &colors,
                                               const BottomBarButtonChrome chrome,
                                               const QSize &iconSize = QSize(44, 44),
                                               const int minHeight = 90) {
        if (!button) {
            return;
        }

        if (!button->autoRaise()) {
            button->setAutoRaise(true);
        }
        if (button->toolButtonStyle() != Qt::ToolButtonTextUnderIcon) {
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }
        if (button->minimumHeight() != minHeight) {
            button->setMinimumHeight(minHeight);
        }
        if (button->iconSize() != iconSize) {
            button->setIconSize(iconSize);
        }

        const QString styleSheet = bottomBarButtonStyleSheet(colors, chrome);
        if (button->styleSheet() != styleSheet) {
            button->setStyleSheet(styleSheet);
        }

        const QColor iconColor = chrome == BottomBarButtonChrome::Transparent
            ? colors.transparentIcon
            : colors.icon;
        applyTintedButtonIcon(button, iconColor, iconSize);
    }

    inline QString bottomBarPushButtonStyleSheet(const ComfortChromeColors &colors,
                                                 const bool accent = false) {
        const QColor background = accent ? colors.accentTop : colors.buttonBackground;
        const QColor hover = accent ? colors.accentTop.lighter(108) : colors.hoverBackground;
        const QColor pressed = accent ? colors.accentBottom : colors.pressedBackground;
        const QColor text = accent ? colors.accentText : colors.buttonText;
        return QStringLiteral(
            "QPushButton {"
            " background: %1;"
            " border: 1px solid %2;"
            " border-radius: 10px;"
            " padding: 10px 18px;"
            " color: %3;"
            " }"
            "QPushButton:hover {"
            " background: %4;"
            " }"
            "QPushButton:pressed, QPushButton:checked {"
            " background: %5;"
            " color: %6;"
            " border-color: %5;"
            " }"
            "QPushButton:disabled {"
            " color: %7;"
            " border-color: %8;"
            " }")
            .arg(background.name(),
                 colors.border.name(),
                 text.name(),
                 hover.name(),
                 pressed.name(),
                 colors.accentText.name(),
                 colors.disabledText.name(),
                 colors.border.darker(120).name());
    }

    inline void applyBottomBarPushButtonChrome(QPushButton *button,
                                               const ComfortChromeColors &colors,
                                               const bool accent = false,
                                               const int minHeight = 54) {
        if (!button) {
            return;
        }

        if (button->minimumHeight() != minHeight) {
            button->setMinimumHeight(minHeight);
        }

        const QString styleSheet = bottomBarPushButtonStyleSheet(colors, accent);
        if (button->styleSheet() != styleSheet) {
            button->setStyleSheet(styleSheet);
        }
    }

    inline void applySectionTitleLabelStyle(QLabel *label,
                                            const fairwindsk::Configuration *configuration,
                                            const QString &preset,
                                            const QPalette &palette,
                                            const qreal pointSize = 20.0) {
        if (!label) {
            return;
        }

        QFont font = label->font();
        font.setBold(true);
        font.setPointSizeF(pointSize);
        label->setFont(font);

        QPalette labelPalette = label->palette();
        const QColor titleColor = comfortThemeColor(
            configuration,
            preset,
            QStringLiteral("text"),
            bestContrastingColor(
                palette.color(QPalette::Window),
                {palette.color(QPalette::WindowText),
                 palette.color(QPalette::Text),
                 palette.color(QPalette::ButtonText)}));
        labelPalette.setColor(QPalette::WindowText, titleColor);
        labelPalette.setColor(QPalette::Text, titleColor);
        label->setPalette(labelPalette);
    }
}

#endif // FAIRWINDSK_UI_ICONUTILS_HPP
