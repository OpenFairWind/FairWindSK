//
// Created by Raffaele Montella on 07/06/24.
//

#ifndef FAIRWINDSK_MYDATA_HPP
#define FAIRWINDSK_MYDATA_HPP

#include <QWidget>

namespace fairwindsk::ui::mydata {

    QT_BEGIN_NAMESPACE
    namespace Ui { class MyData; }
    QT_END_NAMESPACE

    class MyData : public QWidget {
    Q_OBJECT

    public:
        explicit MyData(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~MyData() override;

        QWidget *getCurrentWidget();
        void refreshFromConfiguration();

    public slots:
        void onClose();

    signals:
        void closed(MyData *);

    private:
        void initTabs(int currentIndex);
        void removeTabs();
        QWidget *createTabPage(int index) const;
        QString tabTitle(int index) const;
        QWidget *createPlaceholderPage() const;
        void ensureTabInitialized(int index);
        bool isPlaceholderPage(const QWidget *page) const;

        Ui::MyData *ui;
        QWidget *m_currentWidget = nullptr;
    };
} // fairwindsk::ui::mydata

#endif //FAIRWINDSK_MYDATA_HPP
