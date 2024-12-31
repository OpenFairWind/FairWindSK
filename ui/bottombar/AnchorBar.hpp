//
// Created by Raffaele Montella on 08/12/24.
//

#ifndef ANCHORBAR_H
#define ANCHORBAR_H

#include <QWidget>
#include <nlohmann/json.hpp>

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

            void onResetClicked();
            void onUpClicked();
            void onRaiseClicked();
            void onRadiusDecClicked();
            void onRadiusIncClicked();
            void onDropClicked();
            void onDownClicked();
            void onReleaseClicked();


        signals:
            void hidden();

        void resetCounter();
        void chainUp();
        void raiseAnchor();
        void radiusDec();
        void radiusInc();
        void dropAnchor();
        void chainDown();
        void chainRelease();


    private:
        Ui::AnchorBar *ui;
        nlohmann::json m_signalkPaths;
    };
} // fairwindsk::ui::bottombar


#endif //ANCHORBAR_H
