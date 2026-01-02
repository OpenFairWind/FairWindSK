//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_POBBAR_HPP
#define FAIRWINDSK_POBBAR_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <QSet>
#include <QWidget>

#include "Units.hpp"

namespace Ui { class POBBar; }

namespace fairwindsk::ui::bottombar {

    class POBBar : public QWidget {
    Q_OBJECT

    public:
        explicit POBBar(QWidget *parent = nullptr);

        ~POBBar() override;

        void POB();

    public
        slots:

        void updatePOB(const QJsonObject& update);
        void updateBearing(const QJsonObject& update);
        void updateDistance(const QJsonObject& update);

        void onCancelClicked();
        void onHideClicked();
        void onCurrentIndexChanged(int index);
        void updateElapsed();


    signals:
        void cancelPOB();
        void hidden();

    private:
        Ui::POBBar *ui;
        Units *m_units;
        nlohmann::json m_signalkPaths;
        QTimer *m_timer = nullptr;
        QSet<QString> m_pobUUIDs;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_POBBAR_HPP
