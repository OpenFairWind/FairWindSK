//
// Created by Codex on 02/04/26.
//

#ifndef FAIRWINDSK_TOUCHSCROLLAREA_HPP
#define FAIRWINDSK_TOUCHSCROLLAREA_HPP

#include <QPointer>
#include <QScrollArea>

class QEvent;
class QPushButton;
class QResizeEvent;
class QScrollBar;

namespace fairwindsk::ui::widgets {
    class TouchScrollArea : public QScrollArea {
        Q_OBJECT

    public:
        explicit TouchScrollArea(QWidget *parent = nullptr);
        ~TouchScrollArea() override = default;

    private:
        QPushButton *createScrollButton(QScrollBar *parentScrollBar, const QString &iconPath) const;
        bool eventFilter(QObject *watched, QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void applyTouchStyle();
        void connectScrollBar(QScrollBar *scrollBar);
        void updateScrollControls();

        QPointer<QPushButton> m_verticalUpButton;
        QPointer<QPushButton> m_verticalDownButton;
        QPointer<QPushButton> m_horizontalLeftButton;
        QPointer<QPushButton> m_horizontalRightButton;
    };
}

#endif // FAIRWINDSK_TOUCHSCROLLAREA_HPP
