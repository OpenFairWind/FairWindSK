//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_MOBBAR_HPP
#define FAIRWINDSK_MOBBAR_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <QWidget>

#include "Units.hpp"

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

        void updateMOB(const QJsonObject& update);
        void updateBearing(const QJsonObject& update);
        void updateDistance(const QJsonObject& update);

        void onCancelClicked();
        void onHideClicked();


    signals:
        void cancelMOB();
        void hide();

    private:
        Ui::MOBBar *ui;
        Units *m_units;
        nlohmann::json m_signalkPaths;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_MOBBAR_HPP
