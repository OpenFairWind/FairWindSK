//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_ALARMSBAR_HPP
#define FAIRWINDSK_ALARMSBAR_HPP

#include <QWidget>

namespace Ui { class AlarmsBar; }

namespace fairwindsk::ui::bottombar {

    class AlarmsBar : public QWidget {
    Q_OBJECT

    public:
        explicit AlarmsBar(QWidget *parent = nullptr);

        ~AlarmsBar() override;

    public
        slots:
        void onHideClicked();


    signals:
        void hide();

    private:
        Ui::AlarmsBar *ui;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_ALARMSBAR_HPP
