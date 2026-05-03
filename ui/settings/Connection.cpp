//
// Created by Raffaele Montella on 06/05/24.
//

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QUuid>
#include <QVBoxLayout>

#include "Connection.hpp"
#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"
#include "ui/MainWindow.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchScrollArea.hpp"

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

        // Walk up the widget hierarchy to find the MainWindow.
        fairwindsk::ui::MainWindow *resolveMainWindow(QWidget *context) {
            QWidget *candidate = context;
            while (candidate) {
                if (auto *mainWindow = qobject_cast<fairwindsk::ui::MainWindow *>(candidate)) {
                    return mainWindow;
                }
                candidate = candidate->parentWidget();
            }
            return fairwindsk::ui::MainWindow::instance(context);
        }

        void applyTextPalette(QWidget *widget, const QColor &color) {
            if (!widget) {
                return;
            }

            QPalette p = widget->palette();
            p.setColor(QPalette::WindowText, color);
            p.setColor(QPalette::Text, color);
            widget->setPalette(p);
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

    // -------------------------------------------------------------------------
    // Helper accessors
    // -------------------------------------------------------------------------

    QUrl Connection::currentSignalKServerUrl() const {
        return validatedSignalKServerUrl(m_comboBox ? m_comboBox->currentText() : QString());
    }

    void Connection::appendMessage(const QString &message) const {
        if (m_consoleEdit) {
            m_consoleEdit->append(message);
        }
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

    // -------------------------------------------------------------------------
    // Status label helpers
    // -------------------------------------------------------------------------

    void Connection::setStatusTexts(const QString &state,
                                    const QString &permission,
                                    const QString &expiration) {
        m_stateText = state;
        m_permissionText = permission;
        m_expirationText = expiration;
        updateStatusLabel();
    }

    void Connection::updateStatusLabel() const {
        if (!m_statusLabel) {
            return;
        }

        QString text;
        if (!m_stateText.isEmpty()) {
            text += tr("State: ") + m_stateText;
        }
        if (!m_permissionText.isEmpty()) {
            if (!text.isEmpty()) {
                text += QStringLiteral("  ·  "); // middle dot separator
            }
            text += tr("Permission: ") + m_permissionText;
        }
        if (!m_expirationText.isEmpty()) {
            if (!text.isEmpty()) {
                text += QStringLiteral("  ·  ");
            }
            text += tr("Expires: ") + m_expirationText;
        }
        m_statusLabel->setText(text);
    }

    // -------------------------------------------------------------------------
    // Browser / console visibility
    // -------------------------------------------------------------------------

    void Connection::showConsole() {
        if (!m_browserDrawerOpen) {
            return;
        }

        // Close the browser drawer; the console is always visible in the layout.
        auto *mainWindow = resolveMainWindow(this);
        if (mainWindow) {
            mainWindow->finishActiveDrawer(0);
            // m_browserDrawerOpen is reset in the onFinished callback.
        }
    }

    void Connection::showBrowserPage(const QUrl &url) {
        auto *mainWindow = resolveMainWindow(this);
        if (!mainWindow) {
            return;
        }

        // If the drawer is already open, just navigate to the new URL.
        if (m_browserDrawerOpen && m_browserView) {
            m_browserView->setUrl(url);
            return;
        }

        // Create a fill-center-area container for the browser.
        auto *container = new QWidget;
        container->setObjectName(QStringLiteral("connectionBrowserContainer"));
        container->setProperty("drawerFillCenterArea", true);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *rootLayout = new QVBoxLayout(container);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        // Toolbar: title + close button.
        auto *toolbar = new QWidget(container);
        auto *toolbarLayout = new QHBoxLayout(toolbar);
        toolbarLayout->setContentsMargins(12, 8, 12, 8);
        toolbarLayout->setSpacing(12);

        auto *titleLabel = new QLabel(tr("Token Request — approve or deny in the browser below"), toolbar);
        titleLabel->setWordWrap(false);
        toolbarLayout->addWidget(titleLabel, 1);

        auto *closeButton = new QPushButton(toolbar);
        closeButton->setObjectName(QStringLiteral("drawerNavButton"));
        closeButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/close-google.svg")));
        closeButton->setIconSize(QSize(24, 24));
        closeButton->setMinimumSize(52, 52);
        closeButton->setMaximumSize(52, 52);
        toolbarLayout->addWidget(closeButton);

        rootLayout->addWidget(toolbar);

        // Embedded browser.
        auto *profile = FairWindSK::getInstance() ? FairWindSK::getInstance()->getWebEngineProfile() : nullptr;
        m_browserView = new fairwindsk::ui::web::WebView(profile, container);
        m_browserView->setUrl(url);
        rootLayout->addWidget(m_browserView, 1);

        m_browserDrawerOpen = true;

        connect(closeButton, &QPushButton::clicked, this, [mainWindow]() {
            mainWindow->finishActiveDrawer(0);
        });

        mainWindow->showDrawerAsync(QString(), container, {}, [this](int) {
            m_browserDrawerOpen = false;
            m_browserView = nullptr;
        });
    }

    // -------------------------------------------------------------------------
    // Token request helpers
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // UI state synchronisation
    // -------------------------------------------------------------------------

    void Connection::syncTokenUiState() {
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        const QString token = settings.value("token", "").toString();
        const QString expirationTime = settings.value("expirationTime", "").toString();

        const bool hasPendingRequest = !href.isEmpty();
        const bool hasToken = !token.isEmpty();
        const bool isBusy = !m_activeTokenReply.isNull();
        const bool isConnectionEnabled = connectionEnabled();

        if (m_comboBox) {
            m_comboBox->setEnabled(!hasPendingRequest && !isBusy);
        }
        if (m_connectButton) {
            m_connectButton->setEnabled(!hasPendingRequest && !isBusy);
        }
        if (m_requestTokenButton) {
            m_requestTokenButton->setEnabled(isConnectionEnabled && !hasPendingRequest && !hasToken && !isBusy);
        }
        if (m_cancelButton) {
            m_cancelButton->setEnabled(hasPendingRequest || isBusy);
        }
        if (m_removeTokenButton) {
            m_removeTokenButton->setEnabled(!hasPendingRequest && hasToken && !isBusy);
        }
        updateConnectionToggle();

        if (hasToken) {
            m_permissionText = tr("Approved");
            m_expirationText = expirationTime;
        } else if (!hasPendingRequest && !isBusy) {
            m_permissionText.clear();
            m_expirationText.clear();
        }
        updateStatusLabel();
    }

    void Connection::applyCompletedAccessRequest(const QJsonObject &accessRequest) {
        const QString permission = accessRequest["permission"].toString();

        clearPendingRequest(permission != "APPROVED");

        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        if (permission == "DENIED") {
            m_permissionText = tr("Denied");
            m_expirationText.clear();
            settings.remove("token");
            settings.remove("expirationTime");
            settings.sync();
        } else if (permission == "APPROVED") {
            m_permissionText = tr("Approved");

            if (accessRequest.contains("expirationTime") && accessRequest["expirationTime"].isString()) {
                m_expirationText = accessRequest["expirationTime"].toString();
                settings.setValue("expirationTime", m_expirationText);
            } else {
                m_expirationText.clear();
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
            m_permissionText = permission;
            m_expirationText.clear();
            settings.remove("token");
            settings.remove("expirationTime");
            settings.sync();
        }

        updateStatusLabel();
    }

    void Connection::finishTokenFlowWithError(const QString &state, const QString &message) {
        stopTokenTimer();
        abortActiveTokenReply();
        clearPendingRequest(false);

        m_stateText = state;
        updateStatusLabel();
        showConsole();

        if (!message.isEmpty()) {
            appendMessage(message);
        }

        syncTokenUiState();
    }

    // -------------------------------------------------------------------------
    // URL / connection management
    // -------------------------------------------------------------------------

    void Connection::commitSignalKServerUrl(const bool restartWhenActive) {
        if (!m_comboBox || !m_settings || m_committingServerUrl) {
            return;
        }

        const QString normalized = normalizedSignalKServerUrlText(m_comboBox->currentText());
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
            const QSignalBlocker blocker(m_comboBox);
            m_comboBox->setCurrentText(signalKServerUrl.toString());
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
        if (!m_comboBox) {
            return;
        }

        const QString normalized = normalizedSignalKServerUrlText(serverUrl);
        if (!validatedSignalKServerUrl(normalized).isValid()) {
            return;
        }

        if (m_comboBox->findText(normalized, Qt::MatchExactly) == -1) {
            const QSignalBlocker blocker(m_comboBox);
            m_comboBox->addItem(normalized);
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
        if (!m_connectButton) {
            return;
        }

        const bool enabled = connectionEnabled();
        m_connectButton->setText(enabled ? tr("Pause") : tr("Connect"));
        m_connectButton->setToolTip(enabled
                                        ? tr("Pause the Signal K connection without clearing the selected URL")
                                        : tr("Connect to the Signal K server using the selected URL"));
        m_connectButton->setAccessibleName(m_connectButton->text());
    }

    // -------------------------------------------------------------------------
    // ZeroConf discovery
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // buildUi — construct the layout programmatically
    // -------------------------------------------------------------------------

    void Connection::buildUi() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        auto *scrollArea = new fairwindsk::ui::widgets::TouchScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        rootLayout->addWidget(scrollArea);

        auto *content = new QWidget(scrollArea);
        scrollArea->setWidget(content);

        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(12, 12, 12, 12);
        contentLayout->setSpacing(12);

        // Title + hint
        m_titleLabel = new QLabel(tr("Signal K Connection"), content);
        contentLayout->addWidget(m_titleLabel);

        m_hintLabel = new QLabel(
            tr("Select or type a Signal K server URL. "
               "Use Connect to start or pause the connection. "
               "Request a token to enable full read/write access — "
               "a browser will open so you can approve the request on the server."),
            content);
        m_hintLabel->setWordWrap(true);
        contentLayout->addWidget(m_hintLabel);

        // --- Control frame ---
        m_controlFrame = new QFrame(content);
        auto *controlLayout = new QVBoxLayout(m_controlFrame);
        controlLayout->setContentsMargins(12, 12, 12, 12);
        controlLayout->setSpacing(10);
        contentLayout->addWidget(m_controlFrame);

        // URL row: label + editable combo
        auto *urlRow = new QHBoxLayout();
        urlRow->setContentsMargins(0, 0, 0, 0);
        urlRow->setSpacing(8);

        m_urlLabel = new QLabel(tr("Server URL"), m_controlFrame);
        urlRow->addWidget(m_urlLabel);

        m_comboBox = new fairwindsk::ui::widgets::TouchComboBox(m_controlFrame);
        m_comboBox->setEditable(true);
        m_comboBox->setMinimumHeight(52);
        urlRow->addWidget(m_comboBox, 1);

        controlLayout->addLayout(urlRow);

        // Status row: single label showing State · Permission · Expires
        m_statusLabel = new QLabel(m_controlFrame);
        m_statusLabel->setWordWrap(false);
        controlLayout->addWidget(m_statusLabel);

        // Action row: Connect | Request Token | Cancel | Remove Token
        auto *actionRow = new QHBoxLayout();
        actionRow->setContentsMargins(0, 0, 0, 0);
        actionRow->setSpacing(8);

        m_connectButton = new QPushButton(tr("Connect"), m_controlFrame);
        m_connectButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg")));
        m_connectButton->setIconSize(QSize(20, 20));
        m_connectButton->setToolTip(tr("Connect to the Signal K server using the selected URL"));
        m_connectButton->setAccessibleName(tr("Toggle Signal K connection"));

        m_requestTokenButton = new QPushButton(tr("Request Token"), m_controlFrame);
        m_requestTokenButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/route-export-iec.svg")));
        m_requestTokenButton->setIconSize(QSize(20, 20));
        m_requestTokenButton->setToolTip(tr("Request a read/write access token from the Signal K server"));

        m_cancelButton = new QPushButton(tr("Cancel"), m_controlFrame);
        m_cancelButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/close-google.svg")));
        m_cancelButton->setIconSize(QSize(20, 20));
        m_cancelButton->setToolTip(tr("Cancel the pending token request"));

        m_removeTokenButton = new QPushButton(tr("Remove Token"), m_controlFrame);
        m_removeTokenButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg")));
        m_removeTokenButton->setIconSize(QSize(20, 20));
        m_removeTokenButton->setToolTip(tr("Remove the stored access token"));

        actionRow->addWidget(m_connectButton);
        actionRow->addWidget(m_requestTokenButton);
        actionRow->addWidget(m_cancelButton);
        actionRow->addWidget(m_removeTokenButton);
        actionRow->addStretch(1);

        controlLayout->addLayout(actionRow);

        // --- Console ---
        m_consoleEdit = new QTextEdit(content);
        m_consoleEdit->setReadOnly(true);
        m_consoleEdit->setMinimumHeight(80);
        m_consoleEdit->setMaximumHeight(120);
        m_consoleEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        contentLayout->addWidget(m_consoleEdit);

        contentLayout->addStretch(1);

        // Signals
        connect(m_connectButton, &QPushButton::clicked, this, &Connection::onToggleConnection);
        connect(m_requestTokenButton, &QPushButton::clicked, this, &Connection::onRequestToken);
        connect(m_cancelButton, &QPushButton::clicked, this, &Connection::onCancelRequest);
        connect(m_removeTokenButton, &QPushButton::clicked, this, &Connection::onRemoveToken);

        // Only commit the URL when the user selects an item from the dropdown.
        // Typed URLs are added to the combo on connect so accidental keystrokes
        // don't overwrite the saved server URL.
        connect(m_comboBox,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Connection::onUpdateSignalKServerUrl);
    }

    // -------------------------------------------------------------------------
    // refreshChrome — apply comfort theme to all chrome elements
    // -------------------------------------------------------------------------

    void Connection::refreshChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration()
                                                : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration)
                                          : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 18.0);
        applyTextPalette(m_hintLabel, chrome.text);
        applyTextPalette(m_urlLabel, chrome.text);
        applyTextPalette(m_statusLabel, chrome.text);

        if (m_controlFrame) {
            m_controlFrame->setStyleSheet(QStringLiteral(
                "QFrame { border: 1px solid %1; border-radius: 14px; background: %2; }")
                                              .arg(chrome.border.name(),
                                                   fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 18).name(QColor::HexArgb)));
        }

        fairwindsk::ui::applyBottomBarPushButtonChrome(m_connectButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_requestTokenButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_cancelButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_removeTokenButton, chrome, false, 52);

        fairwindsk::ui::applyTintedButtonIcon(m_connectButton, chrome.buttonText, QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(m_requestTokenButton, chrome.buttonText, QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(m_cancelButton, chrome.buttonText, QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(m_removeTokenButton, chrome.buttonText, QSize(20, 20));
    }

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    Connection::Connection(Settings *settingsWidget, QWidget *parent)
        : QWidget(parent),
          m_settings(settingsWidget) {

        buildUi();

        m_timer = new QTimer(this);
        m_timer->setInterval(kTokenPollIntervalMs);
        connect(m_timer, &QTimer::timeout, this, &Connection::onCheckRequestToken);

        m_networkAccessManager = new QNetworkAccessManager(this);

        // Populate combo with the configured URL and defaults.
        const QString configuredServerUrl = m_settings->getConfiguration()->getSignalKServerUrl();
        addServerUrlOption(configuredServerUrl);
        {
            const QSignalBlocker blocker(m_comboBox);
            m_comboBox->setCurrentText(configuredServerUrl);
        }
        addServerUrlOption(QStringLiteral("http://localhost:3000"));
        addServerUrlOption(QStringLiteral("https://demo.signalk.org"));

        // Load persisted token/href state.
        const QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QString href = settings.value("href", "").toString();
        const QString expirationTime = settings.value("expirationTime", "").toString();

        if (FairWindSK::getInstance()->isDebug()) {
            qDebug() << "href:" << href
                     << "token:" << settings.value("token", "").toString()
                     << "expirationTime:" << expirationTime;
        }

        if (!expirationTime.isEmpty()) {
            m_expirationText = expirationTime;
        }

        syncTokenUiState();

        if (!href.isEmpty()) {
            m_stateText = tr("Pending...");
            updateStatusLabel();
            startTokenTimer();

            const QUrl signalKServerUrl = currentSignalKServerUrl();
            if (signalKServerUrl.isValid()) {
                showBrowserPage(buildSignalKUrl(signalKServerUrl, "/admin/#/security/access/requests"));
            }
        }

        refreshChrome();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        startZeroConfDiscovery();
#else
        appendMessage(tr("mDNS discovery is disabled on mobile builds."));
#endif
    }

    // -------------------------------------------------------------------------
    // event override — refresh chrome on palette change
    // -------------------------------------------------------------------------

    bool Connection::event(QEvent *event) {
        if (event && (event->type() == QEvent::PaletteChange ||
                      event->type() == QEvent::ApplicationPaletteChange)) {
            refreshChrome();
        }
        return QWidget::event(event);
    }

    // -------------------------------------------------------------------------
    // Slots
    // -------------------------------------------------------------------------

    /*
     * onUpdateSignalKServerUrl
     * Invoked when the user picks an item from the combo dropdown.
     * Saves the selected URL to config without restarting the active connection;
     * typed URLs are committed only when the Connect button is pressed.
     */
    void Connection::onUpdateSignalKServerUrl() {
        commitSignalKServerUrl(false);
    }

    /*
     * addService
     * Invoked when ZeroConf finds a new service.
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
     * onToggleConnection
     * Connects or pauses the Signal K connection.
     * Commits (and adds to the combo) any URL the user typed before toggling.
     */
    void Connection::onToggleConnection() {
        commitSignalKServerUrl(false);
        const bool nextEnabled = !connectionEnabled();
        if (nextEnabled && !currentSignalKServerUrl().isValid()) {
            appendMessage(tr("Please provide a valid Signal K server URL before starting the connection."));
            updateConnectionToggle();
            return;
        }

        setConnectionEnabled(nextEnabled);
        m_stateText = nextEnabled ? tr("Starting") : tr("Paused");
        updateStatusLabel();
        appendMessage(nextEnabled
                          ? tr("Starting Signal K connection.")
                          : tr("Signal K connection paused."));
        syncTokenUiState();
    }

    /*
     * onRequestToken
     * Posts a new access-request to the Signal K server and opens the
     * admin browser so the user can approve it.
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

        m_stateText = tr("Requesting...");
        m_permissionText.clear();
        m_expirationText.clear();
        updateStatusLabel();

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
                m_stateText = tr("Requested/pending");
                updateStatusLabel();
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
            m_stateText = tr("Login required");
            updateStatusLabel();
            syncTokenUiState();

            if (signalKServerUrl.isValid()) {
                showBrowserPage(buildSignalKUrl(signalKServerUrl, "/admin/#/login"));
            }
            return;
        }

        const QString errorText = !networkErrorText.isEmpty() ? networkErrorText : replyMessage(responsePayload, reply);
        finishTokenFlowWithError(tr("Request failed"), errorText);
    }

    /*
     * onCheckRequestToken
     * Invoked by the polling timer each second while a request is pending.
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
            m_stateText = tr("Pending...");
            updateStatusLabel();
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
            m_stateText = tr("Pending...");
            updateStatusLabel();
            syncTokenUiState();
            return;
        }

        if (state != "COMPLETED") {
            m_stateText = state;
            updateStatusLabel();
            syncTokenUiState();
            return;
        }

        m_stateText = tr("Completed");
        updateStatusLabel();
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
     * Aborts any in-flight network request and clears the pending href.
     */
    void Connection::onCancelRequest() {
        abortActiveTokenReply();
        stopTokenTimer();
        clearPendingRequest(true);
        syncTokenUiState();

        showConsole();
        m_stateText = tr("Canceled");
        m_permissionText.clear();
        m_expirationText.clear();
        updateStatusLabel();
    }

    /*
     * onRemoveToken
     * Deletes the stored access token from settings.
     */
    void Connection::onRemoveToken() {
        abortActiveTokenReply();
        stopTokenTimer();
        clearPendingRequest(true);
        syncTokenUiState();

        showConsole();
        m_stateText = tr("Removed");
        m_permissionText.clear();
        m_expirationText.clear();
        updateStatusLabel();
    }

    // -------------------------------------------------------------------------
    // Destructor
    // -------------------------------------------------------------------------

    Connection::~Connection() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        stopZeroConfDiscovery();
#endif
        abortActiveTokenReply();
        stopTokenTimer();
    }

} // fairwindsk::ui::settings
