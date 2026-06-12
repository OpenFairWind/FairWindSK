//
// Created by Raffaele Montella on 08/01/22.
//

#ifndef FAIRWINDSK_ABOUT_HPP
#define FAIRWINDSK_ABOUT_HPP

#include <QPixmap>
#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_About.h"

class QResizeEvent;

namespace Ui { class About; }

namespace fairwindsk::ui::about {

    class About : public QWidget {
    Q_OBJECT

    public:
        explicit About(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~About() override;

        QWidget *getCurrentWidget();
        void setCurrentWidget(QWidget *currentWidget);

    public slots:
        void onClose();

    signals:
        void closed(About *);

    protected:
        void resizeEvent(QResizeEvent *event) override;

    private:
        void updateLogoPixmap();

        Ui::About *ui;
        QWidget *m_currentWidget;
        QPixmap m_logoPixmap;
    };
} // fairwindsk::ui::about

#endif //FAIRWINDSK_ABOUT_HPP
