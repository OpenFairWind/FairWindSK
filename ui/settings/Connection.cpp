//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Connection.h" resolved

#include <QThread>

#include "Connection.hpp"
#include "ui_Connection.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    Connection::Connection(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Connection) {

        m_settings = settings;

        ui->setupUi(this);

        // Hide the restart button
        ui->pushButton_restart->setVisible(false);

        // Hide the stop button
        ui->pushButton_stop->setVisible(false);

        // Show the connect button
        ui->pushButton_connect->setVisible(true);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_connect, &QPushButton::clicked, this, &Connection::onConnect);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_stop, &QPushButton::clicked, this, &Connection::onStop);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_restart, &QPushButton::clicked, this, &Connection::onRestart);

        connect(&m_zeroConf, &QZeroConf::serviceAdded, this, &Connection::addService);

        m_zeroConf.startBrowser("_http._tcp");

        if (m_zeroConf.browserExists()) {
            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration active.\n"));
        } else {
            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration not active.\n"));
        }
    }

    void Connection::addService(const QZeroConfService& zcs)
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

    void Connection::onConnect() {
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

    void Connection::onStop() {
        m_stop = true;
    }

    void Connection::onRestart() {
        QApplication::exit(-1);
    }

    Connection::~Connection() {
        m_zeroConf.stopBrowser();
        delete ui;
    }
} // fairwindsk::ui::settings
