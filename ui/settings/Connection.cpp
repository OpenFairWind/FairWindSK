//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Connection.h" resolved

#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QWebEngineSettings>
#include <QJsonDocument>
#include <QSettings>
#include "Connection.hpp"
#include "ui_Connection.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    Connection::Connection(Settings *settingsWidget, QWidget *parent) :
            QWidget(parent), ui(new Ui::Connection) {

        m_settings = settingsWidget;

        ui->setupUi(this);

        // Set the current server
        ui->comboBox_signalkserverurl->setCurrentText(m_settings->getConfiguration()->getSignalKServerUrl());

        auto page = new QWebEnginePage(FairWindSK::getInstance()->getWebEngineProfile());
        ui->webEngineView->setPage(page);

        // Show the connect button
        ui->webEngineView->setVisible(false);
        ui->textEdit_message->setVisible(true);

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        auto href = settings.value("href", "").toString();

        // Get the name of the FairWind++ configuration file
        auto token = settings.value("token", "").toString();

        // Get the name of the FairWind++ configuration file
        auto expirationTime = settings.value("expirationTime", "").toString();

        qDebug() << "href: " << href << " token: " << token << " expirationTime: " << expirationTime;


        if (token.isEmpty()) {
            ui->pushButton_requestToken->setEnabled(true);
            ui->pushButton_cancelRequest->setEnabled(false);
            ui->pushButton_removeToken->setEnabled(false);
        } else {
            ui->pushButton_requestToken->setEnabled(false);
            ui->pushButton_cancelRequest->setEnabled(false);
            ui->pushButton_removeToken->setEnabled(true);
        }

        if (!href.isEmpty()) {
            ui->pushButton_requestToken->setEnabled(false);
            ui->pushButton_cancelRequest->setEnabled(true);
            ui->pushButton_removeToken->setEnabled(false);

            m_timer = new QTimer(this);
            connect(m_timer, SIGNAL(timeout()), this, SLOT(onCheckRequestToken()));
            m_timer->start(1000);

            ui->webEngineView->setUrl(ui->comboBox_signalkserverurl->currentText());
            ui->webEngineView->setVisible(true);
            ui->textEdit_message->setVisible(false);
        }

        if (!expirationTime.isEmpty()) {
            ui->pushButton_requestToken->setEnabled(false);
            ui->pushButton_cancelRequest->setEnabled(false);
            ui->pushButton_removeToken->setEnabled(true);
        }

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_requestToken, &QPushButton::clicked, this, &Connection::onRequestToken);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_cancelRequest, &QPushButton::clicked, this, &Connection::onCancelRequest);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_removeToken, &QPushButton::clicked, this, &Connection::onRemoveToken);


        //
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

    void Connection::onRequestToken() {
        qDebug() << "onRequestToken";

        auto signalKServerUrl = ui->comboBox_signalkserverurl->currentText();

        // Set the URL for the application list
        QUrl url = QUrl(signalKServerUrl + "/signalk/v1/access/requests");

        // Create the network access manager
        QNetworkAccessManager networkAccessManager;

        auto networkRequest = QNetworkRequest(url);
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader," application/json");

        QByteArray data = R"({"clientId":"1234-45653-343453","description":"FairWindSK"})";

        // Create the event loop component
        QEventLoop loop;

        // Connect the network manager to the event loop
        connect(&networkAccessManager, &QNetworkAccessManager::finished, &loop,&QEventLoop::quit);

        // Create the get request
        QNetworkReply *reply = networkAccessManager.post(networkRequest,data);

        // Wait until the request is satisfied
        loop.exec();

        // Check if the response has been successful
        if (reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 202) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto jsonObject = doc.object();
            if (jsonObject.contains("state") && jsonObject["state"].isString()) {
                auto state = jsonObject["state"].toString();
                if ( state == "PENDING") {

                    ui->label_lblState->setText(tr("Requested"));

                    if (jsonObject.contains("href") && jsonObject["href"].isString()) {
                        auto href = jsonObject["href"].toString();

                        // Initialize the QT managed settings
                        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

                        // Store the configuration in the settings
                        settings.setValue("href", href);



                        m_timer = new QTimer(this);
                        connect(m_timer, SIGNAL(timeout()), this, SLOT(onCheckRequestToken()));
                        m_timer->start(1000);

                        ui->pushButton_requestToken->setEnabled(false);
                        ui->pushButton_cancelRequest->setEnabled(true);
                        ui->pushButton_removeToken->setEnabled(false);
                        ui->webEngineView->setUrl(signalKServerUrl+"/admin/#/security/access/requests");
                        ui->webEngineView->setVisible(true);
                        ui->textEdit_message->setVisible(false);

                        qDebug() << "Token requested, href: " << href;
                    }
                } else {
                    ui->label_lblState->setText(state);
                }
            }
        }
        else if(reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 400) {
            ui->pushButton_requestToken->setEnabled(true);
            ui->pushButton_cancelRequest->setEnabled(false);
            ui->pushButton_removeToken->setEnabled(false);
            ui->webEngineView->setUrl(signalKServerUrl+"/admin/#/login");
            ui->webEngineView->setVisible(true);
            ui->textEdit_message->setVisible(false);
        } else {
            ui->webEngineView->setVisible(false);
            ui->textEdit_message->setVisible(true);
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Error connecting the server.\n"));
        }
    }

    void Connection::onCheckRequestToken() {

        auto fairWinSK = FairWindSK::getInstance();

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        auto href = settings.value("href", "").toString();

        if (!href.isEmpty()) {

            auto signalKServerUrl = ui->comboBox_signalkserverurl->currentText();

            // Set the URL for the application list
            QUrl url = QUrl(signalKServerUrl + href);

            // Create the network access manager
            QNetworkAccessManager networkAccessManager;

            auto networkRequest = QNetworkRequest(url);
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, " application/json");

            // Create the event loop component
            QEventLoop loop;

            // Connect the network manager to the event loop
            connect(&networkAccessManager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

            // Create the get request
            QNetworkReply *reply = networkAccessManager.get(networkRequest);

            // Wait until the request is satisfied
            loop.exec();

            // Check if the response has been successful
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {

                auto doc = QJsonDocument::fromJson(reply->readAll());
                auto jsonObject = doc.object();
                if (jsonObject.contains("state") && jsonObject["state"].isString()) {
                    auto state = jsonObject["state"].toString();

                    if (state == "PENDING") {
                        ui->label_lblState->setText(tr("Pending..."));

                    } else if (state == "COMPLETED") {

                        ui->label_lblState->setText(tr("Completed"));

                        m_timer->stop();
                        m_timer->disconnect(this);
                        delete m_timer;
                        m_timer = nullptr;

                        ui->webEngineView->setVisible(false);
                        ui->textEdit_message->setVisible(true);

                        if (jsonObject.contains("statusCode") && jsonObject["statusCode"].isDouble()) {

                            auto statusCode = jsonObject["statusCode"].toInt();

                            if (statusCode != 200) {
                                if (jsonObject.contains("message") && jsonObject["message"].isString()) {
                                    auto message = jsonObject["message"].toString();

                                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Message: ") + message + "\n");
                                }
                            }
                            else {
                                if (jsonObject.contains("accessRequest") && jsonObject["accessRequest"].isObject()) {
                                    auto accessRequestJsonObject = jsonObject["accessRequest"].toObject();
                                    if (accessRequestJsonObject.contains("permission") &&
                                        accessRequestJsonObject["permission"].isString()) {
                                        auto permission = accessRequestJsonObject["permission"].toString();

                                        // Store the configuration in the settings
                                        settings.setValue("href", "");

                                        if (permission == "DENIED") {

                                            ui->label_lblPermission->setText(tr("Denied"));

                                        } else if (permission == "APPROVED") {

                                            ui->label_lblPermission->setText(tr("Approved"));

                                            if (accessRequestJsonObject.contains("expirationTime") &&
                                                accessRequestJsonObject["expirationTime"].isString()) {

                                                auto expirationTime = accessRequestJsonObject["expirationTime"].toString();

                                                ui->label_lblExpirationTime->setText(expirationTime);

                                                // Store the configuration in the settings
                                                settings.setValue("expirationTime", expirationTime);

                                            }

                                            if (accessRequestJsonObject.contains("token") &&
                                                accessRequestJsonObject["token"].isString()) {

                                                auto token = accessRequestJsonObject["token"].toString();

                                                // Store the configuration in the settings
                                                settings.setValue("token", token);
                                            }

                                            ui->webEngineView->setVisible(false);

                                        } else {
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                    }
                }
            }
        }
    }



    void Connection::onCancelRequest() {

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        if (m_timer) {
            m_timer->stop();
            m_timer->disconnect(this);
            delete m_timer;
        }

        // Store the configuration in the settings
        settings.setValue("href", "");

        ui->pushButton_requestToken->setEnabled(true);
        ui->pushButton_cancelRequest->setEnabled(false);
        ui->pushButton_removeToken->setEnabled(false);

        ui->webEngineView->setVisible(false);
        ui->textEdit_message->setVisible(true);

        ui->label_lblState->setText(tr("Canceled"));
    }

    void Connection::onRemoveToken() {
        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Store the configuration in the settings
        settings.setValue("token", "");

        // Store the configuration in the settings
        settings.setValue("expirationTime", "");

        ui->pushButton_requestToken->setEnabled(true);
        ui->pushButton_cancelRequest->setEnabled(false);
        ui->pushButton_removeToken->setEnabled(false);

        ui->webEngineView->setVisible(false);
        ui->textEdit_message->setVisible(true);

        ui->label_lblExpirationTime->setText(tr("Removed"));
    }

    Connection::~Connection() {
        m_zeroConf.stopBrowser();
        delete ui;
        if (m_timer) {
            m_timer->stop();
            m_timer->disconnect(this);
            delete m_timer;
        }
    }

} // fairwindsk::ui::settings
