//
// Created by Codex on 02/04/26.
//

#include "TouchScrollArea.hpp"

#include <QScroller>

namespace fairwindsk::ui::widgets {
    namespace {
        const QString kTouchScrollAreaStyle = QStringLiteral(
            "QScrollArea {"
            " border: 1px solid #7b8794;"
            " border-radius: 8px;"
            " background: #f5f5f2;"
            " }"
            "QScrollBar:vertical {"
            " background: #d7dde6;"
            " width: 22px;"
            " margin: 4px 4px 4px 0px;"
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
            " margin: 0px 4px 4px 4px;"
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
        applyTouchStyle();
    }

    void TouchScrollArea::applyTouchStyle() {
        setStyleSheet(kTouchScrollAreaStyle);
    }
}
