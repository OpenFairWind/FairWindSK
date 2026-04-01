//
// Created by Codex on 02/04/26.
//

#ifndef FAIRWINDSK_TOUCHSCROLLAREA_HPP
#define FAIRWINDSK_TOUCHSCROLLAREA_HPP

#include <QScrollArea>

namespace fairwindsk::ui::widgets {
    class TouchScrollArea : public QScrollArea {
        Q_OBJECT

    public:
        explicit TouchScrollArea(QWidget *parent = nullptr);
        ~TouchScrollArea() override = default;

    private:
        void applyTouchStyle();
    };
}

#endif // FAIRWINDSK_TOUCHSCROLLAREA_HPP
