//
// Created by Raffaele Montella on 08/01/22.
//

#ifndef FAIRWINDSK_ABOUT_HPP
#define FAIRWINDSK_ABOUT_HPP

#include <QWidget>

namespace fairwindsk::ui::about {
    QT_BEGIN_NAMESPACE
    namespace Ui { class About; }
    QT_END_NAMESPACE

    class About : public QWidget {
    Q_OBJECT

    public:
        explicit About(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~About() override;

        QWidget *getCurrentWidget();

    public slots:
        void onAccepted();

    signals:
        void accepted(About *);

    private:
        Ui::About *ui;
        QWidget *m_currentWidget;
    };
} // fairwind::ui::about

#endif //FAIRWINDSK_ABOUT_HPP
