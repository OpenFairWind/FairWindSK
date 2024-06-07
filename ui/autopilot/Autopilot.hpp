//
// Created by Raffaele Montella on 04/06/24.
//

#ifndef FAIRWINDSK_AUTOPILOT_HPP
#define FAIRWINDSK_AUTOPILOT_HPP

#include <QWidget>

namespace Ui { class Autopilot; }

namespace fairwindsk::ui::autopilot {


    class Autopilot : public QWidget {
    Q_OBJECT

    public:
        explicit Autopilot(QWidget *parent = nullptr);

        ~Autopilot() override;

    private:
        Ui::Autopilot *ui;
    };
} // fairwindsk::ui::autopilot

#endif //FAIRWINDSK_AUTOPILOT_HPP
