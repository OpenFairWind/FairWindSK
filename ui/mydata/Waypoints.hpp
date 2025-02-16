//
// Created by Raffaele Montella on 15/02/25.
//

#ifndef WAYPOINTS_H
#define WAYPOINTS_H

#include <QWidget>
#include "MyData.hpp"

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class Waypoints; }
QT_END_NAMESPACE

class Waypoints : public QWidget {
Q_OBJECT

public:
    explicit Waypoints(MyData *myData, QWidget *parent = nullptr);
    ~Waypoints() override;

private:
    Ui::Waypoints *ui;

    MyData *m_myData;
};
} // fairwindsk::ui::mydata

#endif //WAYPOINTS_H
