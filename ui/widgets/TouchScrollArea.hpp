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
        Q_PROPERTY(bool borderless READ isBorderless WRITE setBorderless)

    public:
        explicit TouchScrollArea(QWidget *parent = nullptr);
        ~TouchScrollArea() override = default;
        static int controlExtent();
        static QString scrollBarStyleSheet();
        bool isBorderless() const;
        void setBorderless(bool borderless);

    private:
        bool event(QEvent *event) override;
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
        bool m_isApplyingStyle = false;
        bool m_borderless = false;
    };
}

#endif // FAIRWINDSK_TOUCHSCROLLAREA_HPP
