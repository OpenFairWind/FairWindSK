//
// Created by Codex on 02/04/26.
//

#include "TouchScrollArea.hpp"

#include <algorithm>

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QScroller>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {
    namespace {
        struct TouchScrollColors {
            QColor track;
            QColor handleTop;
            QColor handleMid;
            QColor handleBottom;
            QColor border;
            QColor icon;
            QColor disabled;
            QColor disabledText;
            QColor disabledBorder;
        };

        constexpr int kTouchScrollControlSize = 44;
        constexpr int kTouchScrollControlSpacing = 4;
        constexpr int kTouchScrollReservedExtent = kTouchScrollControlSize + kTouchScrollControlSpacing;

        TouchScrollColors effectiveScrollColors(const QPalette &palette) {
            TouchScrollColors colors;
            colors.border = palette.color(QPalette::Mid);
            colors.disabled = palette.color(QPalette::AlternateBase);
            colors.disabledText = palette.color(QPalette::Disabled, QPalette::ButtonText);
            colors.disabledBorder = palette.color(QPalette::Disabled, QPalette::Mid);

            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const auto scrollPalette = fairWindSK
                ? fairWindSK->getActiveComfortScrollPalette(configuration)
                : fairwindsk::UiScrollPalette{};

            colors.track = scrollPalette.track.isValid() ? scrollPalette.track : palette.color(QPalette::Button);
            colors.handleTop = scrollPalette.handleTop.isValid() ? scrollPalette.handleTop : palette.color(QPalette::Button).lighter(145);
            colors.handleMid = scrollPalette.handleMid.isValid() ? scrollPalette.handleMid : palette.color(QPalette::Button).lighter(118);
            colors.handleBottom = scrollPalette.handleBottom.isValid() ? scrollPalette.handleBottom : palette.color(QPalette::Button).darker(116);
            colors.icon = fairwindsk::ui::comfortIconColor(
                configuration,
                fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default"),
                fairwindsk::ui::bestContrastingColor(
                    colors.handleMid,
                    {palette.color(QPalette::Text),
                     palette.color(QPalette::ButtonText),
                     palette.color(QPalette::WindowText)}));
            return colors;
        }

        QString touchScrollButtonStyle(const TouchScrollColors &colors) {
            return QStringLiteral(
                "QPushButton {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %1, stop:0.42 %2, stop:1 %3);"
                " border: 1px solid %4;"
                " border-top-color: %5;"
                " border-bottom-color: %6;"
                " border-radius: 8px;"
                " padding: 2px;"
                " }"
                "QPushButton:hover {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %7, stop:0.4 %8, stop:1 %9);"
                " }"
                "QPushButton:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %10, stop:0.5 %2, stop:1 %11);"
                " border-top-color: %6;"
                " border-bottom-color: %5;"
                " padding-top: 3px;"
                " padding-bottom: 1px;"
                " }"
                "QPushButton:disabled {"
                " background: %12;"
                " color: %13;"
                " border-color: %14;"
                " }")
                .arg(colors.handleTop.name(), colors.handleMid.name(), colors.handleBottom.name(), colors.border.name(),
                     colors.handleTop.darker(108).name(), colors.handleBottom.name(),
                     colors.handleTop.lighter(110).name(), colors.handleMid.lighter(108).name(), colors.handleBottom.lighter(108).name(),
                     colors.handleMid.darker(115).name(), colors.handleTop.lighter(120).name(),
                     colors.disabled.name(), colors.disabledText.name(), colors.disabledBorder.name());
        }

        QString touchScrollBarStyle(const TouchScrollColors &colors) {
            return QStringLiteral(
                "QScrollBar:vertical {"
                " background: %1;"
                " width: 44px;"
                " margin: 48px 4px 48px 0px;"
                " border-radius: 12px;"
                " }"
                "QScrollBar::handle:vertical {"
                " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                " stop:0 %2, stop:0.35 %3, stop:1 %4);"
                " min-height: 44px;"
                " border: 1px solid %5;"
                " border-top-color: %6;"
                " border-bottom-color: %7;"
                " border-radius: 12px;"
                " }"
                "QScrollBar::handle:vertical:hover {"
                " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                " stop:0 %8, stop:0.35 %9, stop:1 %10);"
                " }"
                "QScrollBar::handle:vertical:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                " stop:0 %11, stop:0.5 %3, stop:1 %12);"
                " }"
                "QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.35 %3, stop:1 %4);"
                " border: 1px solid %5;"
                " border-radius: 10px;"
                " width: 40px;"
                " height: 40px;"
                " subcontrol-origin: margin;"
                " }"
                "QScrollBar::sub-line:vertical { subcontrol-position: top; }"
                "QScrollBar::add-line:vertical { subcontrol-position: bottom; }"
                "QScrollBar:horizontal {"
                " background: %1;"
                " height: 44px;"
                " margin: 0px 48px 4px 48px;"
                " border-radius: 12px;"
                " }"
                "QScrollBar::handle:horizontal {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.35 %3, stop:1 %4);"
                " min-width: 44px;"
                " border: 1px solid %5;"
                " border-top-color: %6;"
                " border-bottom-color: %7;"
                " border-radius: 12px;"
                " }"
                "QScrollBar::handle:horizontal:hover {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %8, stop:0.35 %9, stop:1 %10);"
                " }"
                "QScrollBar::handle:horizontal:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %11, stop:0.5 %3, stop:1 %12);"
                " }"
                "QScrollBar::sub-line:horizontal, QScrollBar::add-line:horizontal {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.35 %3, stop:1 %4);"
                " border: 1px solid %5;"
                " border-radius: 10px;"
                " width: 40px;"
                " height: 40px;"
                " subcontrol-origin: margin;"
                " }"
                "QScrollBar::sub-line:horizontal { subcontrol-position: left; }"
                "QScrollBar::add-line:horizontal { subcontrol-position: right; }"
                "QScrollBar::add-page, QScrollBar::sub-page {"
                " background: transparent;"
                " border: none;"
                " }")
                .arg(colors.track.name(), colors.handleTop.name(), colors.handleMid.name(), colors.handleBottom.name(), colors.border.name(),
                     colors.handleTop.darker(108).name(), colors.handleBottom.name(),
                     colors.handleTop.lighter(110).name(), colors.handleMid.lighter(108).name(), colors.handleBottom.lighter(108).name(),
                     colors.handleMid.darker(115).name(), colors.handleTop.lighter(120).name());
        }

        QString touchScrollAreaStyle(const QPalette &palette, const bool borderless) {
            const TouchScrollColors colors = effectiveScrollColors(palette);
            const QString areaStyle = borderless
                ? QStringLiteral(
                    "QScrollArea {"
                    " border: none;"
                    " border-radius: 0px;"
                    " background: transparent;"
                    " }"
                    "QScrollArea > QWidget > QWidget {"
                    " background: transparent;"
                    " }")
                : QStringLiteral(
                    "QScrollArea {"
                    " border: 1px solid %1;"
                    " border-radius: 8px;"
                    " background: %2;"
                    " }")
                    .arg(palette.color(QPalette::Mid).name(), palette.color(QPalette::Base).name());

            return areaStyle + touchScrollBarStyle(colors);
        }
    }

    int TouchScrollArea::controlExtent() {
        return kTouchScrollControlSize;
    }

    QString TouchScrollArea::scrollBarStyleSheet() {
        return touchScrollBarStyle(effectiveScrollColors(QApplication::palette()));
    }

    bool TouchScrollArea::isBorderless() const {
        return m_borderless;
    }

    void TouchScrollArea::setBorderless(const bool borderless) {
        if (m_borderless == borderless) {
            return;
        }

        m_borderless = borderless;
        applyTouchStyle();
    }

    TouchScrollArea::TouchScrollArea(QWidget *parent)
        : QScrollArea(parent) {
        setObjectName(QStringLiteral("touchScrollArea"));
        QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(viewport(), QScroller::TouchGesture);
        setFrameShape(QFrame::NoFrame);

        m_verticalUpButton = createScrollButton(verticalScrollBar(), QStringLiteral(":/resources/svg/OpenBridge/arrow-up-google.svg"));
        m_verticalDownButton = createScrollButton(verticalScrollBar(), QStringLiteral(":/resources/svg/OpenBridge/arrow-down-google.svg"));
        m_horizontalLeftButton = createScrollButton(horizontalScrollBar(), QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_horizontalRightButton = createScrollButton(horizontalScrollBar(), QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg"));

        connect(m_verticalUpButton, &QPushButton::clicked, this, [this]() {
            if (verticalScrollBar()) {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }
        });
        connect(m_verticalDownButton, &QPushButton::clicked, this, [this]() {
            if (verticalScrollBar()) {
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
        });
        connect(m_horizontalLeftButton, &QPushButton::clicked, this, [this]() {
            if (horizontalScrollBar()) {
                horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }
        });
        connect(m_horizontalRightButton, &QPushButton::clicked, this, [this]() {
            if (horizontalScrollBar()) {
                horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
        });

        connectScrollBar(verticalScrollBar());
        connectScrollBar(horizontalScrollBar());
        applyTouchStyle();
        QTimer::singleShot(0, this, &TouchScrollArea::updateScrollControls);
    }

    void TouchScrollArea::applyTouchStyle() {
        if (m_isApplyingStyle) {
            return;
        }

        m_isApplyingStyle = true;
        const QPalette activePalette = palette();
        const QString scrollAreaStyle = touchScrollAreaStyle(activePalette, m_borderless);
        const TouchScrollColors colors = effectiveScrollColors(activePalette);
        if (styleSheet() != scrollAreaStyle) {
            setStyleSheet(scrollAreaStyle);
        }

        const QString buttonStyle = touchScrollButtonStyle(colors);
        if (m_verticalUpButton) {
            if (m_verticalUpButton->styleSheet() != buttonStyle) {
                m_verticalUpButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(m_verticalUpButton, colors.icon);
        }
        if (m_verticalDownButton) {
            if (m_verticalDownButton->styleSheet() != buttonStyle) {
                m_verticalDownButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(m_verticalDownButton, colors.icon);
        }
        if (m_horizontalLeftButton) {
            if (m_horizontalLeftButton->styleSheet() != buttonStyle) {
                m_horizontalLeftButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(m_horizontalLeftButton, colors.icon);
        }
        if (m_horizontalRightButton) {
            if (m_horizontalRightButton->styleSheet() != buttonStyle) {
                m_horizontalRightButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(m_horizontalRightButton, colors.icon);
        }
        m_isApplyingStyle = false;
    }

    QPushButton *TouchScrollArea::createScrollButton(QScrollBar *parentScrollBar, const QString &iconPath) const {
        if (!parentScrollBar) {
            return nullptr;
        }

        auto *button = new QPushButton(parentScrollBar);
        button->setIcon(QIcon(iconPath));
        button->setFocusPolicy(Qt::NoFocus);
        button->setAutoRepeat(true);
        button->setAutoRepeatDelay(250);
        button->setAutoRepeatInterval(60);
        button->setMinimumSize(kTouchScrollControlSize, kTouchScrollControlSize);
        button->hide();
        return button;
    }

    bool TouchScrollArea::event(QEvent *event) {
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            applyTouchStyle();
        }
        return QScrollArea::event(event);
    }

    void TouchScrollArea::connectScrollBar(QScrollBar *scrollBar) {
        if (!scrollBar) {
            return;
        }

        scrollBar->installEventFilter(this);
        connect(scrollBar, &QScrollBar::rangeChanged, this, &TouchScrollArea::updateScrollControls);
        connect(scrollBar, &QScrollBar::valueChanged, this, &TouchScrollArea::updateScrollControls);
    }

    bool TouchScrollArea::eventFilter(QObject *watched, QEvent *event) {
        if ((watched == verticalScrollBar() || watched == horizontalScrollBar()) && event) {
            switch (event->type()) {
                case QEvent::Resize:
                case QEvent::Show:
                case QEvent::Hide:
                case QEvent::Move:
                case QEvent::EnabledChange:
                case QEvent::StyleChange:
                    updateScrollControls();
                    break;
                default:
                    break;
            }
        }

        return QScrollArea::eventFilter(watched, event);
    }

    void TouchScrollArea::resizeEvent(QResizeEvent *event) {
        QScrollArea::resizeEvent(event);
        updateScrollControls();
    }

    void TouchScrollArea::updateScrollControls() {
        auto *vBar = verticalScrollBar();
        auto *hBar = horizontalScrollBar();
        if (!vBar || !hBar) {
            return;
        }

        const bool showVertical = vBar->isVisible() && vBar->maximum() > vBar->minimum();
        const bool showHorizontal = hBar->isVisible() && hBar->maximum() > hBar->minimum();

        const QRect vRect = vBar->geometry();
        const QRect hRect = hBar->geometry();

        const int vExtent = showVertical ? std::max(kTouchScrollControlSize, vRect.width()) : kTouchScrollControlSize;
        const int hExtent = showHorizontal ? std::max(kTouchScrollControlSize, hRect.height()) : kTouchScrollControlSize;

        const QSize verticalIconSize(vExtent / 2, vExtent / 2);
        const QSize horizontalIconSize(hExtent / 2, hExtent / 2);

        if (m_verticalUpButton) {
            m_verticalUpButton->setFixedSize(vRect.width(), vExtent);
            m_verticalUpButton->setIconSize(verticalIconSize);
            m_verticalUpButton->setEnabled(vBar->value() > vBar->minimum());
        }
        if (m_verticalDownButton) {
            m_verticalDownButton->setFixedSize(vRect.width(), vExtent);
            m_verticalDownButton->setIconSize(verticalIconSize);
            m_verticalDownButton->setEnabled(vBar->value() < vBar->maximum());
        }
        if (m_horizontalLeftButton) {
            m_horizontalLeftButton->setFixedSize(hExtent, hRect.height());
            m_horizontalLeftButton->setIconSize(horizontalIconSize);
            m_horizontalLeftButton->setEnabled(hBar->value() > hBar->minimum());
        }
        if (m_horizontalRightButton) {
            m_horizontalRightButton->setFixedSize(hExtent, hRect.height());
            m_horizontalRightButton->setIconSize(horizontalIconSize);
            m_horizontalRightButton->setEnabled(hBar->value() < hBar->maximum());
        }

        if (showVertical) {
            if (m_verticalUpButton) {
                m_verticalUpButton->setGeometry(0, 0, vRect.width(), vExtent);
                m_verticalUpButton->show();
                m_verticalUpButton->raise();
            }
            if (m_verticalDownButton) {
                m_verticalDownButton->setGeometry(0, vRect.height() - vExtent, vRect.width(), vExtent);
                m_verticalDownButton->show();
                m_verticalDownButton->raise();
            }
        } else {
            if (m_verticalUpButton) {
                m_verticalUpButton->hide();
            }
            if (m_verticalDownButton) {
                m_verticalDownButton->hide();
            }
        }

        if (showHorizontal) {
            if (m_horizontalLeftButton) {
                m_horizontalLeftButton->setGeometry(0, 0, hExtent, hRect.height());
                m_horizontalLeftButton->show();
                m_horizontalLeftButton->raise();
            }
            if (m_horizontalRightButton) {
                m_horizontalRightButton->setGeometry(hRect.width() - hExtent, 0, hExtent, hRect.height());
                m_horizontalRightButton->show();
                m_horizontalRightButton->raise();
            }
        } else {
            if (m_horizontalLeftButton) {
                m_horizontalLeftButton->hide();
            }
            if (m_horizontalRightButton) {
                m_horizontalRightButton->hide();
            }
        }
    }
}
