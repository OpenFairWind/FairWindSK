//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_SignalK.h" resolved

#include "SignalK.hpp"
#include "ui_SignalK.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    SignalK::SignalK(QWidget *parent) :
            QWidget(parent), ui(new Ui::SignalK) {
        ui->setupUi(this);

        auto fairWindSK = FairWindSK::getInstance();

        auto signalKJsonObject = fairWindSK->getConfiguration()["signalk"].toObject();
        ui->lineEdit_SignalK_POS->setText(signalKJsonObject["pos"].toString());
        ui->lineEdit_SignalK_SOG->setText(signalKJsonObject["sog"].toString());
        ui->lineEdit_SignalK_COG->setText(signalKJsonObject["cog"].toString());
        ui->lineEdit_SignalK_HDG->setText(signalKJsonObject["hdg"].toString());
        ui->lineEdit_SignalK_STW->setText(signalKJsonObject["stw"].toString());
        ui->lineEdit_SignalK_DPT->setText(signalKJsonObject["dpt"].toString());
        ui->lineEdit_SignalK_WPT->setText(signalKJsonObject["wpt"].toString());
        ui->lineEdit_SignalK_BTW->setText(signalKJsonObject["btw"].toString());
        ui->lineEdit_SignalK_DTG->setText(signalKJsonObject["dtg"].toString());
        ui->lineEdit_SignalK_ETA->setText(signalKJsonObject["eta"].toString());
        ui->lineEdit_SignalK_TTG->setText(signalKJsonObject["ttg"].toString());
        ui->lineEdit_SignalK_XTE->setText(signalKJsonObject["xte"].toString());
        ui->lineEdit_SignalK_VMG->setText(signalKJsonObject["vmg"].toString());
    }

    SignalK::~SignalK() {
        delete ui;
    }
} // fairwindsk::ui::settings
