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
#include <QEventLoop>
#include <QPointer>
#include "Connection.hpp"
#include "ui_Connection.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr int kBlockingRequestTimeoutMs = 5000;

        QByteArray executeBlockingRequest(QNetworkReply *reply, int *statusCode = nullptr) {
            if (statusCode) {
                *statusCode = -1;
            }
            if (!reply) {
                return {};
            }

            QEventLoop loop;
            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);

            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

            timeoutTimer.start(kBlockingRequestTimeoutMs);
            loop.exec();

            if (!reply->isFinished()) {
                reply->abort();
            }

            if (statusCode) {
                *statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            }

            const QByteArray payload = reply->readAll();
            reply->deleteLater();
            return payload;
        }
    }

    void Connection::appendMessage(const QString &message) const {
        ui->textEdit_message->append(message);
    }

    void Connection::stopTokenTimer() {
        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }
    }

    void Connection::syncTokenUiState() {
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        const QString token = settings.value("token", "").toString();
        const QString expirationTime = settings.value("expirationTime", "").toString();

        const bool hasPendingRequest = !href.isEmpty();
        const bool hasToken = !token.isEmpty();

        ui->pushButton_requestToken->setEnabled(!hasPendingRequest && !hasToken);
        ui->pushButton_cancelRequest->setEnabled(hasPendingRequest);
        ui->pushButton_readOnly->setEnabled(!hasPendingRequest);
        ui->pushButton_removeToken->setEnabled(!hasPendingRequest && hasToken);

        if (hasToken) {
            ui->label_lblPermission->setText(tr("Approved"));
            ui->label_lblExpirationTime->setText(expirationTime);
        } else if (!hasPendingRequest) {
            ui->label_lblPermission->clear();
            ui->label_lblExpirationTime->clear();
        }
    }


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

        if (ui->comboBox_signalkserverurl->findText("https://demo.signalk.org") == -1) {
            ui->comboBox_signalkserverurl->addItem("https://demo.signalk.org");
        }

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
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        // Get the name of the FairWind++ configuration file
        const auto href = settings.value("href", "").toString();

        // Get the name of the FairWind++ configuration file
        const auto token = settings.value("token", "").toString();

        // Get the name of the FairWind++ configuration file
        const auto expirationTime = settings.value("expirationTime", "").toString();

        if (FairWindSK::getInstance()->isDebug()) {
            qDebug() << "href:" << href << "token:" << token << "expirationTime:" << expirationTime;
        }

        syncTokenUiState();

        // Check if the href is not empty
        if (!href.isEmpty()) {
            // The href is not empty

            // Disable the request token button
            ui->pushButton_requestToken->setEnabled(false);

            // Enable the cancel request button
            ui->pushButton_cancelRequest->setEnabled(true);

            // Disable the remove token button
            ui->pushButton_removeToken->setEnabled(false);

            stopTokenTimer();
            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, &Connection::onCheckRequestToken);
            m_timer->start(1000);

            // Set the url of the web page as the signal k server url
            ui->webEngineView->setUrl(QUrl(ui->comboBox_signalkserverurl->currentText()));

            // Make the web page visible
            ui->webEngineView->setVisible(true);

            // Hide the console
            ui->textEdit_message->setVisible(false);
        }

        if (!expirationTime.isEmpty()) {
            ui->label_lblExpirationTime->setText(expirationTime);
        }

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_requestToken, &QPushButton::clicked, this, &Connection::onRequestToken);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_cancelRequest, &QPushButton::clicked, this, &Connection::onCancelRequest);

        // Set the current Signal K server URL in read-only mode
        connect(ui->pushButton_readOnly, &QPushButton::clicked, this, &Connection::onReadOnly);

        // Show the settings view when the user clicks on the Settings button inside the BottomBar object
        connect(ui->pushButton_removeToken, &QPushButton::clicked, this, &Connection::onRemoveToken);

        // Connect the signal k server url changed to the onUpdateSignalKServerUrl method
        connect(ui->comboBox_signalkserverurl, &QComboBox::currentIndexChanged, this, &Connection::onUpdateSignalKServerUrl);
        connect(ui->comboBox_signalkserverurl, &QComboBox::editTextChanged, this, &Connection::onUpdateSignalKServerUrl);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        // Connect the zero conf serviceAdded signal to the addService method
        connect(&m_zeroConf, &QZeroConf::serviceAdded, this, &Connection::addService);

        // Start the zero conf browser
        m_zeroConf.startBrowser("_http._tcp");

        // Check if the browser exists
        if (m_zeroConf.browserExists()) {
            appendMessage(tr("Zero configuration active."));
        } else {
            appendMessage(tr("Zero configuration not active."));
        }
#else
        appendMessage(tr("Zero configuration discovery is disabled on mobile builds."));
#endif
    }

    /*
     * onUpdateSignalKServerUrl
     * Invoked when the signal k server url has changed
     */
    void Connection::onUpdateSignalKServerUrl() const {

        // Set the signal k server url
        m_settings->getConfiguration()->setSignalKServerUrl(ui->comboBox_signalkserverurl->currentText());
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 400);
    }

    /*
    * addService
    * Invoked when zero conf find a new service
    */
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void Connection::addService(const QZeroConfService& item) const {

        // Show a message on the console
        appendMessage(tr("Added service"));
        appendMessage(tr("Name: %1").arg(item->name()));
        appendMessage(tr("Domain: %1").arg(item->domain()));
        appendMessage(tr("Host: %1").arg(item->host()));
        appendMessage(tr("Type: %1").arg(item->type()));
        appendMessage(tr("Ip: %1").arg(item->ip().toString()));
        appendMessage(tr("Port: %1").arg(item->port()));

        // Get the type of protocol
        const auto type = item->type().split(".")[0].replace("_","");

        // Get the host
        const auto host = item->ip().toString();

        // Get the signal k server url
        const auto signalKServerUrl = QString("%1://%2:%3").arg(type, host).arg(item->port());

        // Add the signal k server url to the combo box
        if (ui->comboBox_signalkserverurl->findText(signalKServerUrl) == -1) {
            ui->comboBox_signalkserverurl->addItem(signalKServerUrl);
        }

    }
