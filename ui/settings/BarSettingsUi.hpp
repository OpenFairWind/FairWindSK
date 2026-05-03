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
#include "ui/layout/BarLayout.hpp"

namespace fairwindsk::ui::settings::barsettings {
    inline QString previewEntryText(const fairwindsk::ui::layout::LayoutEntry &entry) {
        using fairwindsk::ui::layout::EntryKind;

        if (entry.kind == EntryKind::Separator || entry.kind == EntryKind::Stretch) {
            return {};
        }
        if (entry.widgetId == QStringLiteral("current_context")) {
            return QObject::tr("APP");
        }
        if (entry.widgetId == QStringLiteral("clock_icons")) {
            return QObject::tr("Clock");
        }
        if (entry.widgetId == QStringLiteral("open_apps")) {
            return QObject::tr("Apps");
        }
        if (entry.widgetId == QStringLiteral("signalk_status")) {
            return QObject::tr("Signal K");
        }

        return fairwindsk::ui::layout::entryLabel(entry);
    }

    inline QString listWidgetChrome(const fairwindsk::ui::ComfortChromeColors &colors,
                                    const bool dashedItems = false) {
        return QStringLiteral(
            "QListWidget {"
            " border: 1px solid %1;"
            " border-radius: 8px;"
            " padding: 4px;"
            " background: %2;"
            " }"
            "QListWidget::item {"
            " margin: 2px;"
            " padding: 6px 8px;"
            " border: 1px %3 %4;"
            " border-radius: 6px;"
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
                 fairwindsk::ui::comfortAlpha(colors.buttonBackground, dashedItems ? 12 : 20).name(QColor::HexArgb),
                 colors.text.name(),
                 colors.accentTop.name(),
                 fairwindsk::ui::comfortAlpha(colors.accentTop, 46).name(QColor::HexArgb),
                 colors.text.name());
    }

    inline QString previewFrameChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
        Q_UNUSED(colors)
        return QStringLiteral(
            "QFrame {"
            " border: none;"
            " background: transparent;"
            " }");
    }

    inline QString paletteChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
        Q_UNUSED(colors)
        return QStringLiteral(
            "QScrollArea { border: none; background: transparent; }"
            "QWidget { background: transparent; }");
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
                                    const QString &iconPath,
                                    const QString &toolTip,
                                    const QString &accessibleName,
                                    const int controlHeight) {
        if (!button) {
            return;
        }

        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(30, 30));
        button->setToolTip(toolTip);
        button->setStatusTip(toolTip);
        button->setAccessibleName(accessibleName);
        button->setMinimumSize(QSize(controlHeight, controlHeight));
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
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
        button->setIconSize(QSize(28, 28));
        button->setToolTip(toolTip);
        button->setStatusTip(toolTip);
        button->setAccessibleName(toolTip);
        button->setMinimumSize(QSize(controlHeight, controlHeight));
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    inline void configureHeightButton(QToolButton *button,
                                      const QString &toolTip,
                                      const int controlHeight) {
        configureActionButton(button,
                              QStringLiteral(":/resources/svg/OpenBridge/layout-extend-height.svg"),
                              toolTip,
                              controlHeight,
                              true);
        if (!button) {
            return;
        }

        QIcon icon;
        icon.addFile(QStringLiteral(":/resources/svg/OpenBridge/layout-extend-height.svg"), QSize(28, 28), QIcon::Normal, QIcon::Off);
        icon.addFile(QStringLiteral(":/resources/svg/OpenBridge/layout-vertical-minimize.svg"), QSize(28, 28), QIcon::Normal, QIcon::On);
        button->setIcon(icon);
        button->setIconSize(QSize(28, 28));
    }

    inline void applySizeButtonChrome(QToolButton *button,
                                      const fairwindsk::ui::ComfortChromeColors &colors,
                                      const int controlHeight) {
        fairwindsk::ui::applyBottomBarToolButtonChrome(
            button,
            colors,
            fairwindsk::ui::BottomBarButtonChrome::Accent,
            QSize(30, 30),
            controlHeight);
        if (button) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            fairwindsk::ui::applyTintedButtonIcon(button, colors.text, QSize(30, 30));
        }
    }

    inline void applyActionButtonChrome(QToolButton *button,
                                        const fairwindsk::ui::ComfortChromeColors &colors,
                                        const int controlHeight) {
        fairwindsk::ui::applyBottomBarToolButtonChrome(
            button,
            colors,
            fairwindsk::ui::BottomBarButtonChrome::Accent,
            QSize(28, 28),
            controlHeight);
        if (button) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            fairwindsk::ui::applyTintedButtonIcon(button, colors.text, QSize(28, 28));
        }
    }
}

#endif // FAIRWINDSK_UI_SETTINGS_BARSETTINGSUI_HPP
