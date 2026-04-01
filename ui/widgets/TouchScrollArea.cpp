//
// Created by Codex on 02/04/26.
//

#include "TouchScrollArea.hpp"

#include <algorithm>

#include <QEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScroller>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>

namespace fairwindsk::ui::widgets {
    namespace {
        const QString kTouchScrollButtonStyle = QStringLiteral(
            "QPushButton {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #fcfcfd, stop:0.45 #eef2f7, stop:1 #d7dde6);"
            " border: 1px solid #7b8794;"
            " border-top-color: #aeb8c4;"
            " border-bottom-color: #5d6875;"
            " border-radius: 8px;"
            " padding: 2px;"
            " }"
            "QPushButton:hover {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #ffffff, stop:0.45 #f4f7fb, stop:1 #dfe5ee);"
            " }"
            "QPushButton:pressed {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #c8d1dc, stop:0.5 #e3e8ef, stop:1 #f7f9fb);"
            " border-top-color: #596473;"
            " border-bottom-color: #a9b3bf;"
            " padding-top: 3px;"
            " padding-bottom: 1px;"
            " }"
            "QPushButton:disabled {"
            " background: #d9dde3;"
            " color: #9aa3ad;"
            " border-color: #aab3bc;"
            " }");
        const QString kTouchScrollAreaStyle = QStringLiteral(
            "QScrollArea {"
            " border: 1px solid #7b8794;"
            " border-radius: 8px;"
            " background: #f5f5f2;"
            " }"
            "QScrollBar:vertical {"
            " background: #d7dde6;"
            " width: 22px;"
            " margin: 26px 4px 26px 0px;"
            " border-radius: 10px;"
            " }"
            "QScrollBar::handle:vertical {"
            " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            " stop:0 #fdfefe, stop:0.35 #eef2f7, stop:1 #bac4cf);"
            " min-height: 36px;"
            " border: 1px solid #7b8794;"
            " border-top-color: #aeb8c4;"
            " border-bottom-color: #5d6875;"
            " border-radius: 9px;"
            " }"
            "QScrollBar::handle:vertical:hover {"
            " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            " stop:0 #ffffff, stop:0.35 #f4f7fb, stop:1 #c8d1dc);"
            " }"
            "QScrollBar::handle:vertical:pressed {"
            " background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            " stop:0 #c8d1dc, stop:0.5 #e3e8ef, stop:1 #f7f9fb);"
            " }"
            "QScrollBar:horizontal {"
            " background: #d7dde6;"
            " height: 22px;"
            " margin: 0px 26px 4px 26px;"
            " border-radius: 10px;"
            " }"
            "QScrollBar::handle:horizontal {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #fdfefe, stop:0.35 #eef2f7, stop:1 #bac4cf);"
            " min-width: 36px;"
            " border: 1px solid #7b8794;"
            " border-top-color: #aeb8c4;"
            " border-bottom-color: #5d6875;"
            " border-radius: 9px;"
            " }"
            "QScrollBar::handle:horizontal:hover {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #ffffff, stop:0.35 #f4f7fb, stop:1 #c8d1dc);"
            " }"
            "QScrollBar::handle:horizontal:pressed {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #c8d1dc, stop:0.5 #e3e8ef, stop:1 #f7f9fb);"
            " }"
            "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page {"
            " background: transparent;"
            " border: none;"
            " width: 0px;"
            " height: 0px;"
            " }");
    }

    TouchScrollArea::TouchScrollArea(QWidget *parent)
        : QScrollArea(parent) {
        setObjectName(QStringLiteral("touchScrollArea"));
        QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(viewport(), QScroller::TouchGesture);
        setFrameShape(QFrame::NoFrame);

        m_verticalUpButton = new QPushButton(this);
        m_verticalDownButton = new QPushButton(this);
        m_horizontalLeftButton = new QPushButton(this);
        m_horizontalRightButton = new QPushButton(this);

        m_verticalUpButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-up-google.svg")));
        m_verticalDownButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-down-google.svg")));
        m_horizontalLeftButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg")));
        m_horizontalRightButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg")));

        applyButtonStyle(m_verticalUpButton);
        applyButtonStyle(m_verticalDownButton);
        applyButtonStyle(m_horizontalLeftButton);
        applyButtonStyle(m_horizontalRightButton);

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
        setStyleSheet(kTouchScrollAreaStyle);
    }

    void TouchScrollArea::applyButtonStyle(QPushButton *button) const {
        if (!button) {
            return;
        }

        button->setStyleSheet(kTouchScrollButtonStyle);
        button->setFocusPolicy(Qt::NoFocus);
        button->setAutoRepeat(true);
        button->setAutoRepeatDelay(250);
        button->setAutoRepeatInterval(60);
        button->hide();
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

        const int vExtent = std::max(22, vRect.width());
        const int hExtent = std::max(22, hRect.height());

        const QSize verticalIconSize(std::max(12, vExtent / 2), std::max(12, vExtent / 2));
        const QSize horizontalIconSize(std::max(12, hExtent / 2), std::max(12, hExtent / 2));

        m_verticalUpButton->setFixedSize(vExtent, vExtent);
        m_verticalDownButton->setFixedSize(vExtent, vExtent);
        m_horizontalLeftButton->setFixedSize(hExtent, hExtent);
        m_horizontalRightButton->setFixedSize(hExtent, hExtent);

        m_verticalUpButton->setIconSize(verticalIconSize);
        m_verticalDownButton->setIconSize(verticalIconSize);
        m_horizontalLeftButton->setIconSize(horizontalIconSize);
        m_horizontalRightButton->setIconSize(horizontalIconSize);

        if (showVertical) {
            m_verticalUpButton->setGeometry(vRect.x(), vRect.top() - 1, vRect.width(), vExtent);
            m_verticalDownButton->setGeometry(vRect.x(), vRect.bottom() - vExtent + 1, vRect.width(), vExtent);
            m_verticalUpButton->show();
            m_verticalDownButton->show();
            m_verticalUpButton->raise();
            m_verticalDownButton->raise();
        } else {
            m_verticalUpButton->hide();
            m_verticalDownButton->hide();
        }

        if (showHorizontal) {
            m_horizontalLeftButton->setGeometry(hRect.left() - 1, hRect.y(), hExtent, hRect.height());
            m_horizontalRightButton->setGeometry(hRect.right() - hExtent + 1, hRect.y(), hExtent, hRect.height());
            m_horizontalLeftButton->show();
            m_horizontalRightButton->show();
            m_horizontalLeftButton->raise();
            m_horizontalRightButton->raise();
        } else {
            m_horizontalLeftButton->hide();
            m_horizontalRightButton->hide();
        }

        if (showVertical && showHorizontal) {
            const QRect cornerRect(vRect.left(), hRect.top(), vRect.width(), hRect.height());
            m_verticalDownButton->setGeometry(cornerRect.x(),
                                              vRect.bottom() - vExtent + 1,
                                              cornerRect.width(),
                                              vExtent);
            m_horizontalRightButton->setGeometry(hRect.right() - hExtent + 1,
                                                 cornerRect.y(),
                                                 hExtent,
                                                 cornerRect.height());
        }
    }
}
