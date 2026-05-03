//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Connection.h" resolved

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSignalBlocker>
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
            if (!normalized.isEmpty() && !normalized.contains(QStringLiteral("://"))) {
                normalized.prepend(QStringLiteral("http://"));
            }
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

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QStringList signalKDiscoveryTypes() {
            return {
                QStringLiteral("_signalk-http._tcp"),
                QStringLiteral("_signalk-https._tcp"),
                QStringLiteral("_http._tcp"),
                QStringLiteral("_https._tcp")
            };
        }

        QString normalizedServiceType(QString type) {
            type = type.trimmed().toLower();
            while (type.endsWith(QLatin1Char('.'))) {
                type.chop(1);
            }
            if (type.endsWith(QStringLiteral(".local"))) {
                type.chop(QStringLiteral(".local").size());
            }
            return type;
        }

        QString cleanServiceHost(QString host) {
            host = host.trimmed();
            while (host.endsWith(QLatin1Char('.'))) {
                host.chop(1);
            }
            return host;
        }

        bool txtContainsSignalKMarker(const QMap<QByteArray, QByteArray> &txt) {
            for (auto it = txt.cbegin(); it != txt.cend(); ++it) {
                const QString key = QString::fromUtf8(it.key()).toLower();
                const QString value = QString::fromUtf8(it.value()).toLower();
                if (key.contains(QStringLiteral("signalk")) ||
                    key.contains(QStringLiteral("signal-k")) ||
                    value.contains(QStringLiteral("signalk")) ||
                    value.contains(QStringLiteral("signal k")) ||
                    value.contains(QStringLiteral("signal-k"))) {
                    return true;
                }
            }

            return false;
        }
