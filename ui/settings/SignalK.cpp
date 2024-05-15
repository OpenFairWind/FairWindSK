//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_SignalK.h" resolved

#include "SignalK.hpp"
#include "ui_SignalK.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    SignalK::SignalK(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::SignalK) {

        m_settings = settings;

        ui->setupUi(this);

        auto fairWindSK = FairWindSK::getInstance();

        auto signalKJsonObject = fairWindSK->getConfiguration()->getRoot()["signalk"];

        ui->lineEdit_SignalK_POS->setText(QString::fromStdString(signalKJsonObject["pos"].get<std::string>()));
        ui->lineEdit_SignalK_SOG->setText(QString::fromStdString(signalKJsonObject["sog"].get<std::string>()));
        ui->lineEdit_SignalK_COG->setText(QString::fromStdString(signalKJsonObject["cog"].get<std::string>()));
        ui->lineEdit_SignalK_HDG->setText(QString::fromStdString(signalKJsonObject["hdg"].get<std::string>()));
        ui->lineEdit_SignalK_STW->setText(QString::fromStdString(signalKJsonObject["stw"].get<std::string>()));
        ui->lineEdit_SignalK_DPT->setText(QString::fromStdString(signalKJsonObject["dpt"].get<std::string>()));
        ui->lineEdit_SignalK_WPT->setText(QString::fromStdString(signalKJsonObject["wpt"].get<std::string>()));
        ui->lineEdit_SignalK_BTW->setText(QString::fromStdString(signalKJsonObject["btw"].get<std::string>()));
        ui->lineEdit_SignalK_DTG->setText(QString::fromStdString(signalKJsonObject["dtg"].get<std::string>()));
        ui->lineEdit_SignalK_ETA->setText(QString::fromStdString(signalKJsonObject["eta"].get<std::string>()));
        ui->lineEdit_SignalK_TTG->setText(QString::fromStdString(signalKJsonObject["ttg"].get<std::string>()));
        ui->lineEdit_SignalK_XTE->setText(QString::fromStdString(signalKJsonObject["xte"].get<std::string>()));
        ui->lineEdit_SignalK_VMG->setText(QString::fromStdString(signalKJsonObject["vmg"].get<std::string>()));
    }

    SignalK::~SignalK() {
        delete ui;
    }
} // fairwindsk::ui::settings
