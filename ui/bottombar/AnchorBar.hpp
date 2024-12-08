//
// Created by Raffaele Montella on 08/12/24.
//

#ifndef ANCHORBAR_H
#define ANCHORBAR_H

#include <QWidget>

namespace Ui { class AnchorBar; }

namespace fairwindsk::ui::bottombar {

    class AnchorBar : public QWidget {
        Q_OBJECT

        public:
        explicit AnchorBar(QWidget *parent = nullptr);

        ~AnchorBar() override;

        public
            slots:
            void onHideClicked();


        signals:
            void hide();

    private:
        Ui::AnchorBar *ui;
    };
} // fairwindsk::ui::bottombar


#endif //ANCHORBAR_H
