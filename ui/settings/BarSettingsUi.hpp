#ifndef FAIRWINDSK_UI_SETTINGS_BARSETTINGSUI_HPP
#define FAIRWINDSK_UI_SETTINGS_BARSETTINGSUI_HPP

#include <algorithm>

#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QSizePolicy>
#include <QToolButton>

#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::settings::barsettings {
    inline QString listWidgetChrome(const fairwindsk::ui::ComfortChromeColors &colors,
                                    const bool dashedItems = false) {
        return QStringLiteral(
            "QListWidget {"
            " border: 1px solid %1;"
            " border-radius: 12px;"
            " padding: 8px;"
            " background: %2;"
            " }"
            "QListWidget::item {"
            " margin: 4px;"
            " padding: 10px 12px;"
            " border: 1px %3 %4;"
            " border-radius: 10px;"
            " background: %5;"
            " color: %6;"
            " }"
            "QListWidget::item:selected {"
            " border-color: %7;"
            " background: %8;"
            " color: %9;"
            " font-weight: 600;"
            " }")
            .arg(colors.border.name(),
                 colors.window.name(),
                 dashedItems ? QStringLiteral("dashed") : QStringLiteral("solid"),
                 colors.border.name(),
                 fairwindsk::ui::comfortAlpha(colors.buttonBackground, dashedItems ? 22 : 32).name(QColor::HexArgb),
                 colors.text.name(),
                 colors.accentTop.name(),
                 fairwindsk::ui::comfortAlpha(colors.accentTop, 46).name(QColor::HexArgb),
                 colors.text.name());
    }

    inline QString previewFrameChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
        return QStringLiteral(
            "QFrame {"
            " border: 1px solid %1;"
            " border-radius: 14px;"
            " background: %2;"
            " }")
            .arg(colors.border.name(),
                 fairwindsk::ui::comfortAlpha(colors.buttonBackground, 18).name(QColor::HexArgb));
    }

    inline QString paletteChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
        return QStringLiteral(
            "QScrollArea { border: 1px solid %1; border-radius: 12px; background: %2; }"
            "QWidget { background: transparent; }")
            .arg(colors.border.name(),
                 colors.window.name());
    }

    inline void applySecondaryLabelStyle(QLabel *label,
                                         const fairwindsk::ui::ComfortChromeColors &colors) {
        if (!label) {
            return;
        }

        QFont font = label->font();
        font.setBold(true);
        font.setPointSizeF(std::max(font.pointSizeF(), 13.0));
        label->setFont(font);

        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, colors.text);
        palette.setColor(QPalette::Text, colors.text);
        label->setPalette(palette);
    }

    inline void applyHintLabelStyle(QLabel *label,
                                    const fairwindsk::ui::ComfortChromeColors &colors) {
        if (!label) {
            return;
        }

        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, colors.text);
        palette.setColor(QPalette::Text, colors.text);
        label->setPalette(palette);
    }

    inline void configureSizeButton(QToolButton *button,
                                    const QString &text,
                                    const QString &toolTip,
                                    const QString &accessibleName,
                                    const int controlHeight) {
        if (!button) {
            return;
        }

        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setText(text);
        button->setToolTip(toolTip);
        button->setStatusTip(toolTip);
        button->setAccessibleName(accessibleName);
        button->setMinimumSize(QSize(132, controlHeight));
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    }

    inline void configureActionButton(QToolButton *button,
                                      const QString &iconPath,
                                      const QString &toolTip,
                                      const int controlHeight,
                                      const bool checkable = false) {
        if (!button) {
            return;
        }

        button->setCheckable(checkable);
        button->setAutoRaise(true);
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(24, 24));
        button->setToolTip(toolTip);
        button->setStatusTip(toolTip);
        button->setAccessibleName(toolTip);
        button->setMinimumSize(QSize(controlHeight, controlHeight));
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    inline void applySizeButtonChrome(QToolButton *button,
                                      const fairwindsk::ui::ComfortChromeColors &colors,
                                      const int controlHeight) {
        fairwindsk::ui::applyBottomBarToolButtonChrome(
            button,
            colors,
            fairwindsk::ui::BottomBarButtonChrome::Flat,
            QSize(),
            controlHeight);
        if (button) {
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        }
    }

    inline void applyActionButtonChrome(QToolButton *button,
                                        const fairwindsk::ui::ComfortChromeColors &colors,
                                        const int controlHeight) {
        fairwindsk::ui::applyBottomBarToolButtonChrome(
            button,
            colors,
            fairwindsk::ui::BottomBarButtonChrome::Flat,
            QSize(24, 24),
            controlHeight);
    }
}

#endif // FAIRWINDSK_UI_SETTINGS_BARSETTINGSUI_HPP
