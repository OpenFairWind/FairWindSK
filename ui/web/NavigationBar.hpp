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

        void setBackEnabled(bool enabled) const;
        void setForwardEnabled(bool enabled) const;
        void setHomeEnabled(bool enabled) const;
        void setSettingsEnabled(bool enabled) const;
        void setReloadActive(bool loading) const;
        void setZoomPercent(double zoomPercent) const;

    public slots:
        void onHomeClicked();
        void onBackClicked();
        void onForwardClicked();
        void onReloadClicked();
        void onZoomOutClicked();
        void onZoomInClicked();
        void onSettingsClicked();
        void onCloseClicked();

    signals:
        void home();
        void back();
        void forward();
        void reload();
        void zoomOut();
        void zoomIn();
        void settings();
        void close();

    private:
        void changeEvent(QEvent *event) override;
        void retintIcons() const;
        Ui::NavigationBar *ui;
    };
} // fairwindsk::ui::web

#endif //FAIRWINDSK_NAVIGATIONBAR_HPP