#endif

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
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
        networkRequest.setTransferTimeout(kBlockingRequestTimeoutMs);

        // Create the client id string
        auto clientId = QUuid::createUuid().toString().replace("{","").replace("}","");

        // Create a message in json
        const auto message = QString("{\n"
                          "  \"clientId\": \"%1\",\n"
                          "  \"description\": \"%2\""
                          "}").arg(clientId, "FairWindSK");

        // Get the message as byte array
        const QByteArray data = message.toUtf8();

        // Create the get request
        QNetworkReply *reply = networkAccessManager.post(networkRequest,data);

        int statusCode = -1;
        const QByteArray responsePayload = executeBlockingRequest(reply, &statusCode);

        // Check if the response has been successful
        if (statusCode == 202) {
            // The response has been successful

            // Get the json document
            const auto doc = QJsonDocument::fromJson(responsePayload);

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
                        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

                        // Store the configuration in the settings
                        settings.setValue("href", href);
                        settings.sync();

                        stopTokenTimer();
                        m_timer = new QTimer(this);
                        connect(m_timer, &QTimer::timeout, this, &Connection::onCheckRequestToken);
                        m_timer->start(1000);

                        // Disable the request token button
                        ui->pushButton_requestToken->setEnabled(false);

                        // Enable the cancel request button
                        ui->pushButton_cancelRequest->setEnabled(true);

                        // Disable the remove token button
                        ui->pushButton_removeToken->setEnabled(false);

                        // Set the web page url to the signal k server url for requests management
                        ui->webEngineView->setUrl(QUrl(signalKServerUrl+"/admin/#/security/access/requests"));

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
            ui->webEngineView->setUrl(QUrl(signalKServerUrl+"/admin/#/login"));

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
            appendMessage(tr("Error connecting the server."));
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
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

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
            networkRequest.setTransferTimeout(kBlockingRequestTimeoutMs);

            // Create the get request
            QNetworkReply *reply = networkAccessManager.get(networkRequest);

            int statusCode = -1;
            const QByteArray responsePayload = executeBlockingRequest(reply, &statusCode);

            // Check if the response has been successful
            if (statusCode == 200) {

                // Get the result as a json document
                const auto doc = QJsonDocument::fromJson(responsePayload);

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

                        stopTokenTimer();

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
                                    appendMessage(tr("Message: %1").arg(message));
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
                                            ui->label_lblExpirationTime->clear();

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

                                            settings.sync();

                                            // Hide the web page
                                            ui->webEngineView->setVisible(false);

                                        } else {

                                            // Set the message in the permission field
                                            ui->label_lblPermission->setText(permission);
                                            settings.sync();
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

            reply->deleteLater();
        }
    }


    /*
    * onCancelRequest
    * Invoked when the cancel request button has hit
    */
    void Connection::onCancelRequest() {

        // Initialize the QT managed settings
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        stopTokenTimer();

        // Remove href
        settings.remove("href");
        settings.remove("expirationTime");
        settings.sync();

        syncTokenUiState();

        // Hide the web page
        ui->webEngineView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Set the state as CANCELED
        ui->label_lblState->setText(tr("Canceled"));
        ui->label_lblPermission->clear();
        ui->label_lblExpirationTime->clear();
    }

    /*
    * onReadOnly
    * Invoked when the read only button has hit
    */
    void Connection::onReadOnly() {

        const QString signalKServerUrl = ui->comboBox_signalkserverurl->currentText().trimmed();

        if (signalKServerUrl.isEmpty()) {
            appendMessage(tr("Please provide a Signal K server URL."));
            return;
        }

        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl);
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);

        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        settings.remove("href");
        settings.remove("token");
        settings.remove("expirationTime");
        settings.sync();

        stopTokenTimer();
        syncTokenUiState();

        ui->webEngineView->setVisible(false);
        ui->textEdit_message->setVisible(true);
        ui->label_lblState->setText(tr("Read only"));
        ui->label_lblPermission->setText(tr("No token"));
        ui->label_lblExpirationTime->clear();

        appendMessage(tr("Configured %1 in read-only mode.").arg(signalKServerUrl));
    }

    /*
    * onRemoveToken
    * Invoked when the remove token button has hit
    */
    void Connection::onRemoveToken() {

        // Initialize the QT managed settings
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        // Store the configuration in the settings
        settings.setValue("token", "");

        // Store the configuration in the settings
        settings.setValue("expirationTime", "");
        settings.remove("href");
        settings.sync();

        stopTokenTimer();
        syncTokenUiState();

        // Hide the web page
        ui->webEngineView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Set the expiration time as removed
        ui->label_lblExpirationTime->setText(tr("Removed"));
        ui->label_lblPermission->clear();
        ui->label_lblState->setText(tr("Removed"));
    }

    /*
     * ~Connection
     * Class destructor
     */
    Connection::~Connection() {

	    // Stop the zeroconf browser
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        m_zeroConf.stopBrowser();
#endif

        stopTokenTimer();

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
