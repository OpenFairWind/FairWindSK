//
// Created by Raffaele Montella on 04/06/24.
//

#ifndef FAIRWINDSK_NAVIGATIONBAR_HPP
#define FAIRWINDSK_NAVIGATIONBAR_HPP

#include <QWidget>

namespace Ui { class NavigationBar; }

namespace fairwindsk::ui::web {

    class NavigationBar : public QWidget {
    Q_OBJECT

    public:
        explicit NavigationBar(QWidget *parent = nullptr);

        ~NavigationBar() override;

    public slots:
        void onHomeClicked();
        void onBackClicked();
        void onForwardClicked();
        void onReloadClicked();
        void onSettingsCicked();

    signals:
        void home();
        void back();
        void forward();
        void reload();
        void settings();

    private:
        Ui::NavigationBar *ui;
    };
} // fairwindsk::ui::web

#endif //FAIRWINDSK_NAVIGATIONBAR_HPP
