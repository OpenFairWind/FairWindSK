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

#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kTouchScrollControlSize = 44;
        constexpr int kTouchScrollControlSpacing = 4;
        constexpr int kTouchScrollReservedExtent = kTouchScrollControlSize + kTouchScrollControlSpacing;

        QString touchScrollButtonStyle(const QPalette &palette) {
            const QColor base = palette.color(QPalette::Button);
            const QColor border = palette.color(QPalette::Mid);
            const QColor light = base.lighter(150);
            const QColor mid = base.lighter(120);
            const QColor dark = base.darker(120);
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
                .arg(light.name(), mid.name(), dark.name(), border.name(),
                     light.darker(108).name(), dark.name(),
                     light.lighter(110).name(), mid.lighter(108).name(), dark.lighter(108).name(),
                     base.darker(115).name(), base.lighter(120).name(),
                     palette.color(QPalette::AlternateBase).name(),
                     palette.color(QPalette::Disabled, QPalette::ButtonText).name(),
                     palette.color(QPalette::Disabled, QPalette::Mid).name());
        }

        QString touchScrollBarStyle(const QPalette &palette) {
            const QColor track = palette.color(QPalette::AlternateBase);
            const QColor handle = palette.color(QPalette::Button);
            const QColor border = palette.color(QPalette::Mid);
            const QColor handleTop = handle.lighter(145);
            const QColor handleMid = handle.lighter(118);
            const QColor handleBottom = handle.darker(116);

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
                .arg(track.name(), handleTop.name(), handleMid.name(), handleBottom.name(), border.name(),
                     handleTop.darker(108).name(), handleBottom.name(),
                     handleTop.lighter(110).name(), handleMid.lighter(108).name(), handleBottom.lighter(108).name(),
                     handle.darker(115).name(), handle.lighter(120).name());
        }

        QString touchScrollAreaStyle(const QPalette &palette, const bool borderless) {
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

            return areaStyle + touchScrollBarStyle(palette);
        }
    }

    int TouchScrollArea::controlExtent() {
        return kTouchScrollControlSize;
    }

    QString TouchScrollArea::scrollBarStyleSheet() {
        return touchScrollBarStyle(QApplication::palette());
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
        if (styleSheet() != scrollAreaStyle) {
            setStyleSheet(scrollAreaStyle);
        }

        const QString buttonStyle = touchScrollButtonStyle(activePalette);
        if (m_verticalUpButton) {
            if (m_verticalUpButton->styleSheet() != buttonStyle) {
                m_verticalUpButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(
                m_verticalUpButton,
                fairwindsk::ui::bestContrastingColor(
                    activePalette.color(QPalette::Button),
                    {activePalette.color(QPalette::Text),
                     activePalette.color(QPalette::ButtonText),
                     activePalette.color(QPalette::WindowText)}));
        }
        if (m_verticalDownButton) {
            if (m_verticalDownButton->styleSheet() != buttonStyle) {
                m_verticalDownButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(
                m_verticalDownButton,
                fairwindsk::ui::bestContrastingColor(
                    activePalette.color(QPalette::Button),
                    {activePalette.color(QPalette::Text),
                     activePalette.color(QPalette::ButtonText),
                     activePalette.color(QPalette::WindowText)}));
        }
        if (m_horizontalLeftButton) {
            if (m_horizontalLeftButton->styleSheet() != buttonStyle) {
                m_horizontalLeftButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(
                m_horizontalLeftButton,
                fairwindsk::ui::bestContrastingColor(
                    activePalette.color(QPalette::Button),
                    {activePalette.color(QPalette::Text),
                     activePalette.color(QPalette::ButtonText),
                     activePalette.color(QPalette::WindowText)}));
        }
        if (m_horizontalRightButton) {
            if (m_horizontalRightButton->styleSheet() != buttonStyle) {
                m_horizontalRightButton->setStyleSheet(buttonStyle);
            }
            fairwindsk::ui::applyTintedButtonIcon(
                m_horizontalRightButton,
                fairwindsk::ui::bestContrastingColor(
                    activePalette.color(QPalette::Button),
                    {activePalette.color(QPalette::Text),
                     activePalette.color(QPalette::ButtonText),
                     activePalette.color(QPalette::WindowText)}));
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
