//
// Created by Raffaele Montella on 08/01/22.
//

#ifndef FAIRWINDSK_ABOUT_HPP
#define FAIRWINDSK_ABOUT_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_About.h"

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

    private:
        Ui::About *ui;
        QWidget *m_currentWidget;
    };
} // fairwindsk::ui::about

#endif //FAIRWINDSK_ABOUT_HPP
