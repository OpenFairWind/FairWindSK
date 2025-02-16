//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <QWidget>

#include "signalk/Waypoint.hpp"

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class Waypoint; }
QT_END_NAMESPACE

class Waypoint : public QWidget {
Q_OBJECT

public:
    explicit Waypoint(signalk::Waypoint *waypoint, QWidget *parent = nullptr);
    ~Waypoint() override;

private:
    Ui::Waypoint *ui;

    signalk::Waypoint *m_waypoint{};
};
} // fairwindsk::ui::mydata

#endif //WAYPOINT_H
