//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Waypoint.h" resolved

#include "Waypoint.hpp"
#include "ui_Waypoint.h"

namespace fairwindsk::ui::mydata {
Waypoint::Waypoint(signalk::Waypoint *waypoint, QWidget *parent) :
    QWidget(parent), ui(new Ui::Waypoint) {
    ui->setupUi(this);

    m_waypoint = waypoint;

    ui->lineEdit_Name->setText(m_waypoint->getName());
    ui->lineEdit_Description->setText(m_waypoint->getDescription());

    ui->comboBox_Type->setCurrentText(m_waypoint->getType());

    qDebug() << m_waypoint->getTimestamp();


    if (const auto coordinates = m_waypoint->getCoordinates(); !coordinates.isValid()) {
        ui->lineEdit_Latitude->setVisible(false);
        ui->lineEdit_Longitude->setVisible(false);
    } else {

        const auto text = coordinates.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere);

        auto parts = text.split(",");

        ui->lineEdit_Latitude->setText(parts[0]);
        ui->lineEdit_Longitude->setText(parts[1]);
    }



}

Waypoint::~Waypoint() {
    delete ui;
}
} // fairwindsk::ui::mydata
