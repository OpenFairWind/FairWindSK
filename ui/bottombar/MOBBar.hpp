//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_MOBBAR_HPP
#define FAIRWINDSK_MOBBAR_HPP

#include <QWidget>

namespace Ui { class MOBBar; }

namespace fairwindsk::ui::bottombar {

    class MOBBar : public QWidget {
    Q_OBJECT

    public:
        explicit MOBBar(QWidget *parent = nullptr);

        ~MOBBar() override;

        void MOB();

    public
        slots:
        void onCancelClicked();
        void onHideClicked();


    signals:
        void cancelMOB();
        void hide();

    private:
        Ui::MOBBar *ui;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_MOBBAR_HPP
