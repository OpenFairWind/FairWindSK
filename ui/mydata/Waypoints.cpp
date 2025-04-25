//
// Created by Raffaele Montella on 15/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Waypoints.h" resolved

#include "Waypoints.hpp"


#include "ui_Waypoints.h"


namespace fairwindsk::ui::mydata {

    Waypoints::Waypoints(MyData *myData, QWidget *parent) :
            QWidget(parent), ui(new Ui::Waypoints) {

        m_myData = myData;

        ui->setupUi(this);

        m_waypointsModel = new WaypointsModel();
        ui->tableView_Waypoints->setModel(m_waypointsModel);
        ui->tableView_Waypoints->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    }

    Waypoints::~Waypoints() {
        delete ui;
    }
} // fairwindsk::ui::mydata
