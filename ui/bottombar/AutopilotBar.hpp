//
// Created by Raffaele Montella on 04/06/24.
//

#ifndef FAIRWINDSK_AUTOPILOTBAR_HPP
#define FAIRWINDSK_AUTOPILOTBAR_HPP

#include <QWidget>

namespace Ui { class AutopilotBar; }

namespace fairwindsk::ui::bottombar {


    class AutopilotBar : public QWidget {
    Q_OBJECT

    public:
        explicit AutopilotBar(QWidget *parent = nullptr);

        ~AutopilotBar() override;

    public
        slots:
        void onHideClicked();


    signals:
        void hide();

    private:
        Ui::AutopilotBar *ui;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_AUTOPILOTBAR_HPP
