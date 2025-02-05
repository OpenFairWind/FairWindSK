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

    /*
     * Connection
     * Class constructor
     */
    Connection::Connection(Settings *settingsWidget, QWidget *parent) :
            QWidget(parent), ui(new Ui::Connection) {

	    // Set the settings widge
        m_settings = settingsWidget;

	    // Set the UI
        ui->setupUi(this);

	    // Set the timer pointer
	    m_timer = nullptr;

        // Set the current signal k server url
        ui->comboBox_signalkserverurl->setCurrentText(m_settings->getConfiguration()->getSignalKServerUrl());

        // Get the web engine profile
        const auto profile = FairWindSK::getInstance()->getWebEngineProfile();

        // Create a web page
        m_page = new QWebEnginePage(profile);

        // Set the page of the web engine view
        ui->webEngineView->setPage(m_page);

        // Hide the web engine view
        ui->webEngineView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Initialize the QT managed settings
        const QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        const auto href = settings.value("href", "").toString();

        // Get the name of the FairWind++ configuration file
        const auto token = settings.value("token", "").toString();

        // Get the name of the FairWind++ configuration file
        const auto expirationTime = settings.value("expirationTime", "").toString();

        qDebug() << "href: " << href << " token: " << token << " expirationTime: " << expirationTime;

        // Check if the token is empty
        if (token.isEmpty()) {
            // The token is empty

            // Enable the request token button
            ui->pushButton_requestToken->setEnabled(true);

            // Disable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(false);

            // Disable the remove token button
            ui->pushButton_removeToken->setEnabled(false);

        } else {
            // The token is present

            // Disable the request token button
            ui->pushButton_requestToken->setEnabled(false);

            // Enable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(false);

            // Enable the remove token button
            ui->pushButton_removeToken->setEnabled(true);
        }

        // Check if the href is not empty
        if (!href.isEmpty()) {
            // The href is not empty

            // Disable the request token button
            ui->pushButton_requestToken->setEnabled(false);

            // Enable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(true);

            // Disable the remove token button
            ui->pushButton_removeToken->setEnabled(false);

            // Create a timer
            m_timer = new QTimer(this);

            // Connect the timeout to onCheckRequestToken
            connect(m_timer, SIGNAL(timeout()), this, SLOT(onCheckRequestToken()));

            // Start the timer
            m_timer->start(1000);

            // Set the url of the web page as the signal k server url
            ui->webEngineView->setUrl(ui->comboBox_signalkserverurl->currentText());

            // Make the web page visible
            ui->webEngineView->setVisible(true);

            // Hide the console
            ui->textEdit_message->setVisible(false);
        }

        // Check if th expiration time is not empty
        if (!expirationTime.isEmpty()) {
            // The expiration time is not empty

            // Disable the request token button
            ui->pushButton_requestToken->setEnabled(false);

            // Disable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(false);

            // Enable the remove token button
            ui->pushButton_removeToken->setEnabled(true);
        }

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_requestToken, &QPushButton::clicked, this, &Connection::onRequestToken);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_cancelRequest, &QPushButton::clicked, this, &Connection::onCancelRequest);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_removeToken, &QPushButton::clicked, this, &Connection::onRemoveToken);

        // Connect the signal k server url changed to the onUpdateSignalKServerUrl method
        connect(ui->comboBox_signalkserverurl, &QComboBox::currentIndexChanged, this, &Connection::onUpdateSignalKServerUrl);
        connect(ui->comboBox_signalkserverurl, &QComboBox::editTextChanged, this, &Connection::onUpdateSignalKServerUrl);

        // Connect the zero conf serviceAdded siggnal to the addService method
        connect(&m_zeroConf, &QZeroConf::serviceAdded, this, &Connection::addService);

        // Start the zero conf browser
        m_zeroConf.startBrowser("_http._tcp");

        // Check if the browser exists
        if (m_zeroConf.browserExists()) {

            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration active.\n"));
        } else {

            // Show a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Zero configuration not active.\n"));
        }
    }

    /*
     * onUpdateSignalKServerUrl
     * Invoked when the signal k server url has changed
     */
    void Connection::onUpdateSignalKServerUrl() const {

        // Set the signal k server url
        m_settings->getConfiguration()->setSignalKServerUrl(ui->comboBox_signalkserverurl->currentText());
    }

    /*
    * addService
    * Invoked when zero conf find a new service
    */
    void Connection::addService(const QZeroConfService& item) const {

        // Show a message on the console
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Added service\n"));
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Name: ") + item->name() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Domain: ") + item->domain() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Host: ") + item->host() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Type: ") + item->type() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Ip: ") + item->ip().toString() + "\n");
        ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Port: ") + QString("%1").arg(item->port()) + "\n\n");

        // Get the type of protocol
        const auto type = item->type().split(".")[0].replace("_","");

        // Get the host
        const auto host = item->ip().toString();

        // Get the signal k server url
        const auto signalKServerUrl = QString("%1://%2:%3").arg(type, host).arg(item->port());

        // Add the signal k server url to the combo box
        ui->comboBox_signalkserverurl->addItem(signalKServerUrl);

    }

    /*
    * onRequestToken
    * Invoked when the button request token is hit
    */
    void Connection::onRequestToken() {

        // Get the signal k server url
        const auto signalKServerUrl = ui->comboBox_signalkserverurl->currentText();

        // Set the URL for the application list
        const auto url = QUrl(signalKServerUrl + "/signalk/v1/access/requests");

        // Create the network access manager
        QNetworkAccessManager networkAccessManager;

        // Create the network request
        auto networkRequest = QNetworkRequest(url);

        // Set the content type header as application/json
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader," application/json");

        // Create the client id string
        auto clientId = QUuid::createUuid().toString().replace("{","").replace("}","");

        // Create a message in json
        const auto message = QString("{\n"
                          "  \"clientId\": \"%1\",\n"
                          "  \"description\": \"%2\""
                          "}").arg(clientId, "FairWindSK");

        // Get the message as byte array
        const QByteArray data = message.toUtf8();

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
            // The response has been successful

            // Get the json document
            const auto doc = QJsonDocument::fromJson(reply->readAll());

            // Get the json object
            const auto jsonObject = doc.object();

            // Check if the json object contains a state item as string
            if (jsonObject.contains("state") && jsonObject["state"].isString()) {

                // Get the state as a string
                const auto state = jsonObject["state"].toString();

                // Check if the state is pending
                if ( state == "PENDING") {

                    // Set the state as requested/pending
                    ui->label_lblState->setText(tr("Requested/pending"));

                    // Check if the json object contains a href item as string
                    if (jsonObject.contains("href") && jsonObject["href"].isString()) {

                        // Get the href as a string
                        const auto href = jsonObject["href"].toString();

                        // Initialize the QT managed settings
                        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

                        // Store the configuration in the settings
                        settings.setValue("href", href);

                        // Create a timer
                        m_timer = new QTimer(this);

                        // Connect the timer timeout to the onCheckRequestToken member function
                        connect(m_timer, SIGNAL(timeout()), this, SLOT(onCheckRequestToken()));

                        // Start the timer (check each second)
                        m_timer->start(1000);

                        // Disable the request token button
                        ui->pushButton_requestToken->setEnabled(false);

                        // Enable the cancel request button
                        ui->pushButton_cancelRequest->setEnabled(true);

                        // Disable the remove token button
                        ui->pushButton_removeToken->setEnabled(false);

                        // Set the web page url to the signal k server url for requests management
                        ui->webEngineView->setUrl(signalKServerUrl+"/admin/#/security/access/requests");

                        // Show the web page
                        ui->webEngineView->setVisible(true);

                        // Hide the console
                        ui->textEdit_message->setVisible(false);

                        // Check if debug mode
                        if (FairWindSK::getInstance()->isDebug()) {

                            // Show a message
                            qDebug() << "Token requested, href: " << href;
                        }
                    }
                } else {
                    // If the state is different then PENDING, show the state
                    ui->label_lblState->setText(state);
                }
            }
        }
        // Check if the response failed due to a 400 error
        else if(reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ) == 400) {

            // Enable the request token button
            ui->pushButton_requestToken->setEnabled(true);

            // Disable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(false);

            // Disable the remove token button
            ui->pushButton_removeToken->setEnabled(false);

            // Set the web page url as the signal k server login url
            ui->webEngineView->setUrl(signalKServerUrl+"/admin/#/login");

            // Set the web page visible
            ui->webEngineView->setVisible(true);

            // Hide the console
            ui->textEdit_message->setVisible(false);
        } else {
            // For any other reason of failure

            // Hide the web page
            ui->webEngineView->setVisible(false);

            // Show the console
            ui->textEdit_message->setVisible(true);

            // Write a message
            ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Error connecting the server.\n"));
        }
    }

    /*
    * onCheckRequestToken
    * Invoked by a timer each second
    */
    void Connection::onCheckRequestToken() {

        // Get a pointer to the FairWindSK instance
        auto fairWinSK = FairWindSK::getInstance();

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Get the name of the FairWind++ configuration file
        const auto href = settings.value("href", "").toString();

        // Check if the href value is valied
        if (!href.isEmpty()) {

            // Get the signal k server url
            const auto signalKServerUrl = ui->comboBox_signalkserverurl->currentText();

            // Set the URL for the request token application
            const auto url = QUrl(signalKServerUrl + href);

            // Create the network access manager
            QNetworkAccessManager networkAccessManager;

            // Create a network request
            auto networkRequest = QNetworkRequest(url);

            // Set the Content Type handler to application/json
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

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

                // Get the result as a json document
                const auto doc = QJsonDocument::fromJson(reply->readAll());

                // Get the content as a json object
                const auto jsonObject = doc.object();

                // Check if the json object as a state and the state is a string
                if (jsonObject.contains("state") && jsonObject["state"].isString()) {

                    // Get the stat as a string
                    const auto state = jsonObject["state"].toString();

                    // Check if the state is PENDING
                    if (state == "PENDING") {

                        // Set the message state as PENDING
                        ui->label_lblState->setText(tr("Pending..."));

                        // Check if the state is COMPLETETD
                    } else if (state == "COMPLETED") {

                        // Set the message state as COMPLETED
                        ui->label_lblState->setText(tr("Completed"));

                        // Stop the timer
                        m_timer->stop();

                        // Disconnect the timer from this member function
                        m_timer->disconnect(this);

                        // Delete the timer
                        delete m_timer;

                        // Set the timer pointer to null
                        m_timer = nullptr;

                        // Hide the web page
                        ui->webEngineView->setVisible(false);

                        // Show the console message
                        ui->textEdit_message->setVisible(true);

                        // Check if statusCode field exist and it is a double
                        if (jsonObject.contains("statusCode") && jsonObject["statusCode"].isDouble()) {

                            // Get the status code as an integer
                            const auto statusCode = jsonObject["statusCode"].toInt();

                            // Check if the status code il different by 200 (it is not fully ok)
                            if (statusCode != 200) {

                                // Check if the message field exists and if it is a string
                                if (jsonObject.contains("message") && jsonObject["message"].isString()) {

                                    // Get the message
                                    const auto message = jsonObject["message"].toString();

                                    // Set the console message
                                    ui->textEdit_message->setText(ui->textEdit_message->toPlainText()+ tr("Message: ") + message + "\n");
                                }
                            }
                            else {
                                // The status code is ok

                                // CHeck if the result contains accessRequest and accassRequest is an object
                                if (jsonObject.contains("accessRequest") && jsonObject["accessRequest"].isObject()) {

                                    // Get the accessRequest as a json object
                                    const auto accessRequestJsonObject = jsonObject["accessRequest"].toObject();

                                    // Check if the json request contains the permissions field and the permission is a string
                                    if (accessRequestJsonObject.contains("permission") &&
                                        accessRequestJsonObject["permission"].isString()) {

                                        // Get the permission as a string
                                        auto permission = accessRequestJsonObject["permission"].toString();

                                        // Remove the href field from the settings
                                        settings.remove("href");

                                        // Check if the permission is DENIED
                                        if (permission == "DENIED") {

                                            // Set the message in the permission field
                                            ui->label_lblPermission->setText(tr("Denied"));

                                            // Check if the permission is APPROVED
                                        } else if (permission == "APPROVED") {

                                            // Set the message in the permission field
                                            ui->label_lblPermission->setText(tr("Approved"));

                                            // Check if the json request contains the expirationTime field and the expirationTime is a string
                                            if (accessRequestJsonObject.contains("expirationTime") &&
                                                accessRequestJsonObject["expirationTime"].isString()) {

                                                // Get the expirationTime as a string
                                                auto expirationTime = accessRequestJsonObject["expirationTime"].toString();

                                                ui->label_lblExpirationTime->setText(expirationTime);

                                                // Store the configuration in the settings
                                                settings.setValue("expirationTime", expirationTime);

                                            }

                                            // Check if the json request contains the token field and the token is a string
                                            if (accessRequestJsonObject.contains("token") &&
                                                accessRequestJsonObject["token"].isString()) {

                                                // Get the token as a string
                                                const auto token = accessRequestJsonObject["token"].toString();

                                                // Store the configuration in the settings
                                                settings.setValue("token", token);
                                            }

                                            // Hide the web page
                                            ui->webEngineView->setVisible(false);

                                        } else {

                                            // Set the message in the permission field
                                            ui->label_lblPermission->setText(permission);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // Set the message in the state field
                        ui->label_lblState->setText(state);
                    }
                }
            }
        }
    }


    /*
    * onCancelRequest
    * Invoked when the cancel request button has hit
    */
    void Connection::onCancelRequest() {

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Check if the time has been allocated
        if (m_timer) {

            // Stop the timer
            m_timer->stop();

            // Disconnect the time
            m_timer->disconnect(this);

            // Delete the timer
            delete m_timer;

            // Set the timer pointer to null
            m_timer = nullptr;
        }

        // Remove href
        settings.remove("href");

        // Enable the request token button
        ui->pushButton_requestToken->setEnabled(true);

        // Disable the cancel request button
        ui->pushButton_cancelRequest->setEnabled(false);

        // Disabke the remove token button
        ui->pushButton_removeToken->setEnabled(false);

        // Hide the web page
        ui->webEngineView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Set the state as CANCELED
        ui->label_lblState->setText(tr("Canceled"));
    }

    /*
    * onRemoveToken
    * Invoked when the remove token button has hit
    */
    void Connection::onRemoveToken() {

        // Initialize the QT managed settings
        QSettings settings("fairwindsk.ini", QSettings::NativeFormat);

        // Store the configuration in the settings
        settings.setValue("token", "");

        // Store the configuration in the settings
        settings.setValue("expirationTime", "");

        // Enable the request token button
        ui->pushButton_requestToken->setEnabled(true);

        // Disable the cancel request button
        ui->pushButton_cancelRequest->setEnabled(false);

        // Disable the remove token button
        ui->pushButton_removeToken->setEnabled(false);

        // Hide the web page
        ui->webEngineView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Set the expiration time as removed
        ui->label_lblExpirationTime->setText(tr("Removed"));
    }

    /*
     * ~Connection
     * Class destructor
     */
    Connection::~Connection() {

	    // Stop the zeroconf browser
        m_zeroConf.stopBrowser();

	    // Check if the timer is instanced
        if (m_timer) {

	        // Stop the timer
            m_timer->stop();

	        // Disconnect the timer
            m_timer->disconnect(this);

	        // Delete the timer
            delete m_timer;

	        // Ser the timer to null
	        m_timer = nullptr;
        }

        // Check if the page has been allocated
        if (m_page) {

            // Delete the page
            delete m_page;

            // Set the pointer to null
            m_page = nullptr;
        }

        // Check if the UI is valid
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the ui pointer to null
            ui = nullptr;
        }
    }

} // fairwindsk::ui::settings
