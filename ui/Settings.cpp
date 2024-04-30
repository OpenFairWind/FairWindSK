//
// Created by Raffaele Montella on 18/03/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Settings.h" resolved

#include <QThread>
#include <QTranslator>
#include "Settings.hpp"
#include "ui_Settings.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui {

    Settings::Settings(QWidget *parent) :
            QWidget(parent), ui(new Ui::Settings) {
        ui->setupUi(this);

        auto fairWindSK = FairWindSK::getInstance();

        // Order by order value
        QMap<int, QPair<AppItem *, QString>> map;

        // Populate the inverted list
        for (auto &hash : fairWindSK->getAppsHashes()) {
            // Get the hash value
            auto app = fairWindSK->getAppItemByHash(hash);
            auto position = app->getOrder();

            map[position] = QPair<AppItem *, QString>(app, hash);
        }

        // Iterate on the available apps' hash values
        for (const auto& item: map) {
            // Get the hash value
            auto appItem = item.first;
            auto name = item.second;
            auto listWidgetItem = new QListWidgetItem(appItem->getDisplayName());
            listWidgetItem->setIcon(QIcon(appItem->getIcon()));
            listWidgetItem->setToolTip(appItem->getDescription());
            listWidgetItem->setData(Qt::UserRole, name);
            if (appItem->getActive()) {
                listWidgetItem->setCheckState(Qt::Checked);
            } else {
                listWidgetItem->setCheckState(Qt::Unchecked);
            }
            ui->listWidget_Apps_List->addItem(listWidgetItem);
        }

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

        // Hide the restart button
        ui->pushButton_restart->setVisible(false);

        // Hide the stop button
        ui->pushButton_stop->setVisible(false);

        // Show the connect button
        ui->pushButton_connect->setVisible(true);

        if (fairwindsk::FairWindSK::getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->checkBox_virtualkeboard, &QCheckBox::stateChanged, this, &Settings::onVirtualKeyboard);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_connect, &QPushButton::clicked, this, &Settings::onConnect);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_stop, &QPushButton::clicked, this, &Settings::onStop);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_restart, &QPushButton::clicked, this, &Settings::onRestart);

        connect(&m_zeroConf, &QZeroConf::serviceAdded, this, &Settings::addService);

        m_zeroConf.startBrowser("_http._tcp");

        if (m_zeroConf.browserExists()) {
            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration active.\n"));
        } else {
            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration not active.\n"));
        }
    }

    void Settings::onVirtualKeyboard(int state) {

        auto fairWindSK = FairWindSK::getInstance();

        if (state == Qt::Checked) {
            fairwindsk::FairWindSK::setVirtualKeyboard(true);
        } else {
            fairwindsk::FairWindSK::setVirtualKeyboard(false);
        }
    }

    void Settings::addService(const QZeroConfService& zcs)
    {

        // Show a message
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Added service\n"));
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Name: ") + zcs->name() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Domain: ") + zcs->domain() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Host: ") + zcs->host() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Type: ") + zcs->type() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Ip: ") + zcs->ip().toString() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Port: ") + QString("%1").arg(zcs->port()) + "\n\n");

        ui->comboBox_signalkserverurl->addItem(zcs->type().split(".")[0].replace("_","")+"://" + zcs->ip().toString() + ":" + QString("%1").arg(zcs->port()));

    }

    void Settings::onConnect() {
        auto fairWinSK = FairWindSK::getInstance();

        auto signalKServerUrl = ui->comboBox_signalkserverurl->currentText();
        auto username = ui->lineEdit_username->text();
        auto password = ui->lineEdit_password->text();

        // ui->textEdit_message->setText("");

        ui->comboBox_signalkserverurl->setEnabled(false);
        ui->lineEdit_username->setEnabled(false);
        ui->lineEdit_password->setEnabled(false);


        // Define the parameters map
        QMap<QString, QVariant> params;

        // Set some defaults
        params["active"] = true;


        // Setup the debug mode
        params["debug"] = fairWinSK->isDebug();


        // Check if the Signal K Server URL is empty
        if (signalKServerUrl.isEmpty()) {

            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("The Signal K Server URL is missing\n"));

        } else {

            // Set the url
            params["url"] = ui->comboBox_signalkserverurl->currentText() + "/signalk";
        }

        // Check if the user name is empty
        if (username.isEmpty()) {

            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Username is missing\n"));
        } else {

            // Set the username
            params["username"] = ui->lineEdit_username->text();
        }

        // Check if the password is empty
        if (password.isEmpty()) {

            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("The password is missing\n"));

        } else {

            // Set the url
            params["password"] = ui->lineEdit_password->text();
        }

        // Check if the token is defined or if username/password are defined
        if (params.contains("url") && params.contains("username") && params.contains("password")) {

            // The result
            bool result = false;

            // Number of connection tentatives
            int count = 0;

            // Set the stop flag
            m_stop = false;

            // Hide the connect button
            ui->pushButton_connect->setVisible(false);

            // Show the stop button
            ui->pushButton_stop->setVisible(true);

            // Start the connection
            do {

                auto message = QString(
                        tr("Trying to connect to the ") + signalKServerUrl + tr(" Signal K server") + "(%1/%2)... \n"
                        ).arg(count+1).arg(fairWinSK->getRetry());


                // Show the message
                ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ message);

                // Try to connect
                result = fairWinSK->getSignalKClient()->init(params);

                // Check if the connection is successful
                if (result) {

                    // Show a message
                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText() + tr("\nConnected and authenticated!\n"));

                    // Set the Signal K Server URL
                    fairWinSK->setSignalKServerUrl(signalKServerUrl);

                    // Set the token
                    fairWinSK->setToken(fairWinSK->getSignalKClient()->getToken());

                    // Get all the document
                    auto allSignalK = QJsonDocument(fairWinSK->getSignalKClient()->getAll());

                    // Show the Signal K document
                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ "\n" + allSignalK.toJson() + "\n");

                    // Hide the stop button
                    ui->pushButton_stop->setVisible(false);

                    // Show the restart button
                    ui->pushButton_restart->setVisible(true);

                    // Exit the loop
                    break;
                }

                // Process the events
                QApplication::processEvents();

                // Increase the number of retry
                count++;

                // Wait for m_mSleep microseconds
                QThread::msleep(fairWinSK->getSleep());

                // Loop until the number of retry or the stop button is pressed
            } while (count < fairWinSK->getRetry() && !m_stop);

            // Check if no connection was successful
            if (!result) {

                // Check if the connection has been stopped
                if (m_stop) {
                    // Show a message
                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText() + tr("\nConnection stopped.\n"));
                } else {
                    // Show a message
                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText() + tr("\nConnection failed!\n"));
                }

                // Enable the Signal K Server URL widget
                ui->comboBox_signalkserverurl->setEnabled(true);

                // Enable the username widget
                ui->lineEdit_username->setEnabled(true);

                // Enable the password widget
                ui->lineEdit_password->setEnabled(true);

                // Hide the stop button
                ui->pushButton_stop->setVisible(false);

                // Show the connect button
                ui->pushButton_connect->setVisible(true);
            }
        }
    }

    void Settings::onStop() {
        m_stop = true;
    }

    void Settings::onRestart() {
        QApplication::exit(-1);
    }
    Settings::~Settings() {
        m_zeroConf.stopBrowser();
        delete ui;
    }



} // fairwindsk::ui
