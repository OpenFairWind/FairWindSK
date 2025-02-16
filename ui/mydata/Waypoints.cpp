//
// Created by Raffaele Montella on 15/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Waypoints.h" resolved

#include "Waypoints.hpp"
#include "Waypoints.hpp"

#include <ui_Waypoint.h>

#include "FairWindSK.hpp"
#include "ui_Waypoints.h"
#include "Waypoint.hpp"

namespace fairwindsk::ui::mydata {

    Waypoints::Waypoints(MyData *myData, QWidget *parent) :
            QWidget(parent), ui(new Ui::Waypoints) {

        m_myData = myData;

        ui->setupUi(this);

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        int row = 0;
        const auto waypoints = fairWindSK->getSignalKClient()->getWaypoints();
        const auto keys =  waypoints.keys();
        for (const auto& key:keys) {

            signalk::Waypoint waypoint = waypoints.value(key);
            const auto waypointWidget = new mydata::Waypoint(&waypoint);
            ui->gridLayout_Waypoint_Items->addWidget(waypointWidget, row, 0);
            row++;
        }
    }

    Waypoints::~Waypoints() {
        delete ui;
    }
} // fairwindsk::ui::mydata