#endif
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
        const bool isConnectionEnabled = connectionEnabled();

        ui->comboBox_signalkserverurl->setEnabled(!hasPendingRequest && !isBusy);
        ui->pushButton_connectionToggle->setEnabled(!hasPendingRequest && !isBusy);
        ui->pushButton_requestToken->setEnabled(isConnectionEnabled && !hasPendingRequest && !hasToken && !isBusy);
        ui->pushButton_cancelRequest->setEnabled(hasPendingRequest || isBusy);
        ui->pushButton_readOnly->setEnabled(!hasPendingRequest && !isBusy);
        ui->pushButton_removeToken->setEnabled(!hasPendingRequest && hasToken && !isBusy);
        updateConnectionToggle();

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

    void Connection::commitSignalKServerUrl(const bool restartWhenActive) {
        if (!ui || !ui->comboBox_signalkserverurl || !m_settings || m_committingServerUrl) {
            return;
        }

        const QString normalized = normalizedSignalKServerUrlText(ui->comboBox_signalkserverurl->currentText());
        if (normalized.isEmpty()) {
            const bool changed = !m_settings->getConfiguration()->getSignalKServerUrl().isEmpty();
            m_settings->getConfiguration()->setSignalKServerUrl(QString());
            if (changed && restartWhenActive) {
                m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);
            }
            updateConnectionToggle();
            return;
        }

        const QUrl signalKServerUrl = validatedSignalKServerUrl(normalized);
        if (!signalKServerUrl.isValid()) {
            updateConnectionToggle();
            return;
        }

        m_committingServerUrl = true;
        addServerUrlOption(signalKServerUrl.toString());
        {
            const QSignalBlocker blocker(ui->comboBox_signalkserverurl);
            ui->comboBox_signalkserverurl->setCurrentText(signalKServerUrl.toString());
        }
        m_committingServerUrl = false;

        const bool changed = m_settings->getConfiguration()->getSignalKServerUrl() != signalKServerUrl.toString();
        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl.toString());
        if (changed && restartWhenActive && connectionEnabled()) {
            m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 400);
        }
        updateConnectionToggle();
    }

    void Connection::addServerUrlOption(const QString &serverUrl) const {
        if (!ui || !ui->comboBox_signalkserverurl) {
            return;
        }

        const QString normalized = normalizedSignalKServerUrlText(serverUrl);
        if (!validatedSignalKServerUrl(normalized).isValid()) {
            return;
        }

        if (ui->comboBox_signalkserverurl->findText(normalized, Qt::MatchExactly) == -1) {
            const QSignalBlocker blocker(ui->comboBox_signalkserverurl);
            ui->comboBox_signalkserverurl->addItem(normalized);
        }
    }

    bool Connection::connectionEnabled() const {
        return m_settings && m_settings->getConfiguration()->getSignalKConnectionEnabled();
    }

    void Connection::setConnectionEnabled(const bool enabled) {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        if (m_settings->getConfiguration()->getSignalKConnectionEnabled() == enabled) {
            updateConnectionToggle();
            return;
        }

        m_settings->getConfiguration()->setSignalKConnectionEnabled(enabled);
        updateConnectionToggle();
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);
    }

    void Connection::updateConnectionToggle() {
        if (!ui || !ui->pushButton_connectionToggle) {
            return;
        }

        const bool enabled = connectionEnabled();
        ui->pushButton_connectionToggle->setText(enabled ? tr("Pause Connection") : tr("Start Connection"));
        ui->pushButton_connectionToggle->setToolTip(enabled
                                                        ? tr("Pause the Signal K connection without clearing the selected URL")
                                                        : tr("Start the Signal K connection using the selected URL"));
        ui->pushButton_connectionToggle->setAccessibleName(ui->pushButton_connectionToggle->text());
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void Connection::startZeroConfDiscovery() {
        stopZeroConfDiscovery();

        QStringList activeTypes;
        for (const QString &serviceType : signalKDiscoveryTypes()) {
            auto *browser = new QZeroConf(this);
            connect(browser, &QZeroConf::serviceAdded, this, &Connection::addService);
            connect(browser, &QZeroConf::serviceUpdated, this, &Connection::addService);
            connect(browser, &QZeroConf::error, this, [this, serviceType](const QZeroConf::error_t) {
                appendMessage(tr("mDNS discovery failed for %1.").arg(serviceType));
            });
            browser->startBrowser(serviceType);
            m_zeroConfBrowsers.append(browser);
            if (browser->browserExists()) {
                activeTypes.append(serviceType);
            }
        }

        appendMessage(activeTypes.isEmpty()
                          ? tr("mDNS discovery starting.")
                          : tr("mDNS discovery active for %1.").arg(activeTypes.join(QStringLiteral(", "))));
    }

    void Connection::stopZeroConfDiscovery() {
        for (auto *browser : m_zeroConfBrowsers) {
            if (!browser) {
                continue;
            }
            browser->stopBrowser();
            browser->deleteLater();
        }
        m_zeroConfBrowsers.clear();
    }

    bool Connection::isSignalKDiscoveryService(const QZeroConfService &item) const {
        if (!item) {
            return false;
        }

        const QString type = normalizedServiceType(item->type());
        if (type == QStringLiteral("_signalk-http._tcp") || type == QStringLiteral("_signalk-https._tcp")) {
            return true;
        }

        if (type != QStringLiteral("_http._tcp") && type != QStringLiteral("_https._tcp")) {
            return false;
        }

        const QMap<QByteArray, QByteArray> txt = item->txt();
        const QString name = item->name().toLower();
        if (name.contains(QStringLiteral("signalk")) || name.contains(QStringLiteral("signal k"))) {
            return true;
        }

        const bool hasSignalKTxtShape = txt.contains("roles") && (txt.contains("self") || txt.contains("swname"));
        return hasSignalKTxtShape || txtContainsSignalKMarker(txt);
    }

    QString Connection::signalKUrlForService(const QZeroConfService &item) const {
        if (!isSignalKDiscoveryService(item)) {
            return {};
        }

        const QString type = normalizedServiceType(item->type());
        const QString scheme = type.contains(QStringLiteral("https")) ? QStringLiteral("https") : QStringLiteral("http");
        QString host = item->ip().isNull() ? QString() : item->ip().toString();
        if (host.isEmpty()) {
            host = cleanServiceHost(item->host());
        }
        if (host.isEmpty() || item->port() == 0) {
            return {};
        }

        QUrl url;
        url.setScheme(scheme);
        url.setHost(host);
        url.setPort(item->port());
        return normalizedSignalKServerUrlText(url.toString());
    }
#endif


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
        const QString configuredServerUrl = m_settings->getConfiguration()->getSignalKServerUrl();
        addServerUrlOption(configuredServerUrl);
        ui->comboBox_signalkserverurl->setCurrentText(configuredServerUrl);

        addServerUrlOption(QStringLiteral("http://localhost:3000"));
        addServerUrlOption(QStringLiteral("https://demo.signalk.org"));

        for (auto *button : {
                 ui->pushButton_connectionToggle,
                 ui->pushButton_requestToken,
                 ui->pushButton_cancelRequest,
                 ui->pushButton_readOnly,
                 ui->pushButton_removeToken
             }) {
            button->setMinimumHeight(52);
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
        connect(ui->pushButton_connectionToggle, &QPushButton::clicked, this, &Connection::onToggleConnection);

        connect(ui->comboBox_signalkserverurl,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Connection::onUpdateSignalKServerUrl);
        connect(ui->comboBox_signalkserverurl,
                &fairwindsk::ui::widgets::TouchComboBox::editTextChanged,
                this,
                &Connection::onUpdateSignalKServerUrl);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        startZeroConfDiscovery();
#else
        appendMessage(tr("mDNS discovery is disabled on mobile builds."));
#endif
    }

    /*
     * onUpdateSignalKServerUrl
     * Invoked when the signal k server url has changed
     */
    void Connection::onUpdateSignalKServerUrl() {
        commitSignalKServerUrl(true);
    }

    /*
    * addService
    * Invoked when zero conf find a new service
    */
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void Connection::addService(const QZeroConfService &item) {
        const QString signalKServerUrl = signalKUrlForService(item);
        if (signalKServerUrl.isEmpty()) {
            return;
        }

        addServerUrlOption(signalKServerUrl);
        appendMessage(tr("Discovered Signal K %1 at %2.").arg(item->name(), signalKServerUrl));
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

        commitSignalKServerUrl(false);
        const QUrl signalKServerUrl = currentSignalKServerUrl();
        if (!signalKServerUrl.isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL."));
            return;
        }

        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl.toString());
        m_settings->getConfiguration()->setSignalKConnectionEnabled(true);
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);
        updateConnectionToggle();

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
        commitSignalKServerUrl(false);
        const QUrl signalKServerUrl = currentSignalKServerUrl();
        if (!signalKServerUrl.isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL."));
            return;
        }

        m_settings->getConfiguration()->setSignalKServerUrl(signalKServerUrl.toString());
        m_settings->getConfiguration()->setSignalKConnectionEnabled(true);
        m_settings->markDirty(FairWindSK::RuntimeSignalKConnection, 0);
        updateConnectionToggle();

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

    void Connection::onToggleConnection() {
        commitSignalKServerUrl(false);
        const bool nextEnabled = !connectionEnabled();
        if (nextEnabled && !currentSignalKServerUrl().isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL before starting the connection."));
            updateConnectionToggle();
            return;
        }

        setConnectionEnabled(nextEnabled);
        showConsole();
        ui->label_lblState->setText(nextEnabled ? tr("Starting") : tr("Paused"));
        appendMessage(nextEnabled
                          ? tr("Starting Signal K connection.")
                          : tr("Signal K connection paused."));
        syncTokenUiState();
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
        stopZeroConfDiscovery();
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
