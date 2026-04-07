//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Connection.h" resolved

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUuid>
#include <QVBoxLayout>

#include "Connection.hpp"
#include "ui_Connection.h"
#include "FairWindSK.hpp"
#include "ui/widgets/TouchComboBox.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr int kTokenRequestTimeoutMs = 5000;
        constexpr int kTokenPollIntervalMs = 1000;

        QString normalizedSignalKServerUrlText(const QString &rawText) {
            QString normalized = rawText.trimmed();
            while (normalized.endsWith('/')) {
                normalized.chop(1);
            }
            return normalized;
        }

        QUrl validatedSignalKServerUrl(const QString &rawText) {
            const QString normalized = normalizedSignalKServerUrlText(rawText);
            if (normalized.isEmpty()) {
                return {};
            }

            const QUrl url(normalized);
            if (!url.isValid() || url.host().isEmpty()) {
                return {};
            }

            const QString scheme = url.scheme().toLower();
            if (scheme != "http" && scheme != "https") {
                return {};
            }

            return url;
        }

        QUrl buildSignalKUrl(const QUrl &baseUrl, const QString &path) {
            if (!baseUrl.isValid()) {
                return {};
            }

            return QUrl(normalizedSignalKServerUrlText(baseUrl.toString()) + path);
        }

        QString replyMessage(const QByteArray &payload, QNetworkReply *reply) {
            const QJsonDocument document = QJsonDocument::fromJson(payload);
            if (document.isObject()) {
                const QJsonObject object = document.object();
                if (object.contains("message") && object["message"].isString()) {
                    return object["message"].toString();
                }
                if (object.contains("error") && object["error"].isString()) {
                    return object["error"].toString();
                }
            }

            if (reply && !reply->errorString().isEmpty()) {
                return reply->errorString();
            }

            return QObject::tr("Error connecting the server.");
        }
    }

    QUrl Connection::currentSignalKServerUrl() const {
        return validatedSignalKServerUrl(ui->comboBox_signalkserverurl->currentText());
    }

    void Connection::appendMessage(const QString &message) const {
        ui->textEdit_message->append(message);
    }

    void Connection::stopTokenTimer() {
        if (m_timer) {
            m_timer->stop();
        }
    }

    void Connection::startTokenTimer() {
        if (m_timer) {
            m_timer->start(kTokenPollIntervalMs);
        }
    }

    void Connection::showConsole() const {
        if (m_browserView) {
            m_browserView->setVisible(false);
        }
        ui->textEdit_message->setVisible(true);
    }

    void Connection::showBrowserPage(const QUrl &url) const {
        if (m_browserView) {
            m_browserView->setUrl(url);
            m_browserView->setVisible(true);
        }
        ui->textEdit_message->setVisible(false);
    }

    void Connection::abortActiveTokenReply() {
        if (!m_activeTokenReply) {
            return;
        }

        disconnect(m_activeTokenReply, nullptr, this, nullptr);
        m_activeTokenReply->abort();
        m_activeTokenReply->deleteLater();
        m_activeTokenReply = nullptr;
    }

    void Connection::setPendingRequestHref(const QString &href) const {
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        settings.setValue("href", href);
        settings.setValue("token", "");
        settings.setValue("expirationTime", "");
        settings.sync();
    }

    void Connection::clearPendingRequest(const bool clearToken) const {
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        settings.remove("href");
        if (clearToken) {
            settings.remove("token");
            settings.remove("expirationTime");
        }
        settings.sync();
    }

    void Connection::syncTokenUiState() {
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        const QString token = settings.value("token", "").toString();
        const QString expirationTime = settings.value("expirationTime", "").toString();

        const bool hasPendingRequest = !href.isEmpty();
        const bool hasToken = !token.isEmpty();
        const bool isBusy = !m_activeTokenReply.isNull();

        ui->comboBox_signalkserverurl->setEnabled(!hasPendingRequest && !isBusy);
        ui->pushButton_requestToken->setEnabled(!hasPendingRequest && !hasToken && !isBusy);
        ui->pushButton_cancelRequest->setEnabled(hasPendingRequest || isBusy);
        ui->pushButton_readOnly->setEnabled(!hasPendingRequest && !isBusy);
        ui->pushButton_removeToken->setEnabled(!hasPendingRequest && hasToken && !isBusy);

        if (hasToken) {
            ui->label_lblPermission->setText(tr("Approved"));
            ui->label_lblExpirationTime->setText(expirationTime);
        } else if (!hasPendingRequest && !isBusy) {
            ui->label_lblPermission->clear();
            ui->label_lblExpirationTime->clear();
        }
    }

    void Connection::applyCompletedAccessRequest(const QJsonObject &accessRequest) {
        const QString permission = accessRequest["permission"].toString();

        clearPendingRequest(permission != "APPROVED");

        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        if (permission == "DENIED") {
            ui->label_lblPermission->setText(tr("Denied"));
            ui->label_lblExpirationTime->clear();
            settings.remove("token");
            settings.remove("expirationTime");
            settings.sync();
        } else if (permission == "APPROVED") {
            ui->label_lblPermission->setText(tr("Approved"));

            if (accessRequest.contains("expirationTime") && accessRequest["expirationTime"].isString()) {
                const QString expirationTime = accessRequest["expirationTime"].toString();
                ui->label_lblExpirationTime->setText(expirationTime);
                settings.setValue("expirationTime", expirationTime);
            } else {
                ui->label_lblExpirationTime->clear();
                settings.remove("expirationTime");
            }

            if (accessRequest.contains("token") && accessRequest["token"].isString()) {
                settings.setValue("token", accessRequest["token"].toString());
            } else {
                settings.remove("token");
            }

            settings.sync();
            m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);
        } else {
            ui->label_lblPermission->setText(permission);
            ui->label_lblExpirationTime->clear();
            settings.remove("token");
            settings.remove("expirationTime");
            settings.sync();
        }
    }

    void Connection::finishTokenFlowWithError(const QString &state, const QString &message) {
        stopTokenTimer();
        abortActiveTokenReply();
        clearPendingRequest(false);

        ui->label_lblState->setText(state);
        showConsole();

        if (!message.isEmpty()) {
            appendMessage(message);
        }

        syncTokenUiState();
    }


    /*
     * Connection
     * Class constructor
     */
    Connection::Connection(Settings *settingsWidget, QWidget *parent)
        : QWidget(parent),
          ui(new Ui::Connection) {

        // Set the settings widge
        m_settings = settingsWidget;

        // Set the UI
        ui->setupUi(this);

        m_timer = new QTimer(this);
        m_timer->setInterval(kTokenPollIntervalMs);
        connect(m_timer, &QTimer::timeout, this, &Connection::onCheckRequestToken);

        m_networkAccessManager = new QNetworkAccessManager(this);

        // Set the current signal k server url
        ui->comboBox_signalkserverurl->setCurrentText(m_settings->getConfiguration()->getSignalKServerUrl());

        if (ui->comboBox_signalkserverurl->findText("https://demo.signalk.org") == -1) {
            ui->comboBox_signalkserverurl->addItem("https://demo.signalk.org");
        }

        auto *browserLayout = new QVBoxLayout(ui->widgetBrowserHost);
        browserLayout->setContentsMargins(0, 0, 0, 0);
        browserLayout->setSpacing(0);
        m_browserView = new fairwindsk::ui::web::WebView(FairWindSK::getInstance()->getWebEngineProfile(),
                                                         ui->widgetBrowserHost);
        browserLayout->addWidget(m_browserView);
        m_browserView->setVisible(false);

        // Show the console
        ui->textEdit_message->setVisible(true);

        // Initialize the QT managed settings
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        const QString expirationTime = settings.value("expirationTime", "").toString();

        if (FairWindSK::getInstance()->isDebug()) {
            qDebug() << "href:" << href
                     << "token:" << settings.value("token", "").toString()
                     << "expirationTime:" << expirationTime;
        }

        syncTokenUiState();

        if (!href.isEmpty()) {
            ui->label_lblState->setText(tr("Pending..."));
            startTokenTimer();

            const QUrl signalKServerUrl = currentSignalKServerUrl();
            if (signalKServerUrl.isValid()) {
                showBrowserPage(buildSignalKUrl(signalKServerUrl, "/admin/#/security/access/requests"));
            }
        }

        if (!expirationTime.isEmpty()) {
            ui->label_lblExpirationTime->setText(expirationTime);
        }

        connect(ui->pushButton_requestToken, &QPushButton::clicked, this, &Connection::onRequestToken);
        connect(ui->pushButton_cancelRequest, &QPushButton::clicked, this, &Connection::onCancelRequest);
        connect(ui->pushButton_readOnly, &QPushButton::clicked, this, &Connection::onReadOnly);
        connect(ui->pushButton_removeToken, &QPushButton::clicked, this, &Connection::onRemoveToken);

        connect(ui->comboBox_signalkserverurl,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Connection::onUpdateSignalKServerUrl);
        connect(ui->comboBox_signalkserverurl,
                &fairwindsk::ui::widgets::TouchComboBox::editTextChanged,
                this,
                &Connection::onUpdateSignalKServerUrl);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        connect(&m_zeroConf, &QZeroConf::serviceAdded, this, &Connection::addService);
        m_zeroConf.startBrowser("_http._tcp");

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
        m_settings->getConfiguration()->setSignalKServerUrl(normalizedSignalKServerUrlText(ui->comboBox_signalkserverurl->currentText()));
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 400);
    }

    /*
    * addService
    * Invoked when zero conf find a new service
    */
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void Connection::addService(const QZeroConfService &item) const {

        // Show a message on the console
        appendMessage(tr("Added service"));
        appendMessage(tr("Name: %1").arg(item->name()));
        appendMessage(tr("Domain: %1").arg(item->domain()));
        appendMessage(tr("Host: %1").arg(item->host()));
        appendMessage(tr("Type: %1").arg(item->type()));
        appendMessage(tr("Ip: %1").arg(item->ip().toString()));
        appendMessage(tr("Port: %1").arg(item->port()));

        // Get the type of protocol
        const auto type = item->type().split(".")[0].replace("_", "");

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
        if (m_activeTokenReply) {
            return;
        }

        const QUrl signalKServerUrl = currentSignalKServerUrl();
        if (!signalKServerUrl.isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL."));
            return;
        }

        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl.toString());
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);

        clearPendingRequest(true);
        stopTokenTimer();

        ui->label_lblState->setText(tr("Requesting..."));
        ui->label_lblPermission->clear();
        ui->label_lblExpirationTime->clear();
        showConsole();

        QNetworkRequest networkRequest(buildSignalKUrl(signalKServerUrl, "/signalk/v1/access/requests"));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        networkRequest.setTransferTimeout(kTokenRequestTimeoutMs);

        const QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QJsonObject requestObject{
            {"clientId", clientId},
            {"description", "FairWindSK"}
        };

        m_activeTokenReply = m_networkAccessManager->post(networkRequest, QJsonDocument(requestObject).toJson(QJsonDocument::Compact));
        connect(m_activeTokenReply, &QNetworkReply::finished, this, &Connection::handleTokenRequestReply);
        syncTokenUiState();
    }

    void Connection::handleTokenRequestReply() {
        auto *reply = qobject_cast<QNetworkReply *>(sender());
        if (!reply) {
            return;
        }

        const QByteArray responsePayload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto networkError = reply->error();
        const QString networkErrorText = reply->errorString();

        if (m_activeTokenReply == reply) {
            m_activeTokenReply = nullptr;
        }
        reply->deleteLater();

        if (networkError == QNetworkReply::OperationCanceledError) {
            syncTokenUiState();
            return;
        }

        const QUrl signalKServerUrl = currentSignalKServerUrl();

        if (statusCode == 202) {
            const QJsonDocument document = QJsonDocument::fromJson(responsePayload);
            const QJsonObject jsonObject = document.object();
            const QString state = jsonObject["state"].toString();
            const QString href = jsonObject["href"].toString();

            if (state == "PENDING" && !href.isEmpty()) {
                setPendingRequestHref(href);
                ui->label_lblState->setText(tr("Requested/pending"));
                startTokenTimer();
                syncTokenUiState();

                if (signalKServerUrl.isValid()) {
                    showBrowserPage(buildSignalKUrl(signalKServerUrl, "/admin/#/security/access/requests"));
                }

                if (FairWindSK::getInstance()->isDebug()) {
                    qDebug() << "Token requested, href:" << href;
                }
                return;
            }

            finishTokenFlowWithError(tr("Request failed"), replyMessage(responsePayload, reply));
            return;
        }

        if (statusCode == 400) {
            ui->label_lblState->setText(tr("Login required"));
            syncTokenUiState();

            if (signalKServerUrl.isValid()) {
                showBrowserPage(buildSignalKUrl(signalKServerUrl, "/admin/#/login"));
            } else {
                showConsole();
            }
            return;
        }

        const QString errorText = !networkErrorText.isEmpty() ? networkErrorText : replyMessage(responsePayload, reply);
        finishTokenFlowWithError(tr("Request failed"), errorText);
    }

    /*
    * onCheckRequestToken
    * Invoked by a timer each second
    */
    void Connection::onCheckRequestToken() {
        if (m_activeTokenReply) {
            return;
        }

        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        if (href.isEmpty()) {
            stopTokenTimer();
            syncTokenUiState();
            return;
        }

        const QUrl signalKServerUrl = currentSignalKServerUrl();
        if (!signalKServerUrl.isValid()) {
            finishTokenFlowWithError(tr("Invalid URL"), tr("Please provide a valid Signal K server URL."));
            return;
        }

        QNetworkRequest networkRequest(buildSignalKUrl(signalKServerUrl, href));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        networkRequest.setTransferTimeout(kTokenRequestTimeoutMs);

        m_activeTokenReply = m_networkAccessManager->get(networkRequest);
        connect(m_activeTokenReply, &QNetworkReply::finished, this, &Connection::handleTokenStatusReply);
        syncTokenUiState();
    }

    void Connection::handleTokenStatusReply() {
        auto *reply = qobject_cast<QNetworkReply *>(sender());
        if (!reply) {
            return;
        }

        const QByteArray responsePayload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto networkError = reply->error();

        if (m_activeTokenReply == reply) {
            m_activeTokenReply = nullptr;
        }
        reply->deleteLater();

        if (networkError == QNetworkReply::OperationCanceledError) {
            syncTokenUiState();
            return;
        }

        if (networkError != QNetworkReply::NoError && statusCode <= 0) {
            ui->label_lblState->setText(tr("Pending..."));
            syncTokenUiState();
            return;
        }

        if (statusCode != 200) {
            syncTokenUiState();
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(responsePayload);
        const QJsonObject jsonObject = document.object();
        const QString state = jsonObject["state"].toString();

        if (state == "PENDING") {
            ui->label_lblState->setText(tr("Pending..."));
            syncTokenUiState();
            return;
        }

        if (state != "COMPLETED") {
            ui->label_lblState->setText(state);
            syncTokenUiState();
            return;
        }

        ui->label_lblState->setText(tr("Completed"));
        stopTokenTimer();
        showConsole();

        const int requestStatusCode = jsonObject["statusCode"].toInt(-1);
        if (requestStatusCode != 200) {
            finishTokenFlowWithError(tr("Completed"), replyMessage(responsePayload, nullptr));
            return;
        }

        if (!jsonObject.contains("accessRequest") || !jsonObject["accessRequest"].isObject()) {
            finishTokenFlowWithError(tr("Completed"), tr("The access request response is incomplete."));
            return;
        }

        applyCompletedAccessRequest(jsonObject["accessRequest"].toObject());
        syncTokenUiState();
    }

    /*
    * onCancelRequest
    * Invoked when the cancel request button has hit
    */
    void Connection::onCancelRequest() {
        abortActiveTokenReply();
        stopTokenTimer();
        clearPendingRequest(true);
        syncTokenUiState();

        showConsole();
        ui->label_lblState->setText(tr("Canceled"));
        ui->label_lblPermission->clear();
        ui->label_lblExpirationTime->clear();
    }

    /*
    * onReadOnly
    * Invoked when the read only button has hit
    */
    void Connection::onReadOnly() {
        const QUrl signalKServerUrl = currentSignalKServerUrl();
        if (!signalKServerUrl.isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL."));
            return;
        }

        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl.toString());
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);

        abortActiveTokenReply();
        stopTokenTimer();
        clearPendingRequest(true);
        syncTokenUiState();

        showConsole();
        ui->label_lblState->setText(tr("Read only"));
        ui->label_lblPermission->setText(tr("No token"));
        ui->label_lblExpirationTime->clear();

        appendMessage(tr("Configured %1 in read-only mode.").arg(signalKServerUrl.toString()));
    }

    /*
    * onRemoveToken
    * Invoked when the remove token button has hit
    */
    void Connection::onRemoveToken() {
        abortActiveTokenReply();
        stopTokenTimer();
        clearPendingRequest(true);
        syncTokenUiState();

        showConsole();
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

        abortActiveTokenReply();
        stopTokenTimer();

        // Check if the UI is valid
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the ui pointer to null
            ui = nullptr;
        }
    }

} // fairwindsk::ui::settings
