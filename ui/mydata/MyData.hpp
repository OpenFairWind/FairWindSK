//
// Created by Raffaele Montella on 07/06/24.
//

#ifndef FAIRWINDSK_MYDATA_HPP
#define FAIRWINDSK_MYDATA_HPP

#include <QWidget>

namespace Ui { class MyData; }

namespace fairwindsk::ui::mydata {

    class MyData : public QWidget {
    Q_OBJECT

    public:
        explicit MyData(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~MyData() override;

        QWidget *getCurrentWidget();

    public slots:
        void onClose();

    signals:
        void closed(MyData *);

    private:
        Ui::MyData *ui;
        QWidget *m_currentWidget;
    };
} // fairwindsk::ui::mydata

#endif //FAIRWINDSK_MYDATA_HPP
