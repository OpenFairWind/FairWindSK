//
// Created by Raffaele Montella on 28/03/21.
//

#include "WebView.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QtGlobal>

#include "Localization.hpp"
#include "ui/IconUtils.hpp"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QAuthenticator>
#include <QMessageBox>
#include <QTimer>
#include <QStyle>
#include <QWebEngineHistory>
#include <QWebEngineView>
#include <QWebEngineCertificateError>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QWebEnginePermission>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <QWebEngineFileSystemAccessRequest>
#endif
#include <QWebEngineRegisterProtocolHandlerRequest>

#include "WebPage.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui_CertificateErrorDialog.h"
#include "ui_PasswordDialog.h"
#else
#include <QQuickItem>
#include <QQuickWidget>
#include <QQmlContext>
#include <QMetaObject>
#include <QVariant>
#endif

namespace fairwindsk::ui::web {
    namespace {
        constexpr int kHostedAppLoadTimeoutMs = 15000;
        constexpr int kHostedAppAuthLoopWindowMs = 15000;
        constexpr int kHostedAppAuthLoopThreshold = 3;
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        QString questionForFeature(const QWebEnginePage::Feature feature) {
            switch (feature) {
                case QWebEnginePage::Geolocation:
                    return QObject::tr("Allow %1 to access your location information?");
                case QWebEnginePage::MediaAudioCapture:
                    return QObject::tr("Allow %1 to access your microphone?");
                case QWebEnginePage::MediaVideoCapture:
                    return QObject::tr("Allow %1 to access your webcam?");
                case QWebEnginePage::MediaAudioVideoCapture:
                    return QObject::tr("Allow %1 to access your microphone and webcam?");
                case QWebEnginePage::MouseLock:
                    return QObject::tr("Allow %1 to lock your mouse cursor?");
                case QWebEnginePage::DesktopVideoCapture:
                    return QObject::tr("Allow %1 to capture video of your desktop?");
                case QWebEnginePage::DesktopAudioVideoCapture:
                    return QObject::tr("Allow %1 to capture audio and video of your desktop?");
                case QWebEnginePage::Notifications:
                    return QObject::tr("Allow %1 to show notification on your desktop?");
                default:
                    break;
            }
            return {};
        }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QString questionForPermission(const QWebEnginePermission::PermissionType permissionType) {
            switch (permissionType) {
                case QWebEnginePermission::PermissionType::Geolocation:
                    return QObject::tr("Allow %1 to access your location information?");
                case QWebEnginePermission::PermissionType::MediaAudioCapture:
                    return QObject::tr("Allow %1 to access your microphone?");
                case QWebEnginePermission::PermissionType::MediaVideoCapture:
                    return QObject::tr("Allow %1 to access your webcam?");
                case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
                    return QObject::tr("Allow %1 to access your microphone and webcam?");
                case QWebEnginePermission::PermissionType::MouseLock:
                    return QObject::tr("Allow %1 to lock your mouse cursor?");
                case QWebEnginePermission::PermissionType::DesktopVideoCapture:
                    return QObject::tr("Allow %1 to capture video of your desktop?");
                case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
                    return QObject::tr("Allow %1 to capture audio and video of your desktop?");
                case QWebEnginePermission::PermissionType::Notifications:
                    return QObject::tr("Allow %1 to show notification on your desktop?");
                case QWebEnginePermission::PermissionType::ClipboardReadWrite:
                    return QObject::tr("Allow %1 to read and write your clipboard?");
                case QWebEnginePermission::PermissionType::LocalFontsAccess:
                    return QObject::tr("Allow %1 to access your local fonts?");
                case QWebEnginePermission::PermissionType::Unsupported:
                    break;
            }
            return {};
        }
#endif

        QString mobileDataUrlForHtml(const QString &html) {
            return QStringLiteral("data:text/html;charset=UTF-8,%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(html)));
        }
#endif

        QLocale activeWebLocale() {
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            return configuration
                   ? fairwindsk::localization::effectiveLocale(*configuration)
                   : fairwindsk::localization::effectiveLocaleForSelection(QStringLiteral("system"));
        }

        QString javaScriptStringLiteral(const QString &value) {
            QJsonArray values;
            values.append(value);
            const QString json = QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact));
            return json.mid(1, json.size() - 2);
        }

        QString webLocalizationScript() {
            const QLocale locale = activeWebLocale();
            const QString language = javaScriptStringLiteral(fairwindsk::localization::languageTag(locale));
            const QString culture = javaScriptStringLiteral(fairwindsk::localization::cultureName(locale));
            return QStringLiteral(
                       "(function(){"
                       "const language=%1;"
                       "const culture=%2;"
                       "if(document.documentElement){document.documentElement.setAttribute('lang',language);}"
                       "window.fairwindskLanguage=language;"
                       "window.fairwindskCulture=culture;"
                       "})();")
                .arg(language, culture);
        }

        QString signalKRestartPlaceholderHtml(const QPalette &palette,
                                              const QString &title,
                                              const QString &body) {
            const QColor windowColor = palette.color(QPalette::Window);
            const QColor panelColor = palette.color(QPalette::Base);
            const QColor textColor = fairwindsk::ui::bestContrastingColor(
                windowColor,
                {palette.color(QPalette::WindowText),
                 palette.color(QPalette::Text),
                 palette.color(QPalette::ButtonText)});
            const QColor mutedTextColor = fairwindsk::ui::comfortAlpha(textColor, 204);
            const QColor borderColor = fairwindsk::ui::comfortAlpha(palette.color(QPalette::Mid), 188);
            const QColor shadowColor = fairwindsk::ui::comfortAlpha(windowColor.darker(180), 180);

            const QLocale locale = activeWebLocale();
            const QString languageTag = fairwindsk::localization::languageTag(locale).toHtmlEscaped();
            const QString cultureName = fairwindsk::localization::cultureName(locale).toHtmlEscaped();
            const QString style = QStringLiteral(
                                      "html,body{height:100%%;margin:0;background:%1;color:%2;"
                                      "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;}"
                                      "body{display:flex;align-items:center;justify-content:center;}"
                                      ".panel{max-width:36rem;padding:2rem 2.5rem;border:1px solid %3;border-radius:18px;"
                                      "background:linear-gradient(180deg,%4,%5);box-shadow:0 18px 48px %6;"
                                      "text-align:center;}"
                                      ".title{font-size:2rem;font-weight:700;margin-bottom:0.75rem;}"
                                      ".body{font-size:1.05rem;line-height:1.5;color:%7;}")
                                      .arg(windowColor.name(),
                                           textColor.name(),
                                           borderColor.name(QColor::HexArgb),
                                           panelColor.lighter(104).name(),
                                           panelColor.darker(106).name(),
                                           shadowColor.name(QColor::HexArgb),
                                           mutedTextColor.name(QColor::HexArgb));

            return QStringLiteral(
                "<!doctype html>"
                "<html lang=\"%1\"><head><meta charset=\"utf-8\">"
                "<meta name=\"fairwindsk-culture\" content=\"%2\">"
                "<style>%3</style></head>"
                "<body><div class=\"panel\">"
                "<div class=\"title\">%4</div>"
                "<div class=\"body\">%5</div>"
                "</div></body></html>")
                .arg(languageTag,
                     cultureName,
                     style,
                     title.toHtmlEscaped(),
                     body.toHtmlEscaped());
        }
    }

    WebView::WebView(fairwindsk::WebProfileHandle *profile, QWidget *parent)
        : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        m_loadTimeoutTimer.setSingleShot(true);
        m_loadTimeoutTimer.setInterval(kHostedAppLoadTimeoutMs);
        connect(&m_loadTimeoutTimer, &QTimer::timeout, this, &WebView::handleLoadTimeout);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        initializeDesktop(profile);
#else
        Q_UNUSED(profile);
        initializeMobile();
#endif

        if (m_viewWidget) {
            m_viewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            layout->addWidget(m_viewWidget);
        }

        if (auto *client = fairwindsk::FairWindSK::getInstance()->getSignalKClient()) {
            connect(client,
                    &fairwindsk::signalk::Client::connectivityChanged,
                    this,
                    &WebView::handleConnectivityChanged,
                    Qt::UniqueConnection);
            connect(client,
                    &fairwindsk::signalk::Client::serverStateResynchronized,
                    this,
                    &WebView::handleServerStateResynchronized,
                    Qt::UniqueConnection);
        }
    }

    WebView::~WebView() = default;

    void WebView::load(const QUrl &url) {
        setUrl(url);
    }

    void WebView::setUrl(const QUrl &url) {
        if (url.isValid() && !url.isEmpty() && url.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) != 0) {
            m_requestedUrl = url;
        }
        stopLoadTimeoutWatch();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->setUrl(url);
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "loadUrl", Q_ARG(QVariant, url.toString()));
        }
#endif
    }

    QUrl WebView::url() const {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        return m_desktopView ? m_desktopView->url() : QUrl();
#else
        if (m_quickRoot) {
            const QVariant value = m_quickRoot->property("currentUrl");
            if (value.canConvert<QUrl>()) {
                return value.toUrl();
            }
            return QUrl::fromUserInput(value.toString());
        }
        return m_currentUrl;
#endif
    }

    void WebView::setHtml(const QString &html, const QUrl &baseUrl) {
        stopLoadTimeoutWatch();
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->setHtml(html, baseUrl);
        }
#else
        Q_UNUSED(baseUrl);
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "setHtmlContent", Q_ARG(QVariant, html), Q_ARG(QVariant, baseUrl.toString()));
        }
#endif
    }

    void WebView::reload() {
        if (m_healthState != HealthState::Normal && m_requestedUrl.isValid()) {
            m_restartPlaceholderVisible = false;
            setHealthState(HealthState::Normal, tr("Retrying hosted app"));
            setUrl(m_requestedUrl);
            return;
        }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->reload();
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "reloadPage");
        }
#endif
    }

    void WebView::stop() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->stop();
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "stopLoading");
        }
#endif
    }

    void WebView::back() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->back();
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "goBack");
        }
#endif
    }

    void WebView::forward() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->forward();
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "goForward");
        }
#endif
    }

    bool WebView::canGoBack() const {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        return m_desktopView && m_desktopView->history() && m_desktopView->history()->canGoBack();
#else
        return m_quickRoot ? m_quickRoot->property("canGoBack").toBool() : m_canGoBack;
#endif
    }

    bool WebView::canGoForward() const {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        return m_desktopView && m_desktopView->history() && m_desktopView->history()->canGoForward();
#else
        return m_quickRoot ? m_quickRoot->property("canGoForward").toBool() : m_canGoForward;
#endif
    }

    void WebView::runJavaScript(const QString &script) {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView && m_desktopView->page()) {
            m_desktopView->page()->runJavaScript(script);
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "runScript", Q_ARG(QVariant, script));
        }
#endif
    }

    double WebView::zoomPercent() const {
        return m_zoomPercent;
    }

    void WebView::setZoomPercent(const double zoomPercent) {
        m_zoomPercent = qBound(25.0, zoomPercent, 400.0);
        applyZoom();
    }

    void WebView::releaseMobileFocus() {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        m_mobileWebContentFocused = false;
        m_mobileTextInputActive = false;
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "releaseWebFocus");
        }
#endif
    }

    void WebView::applyMobileShellMetrics(const QMargins &safeAreaMargins,
                                          const int keyboardInset,
                                          const bool keyboardVisible,
                                          const bool compactMode) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot,
                                      "applyShellMetrics",
                                      Q_ARG(QVariant, safeAreaMargins.left()),
                                      Q_ARG(QVariant, safeAreaMargins.top()),
                                      Q_ARG(QVariant, safeAreaMargins.right()),
                                      Q_ARG(QVariant, safeAreaMargins.bottom()),
                                      Q_ARG(QVariant, keyboardInset),
                                      Q_ARG(QVariant, keyboardVisible),
                                      Q_ARG(QVariant, compactMode));
        }
#else
        Q_UNUSED(safeAreaMargins);
        Q_UNUSED(keyboardInset);
        Q_UNUSED(keyboardVisible);
        Q_UNUSED(compactMode);
#endif
    }

    QString WebView::zoomScript(const double zoomPercent) {
        const QString factorText = QString::number(qBound(0.25, zoomPercent / 100.0, 4.0), 'f', 2);
        return QStringLiteral(
                   "(function(){"
                   "const factor=%1;"
                   "document.documentElement.style.zoom=factor;"
                   "if (document.body) {"
                   "document.body.style.zoom=factor;"
                   "document.body.style.transformOrigin='top left';"
                   "document.body.style.width=(100/factor)+'%%';"
                   "}"
                   "})();")
            .arg(factorText);
    }

    void WebView::applyZoom() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView) {
            m_desktopView->setZoomFactor(m_zoomPercent / 100.0);
        }
#else
        if (m_quickRoot) {
            QMetaObject::invokeMethod(m_quickRoot, "runScript", Q_ARG(QVariant, zoomScript(m_zoomPercent)));
        }
#endif
    }

    void WebView::applyWebLocalization() {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        if (m_desktopView && m_desktopView->page()) {
            m_desktopView->page()->runJavaScript(webLocalizationScript());
        }
#else
        if (m_quickRoot) {
            const QLocale locale = activeWebLocale();
            QMetaObject::invokeMethod(m_quickRoot,
                                      "applyLocalization",
                                      Q_ARG(QVariant, fairwindsk::localization::languageTag(locale)),
                                      Q_ARG(QVariant, fairwindsk::localization::cultureName(locale)));
        }
#endif
    }

    void WebView::showSignalKRestartPlaceholder() {
        showFallbackPlaceholder(HealthState::Restarting,
                                tr("Signal K is restarting"),
                                tr("FairWindSK will reconnect automatically when the server is available again."),
                                m_restartResumeUrl);
    }

    void WebView::showFallbackPlaceholder(const HealthState state,
                                          const QString &title,
                                          const QString &body,
                                          const QUrl &resumeUrl) {
        stop();
        stopLoadTimeoutWatch();
        if (resumeUrl.isValid() && !resumeUrl.isEmpty() && resumeUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) != 0) {
            m_requestedUrl = resumeUrl;
        }
        m_restartPlaceholderVisible = state == HealthState::Restarting;
        setHealthState(state, title);
        setHtml(signalKRestartPlaceholderHtml(palette(), title, body));
        applyZoom();
    }

    void WebView::setHealthState(const HealthState state, const QString &summary) {
        const QString normalizedSummary = summary.trimmed().isEmpty() ? tr("Web view ready") : summary.trimmed();
        if (m_healthState == state && m_healthSummary == normalizedSummary) {
            return;
        }

        m_healthState = state;
        m_healthSummary = normalizedSummary;
        emit healthStateChanged(m_healthState, m_healthSummary);
    }

    void WebView::startLoadTimeoutWatch() {
        if (m_requestedUrl.isValid() && !m_requestedUrl.isEmpty()) {
            m_loadTimeoutTimer.start();
        }
    }

    void WebView::stopLoadTimeoutWatch() {
        if (m_loadTimeoutTimer.isActive()) {
            m_loadTimeoutTimer.stop();
        }
    }

    bool WebView::isSignalKHostedUrl(const QUrl &url) const {
        if (!url.isValid() || url.isEmpty()) {
            return false;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        if (!configuration) {
            return false;
        }

        const QUrl serverUrl = QUrl::fromUserInput(configuration->getSignalKServerUrl().trimmed());
        return serverUrl.isValid()
               && !serverUrl.isEmpty()
               && serverUrl.scheme().compare(url.scheme(), Qt::CaseInsensitive) == 0
               && serverUrl.host().compare(url.host(), Qt::CaseInsensitive) == 0
               && (serverUrl.port() == url.port() || (serverUrl.port() < 0 && url.port() < 0));
    }

    bool WebView::noteAuthenticationChallenge(const QString &originLabel) {
        const QDateTime now = QDateTime::currentDateTimeUtc();
        if (!m_lastAuthChallengeAt.isValid()
            || m_lastAuthChallengeAt.msecsTo(now) > kHostedAppAuthLoopWindowMs) {
            m_authChallengeCount = 0;
        }

        m_lastAuthChallengeAt = now;
        ++m_authChallengeCount;
        if (m_authChallengeCount < kHostedAppAuthLoopThreshold) {
            return false;
        }

        showFallbackPlaceholder(
            HealthState::Failed,
            tr("Hosted app authentication loop"),
            tr("The hosted app keeps requesting credentials for %1. Use reload to retry, home to return to the app start page, settings to inspect server access, or close to go back to the launcher.")
                .arg(originLabel.isEmpty() ? tr("this service") : originLabel),
            m_requestedUrl);
        return true;
    }

    void WebView::handleLoadTimeout() {
        if (!m_requestedUrl.isValid() || m_requestedUrl.isEmpty()) {
            return;
        }

        ++m_consecutiveFailureCount;

        showFallbackPlaceholder(HealthState::Failed,
                                m_consecutiveFailureCount > 1 ? tr("Hosted app retry timeout") : tr("Hosted app timeout"),
                                m_consecutiveFailureCount > 1
                                    ? tr("The hosted app keeps timing out. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher.")
                                    : tr("The page is taking too long to respond. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher."),
                                m_requestedUrl);
        emit loadFinished(false);
    }

    void WebView::handleServerStateResynchronized(const bool recoveredFromDisconnect) {
        if (!recoveredFromDisconnect || !m_restartPlaceholderVisible || !m_restartResumeUrl.isValid()) {
            return;
        }

        m_restartPlaceholderVisible = false;
        setHealthState(HealthState::Normal, tr("Hosted app restored"));
        setUrl(m_restartResumeUrl);
    }

    void WebView::handleConnectivityChanged(const bool, const bool streamHealthy, const QString &statusText) {
        if (streamHealthy && m_healthState == HealthState::Degraded && isSignalKHostedUrl(m_requestedUrl)) {
            setHealthState(HealthState::Normal, tr("Hosted app live"));
        }

        if (streamHealthy || !statusText.contains(QStringLiteral("restart"), Qt::CaseInsensitive)) {
            if ((!streamHealthy || !statusText.trimmed().isEmpty()) && isSignalKHostedUrl(m_requestedUrl)) {
                setHealthState(HealthState::Degraded,
                               statusText.trimmed().isEmpty()
                                   ? tr("Hosted app server degraded")
                                   : tr("Hosted app server degraded: %1").arg(statusText.trimmed()));
            }
            return;
        }

        const QUrl currentUrl = url();
        if (!currentUrl.isValid()
            || currentUrl.isEmpty()
            || currentUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) == 0) {
            return;
        }

        m_restartResumeUrl = currentUrl;
        showSignalKRestartPlaceholder();
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void WebView::initializeDesktop(fairwindsk::WebProfileHandle *profile) {
        m_desktopView = new QWebEngineView(this);
        m_viewWidget = m_desktopView;

        connect(m_desktopView, &QWebEngineView::loadStarted, this, [this]() {
            m_loadProgress = 0;
            startLoadTimeoutWatch();
            emit loadStarted();
        });
        connect(m_desktopView, &QWebEngineView::loadProgress, this, [this](const int progress) {
            m_loadProgress = progress;
            emit loadProgress(progress);
        });
        connect(m_desktopView, &QWebEngineView::loadFinished, this, [this](const bool success) {
            stopLoadTimeoutWatch();
            m_loadProgress = success ? 100 : -1;
            if (success) {
                m_consecutiveFailureCount = 0;
                const QUrl currentUrl = m_desktopView ? m_desktopView->url() : QUrl();
                if (!currentUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive)) {
                    m_restartPlaceholderVisible = true;
                    setHealthState(HealthState::Failed, tr("Hosted app fallback active"));
                } else if (!currentUrl.isValid() || currentUrl.isEmpty()) {
                    ++m_consecutiveFailureCount;
                    showFallbackPlaceholder(HealthState::Failed,
                                            m_consecutiveFailureCount > 1 ? tr("Hosted app still blank") : tr("Hosted app blank page"),
                                            tr("The app returned an empty page. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher."),
                                            m_requestedUrl);
                } else {
                    m_restartPlaceholderVisible = false;
                    m_requestedUrl = currentUrl;
                    setHealthState(HealthState::Normal, tr("Hosted app live"));
                }
                applyWebLocalization();
                applyZoom();
            } else if (m_requestedUrl.isValid()) {
                ++m_consecutiveFailureCount;
                showFallbackPlaceholder(HealthState::Failed,
                                        m_consecutiveFailureCount > 1 ? tr("Hosted app retry failed") : tr("Hosted app unavailable"),
                                        m_consecutiveFailureCount > 1
                                            ? tr("The page still could not be loaded. Use reload to retry again, home to return to the app start page, settings to inspect connectivity, or close to go back to the launcher.")
                                            : tr("The page could not be loaded. Use reload to retry, home to return to the app start page, settings to inspect connectivity, or close to go back to the launcher."),
                                        m_requestedUrl);
            }
            emit loadFinished(success);
        });
        connect(m_desktopView, &QWebEngineView::urlChanged, this, &WebView::urlChanged);
        connect(m_desktopView, &QWebEngineView::titleChanged, this, &WebView::titleChanged);
        connect(m_desktopView, &QWebEngineView::renderProcessTerminated, this,
                [this](QWebEnginePage::RenderProcessTerminationStatus termStatus, const int statusCode) {
                    QString status;
                    switch (termStatus) {
                        case QWebEnginePage::NormalTerminationStatus:
                            status = tr("Render process normal exit");
                            break;
                        case QWebEnginePage::AbnormalTerminationStatus:
                            status = tr("Render process abnormal exit");
                            break;
                        case QWebEnginePage::CrashedTerminationStatus:
                            status = tr("Render process crashed");
                            break;
                        case QWebEnginePage::KilledTerminationStatus:
                            status = tr("Render process killed");
                            break;
                    }
                    showFallbackPlaceholder(HealthState::Failed,
                                            status,
                                            tr("The hosted app stopped unexpectedly (code %1). Use reload to try again, home to return to the app start page, settings to inspect connectivity, or close to go back to the launcher.")
                                                .arg(statusCode),
                                            m_requestedUrl);
                });

        m_webPage = new WebPage(profile, m_desktopView);
        setDesktopPage(m_webPage);
    }

    void WebView::setDesktopPage(WebPage *page) {
        auto *oldPage = qobject_cast<WebPage *>(m_desktopView ? m_desktopView->page() : nullptr);
        if (oldPage) {
            disconnect(oldPage, nullptr, this, nullptr);
        }

        if (m_desktopView) {
            m_desktopView->setPage(page);
        }
        m_webPage = page;

        if (!page) {
            return;
        }

        connect(page, &WebPage::createCertificateErrorDialog, this, [this](QWebEngineCertificateError error) {
            auto *dialog = new QWidget(window());

            Ui::CertificateErrorDialog certificateDialog;
            certificateDialog.setupUi(dialog);
            certificateDialog.buttonBox->hide();
            certificateDialog.m_iconLabel->setText(QString());
            const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, window()));
            certificateDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));
            certificateDialog.m_errorLabel->setText(error.description());
            dialog->setWindowTitle(tr("Certificate Error"));

            if (drawer::execDrawer(this,
                                   tr("Certificate Error"),
                                   dialog,
                                   {{tr("Continue"), int(QMessageBox::Ok), false},
                                    {tr("Cancel"), int(QMessageBox::Cancel), true}},
                                   int(QMessageBox::Cancel)) == QMessageBox::Ok) {
                error.acceptCertificate();
            } else {
                error.rejectCertificate();
            }
        });

        connect(page, &WebPage::signalKRestartConfirmed, this, [this]() {
            const QUrl currentUrl = url();
            if (currentUrl.isValid()
                && !currentUrl.isEmpty()
                && currentUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) != 0) {
                m_restartResumeUrl = currentUrl;
            }
            QTimer::singleShot(1500, this, [this]() {
                auto *client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
                if (client && !client->isStreamHealthy()) {
                    showSignalKRestartPlaceholder();
                }
            });
        });

        connect(page, &QWebEnginePage::authenticationRequired, this,
                [this](const QUrl &requestUrl, QAuthenticator *auth) {
                    if (noteAuthenticationChallenge(requestUrl.host())) {
                        *auth = QAuthenticator();
                        return;
                    }
                    auto *dialog = new QWidget(window());

                    Ui::PasswordDialog passwordDialog;
                    passwordDialog.setupUi(dialog);
                    passwordDialog.buttonBox->hide();

                    passwordDialog.m_iconLabel->setText(QString());
                    const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, window()));
                    passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));

                    passwordDialog.m_infoLabel->setText(
                        tr("Enter username and password for \"%1\" at %2").arg(auth->realm(),
                                                                                requestUrl.toString().toHtmlEscaped()));
                    passwordDialog.m_infoLabel->setWordWrap(true);

                    if (drawer::execDrawer(this,
                                           tr("Authentication Required"),
                                           dialog,
                                           {{tr("OK"), int(QMessageBox::Ok), true},
                                            {tr("Cancel"), int(QMessageBox::Cancel), false}},
                                           int(QMessageBox::Cancel)) == QMessageBox::Ok) {
                        auth->setUser(passwordDialog.m_userNameLineEdit->text());
                        auth->setPassword(passwordDialog.m_passwordLineEdit->text());
                    } else {
                        *auth = QAuthenticator();
                    }
                });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        connect(page, &QWebEnginePage::permissionRequested, this, [this](QWebEnginePermission permission) {
            if (!permission.isValid()) {
                return;
            }

            const QString question = questionForPermission(permission.permissionType()).arg(permission.origin().host());
            if (!question.isEmpty()
                && drawer::question(window(),
                                    tr("Permission Request"),
                                    question,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No) == QMessageBox::Yes) {
                permission.grant();
            } else {
                permission.deny();
            }
        });
#else
        connect(page, &QWebEnginePage::featurePermissionRequested, this,
                [this, page](const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
                    const QString question = questionForFeature(feature).arg(securityOrigin.host());
                    if (!question.isEmpty()
                        && drawer::question(window(),
                                            tr("Permission Request"),
                                            question,
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No) == QMessageBox::Yes) {
                        page->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
                    } else {
                        page->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionDeniedByUser);
                    }
                });
#endif

        connect(page, &QWebEnginePage::proxyAuthenticationRequired, this,
                [this](const QUrl &, QAuthenticator *auth, const QString &proxyHost) {
                    if (noteAuthenticationChallenge(proxyHost)) {
                        *auth = QAuthenticator();
                        return;
                    }
                    auto *dialog = new QWidget(window());

                    Ui::PasswordDialog passwordDialog;
                    passwordDialog.setupUi(dialog);
                    passwordDialog.buttonBox->hide();

                    passwordDialog.m_iconLabel->setText(QString());
                    const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, window()));
                    passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));
                    passwordDialog.m_infoLabel->setText(tr("Connect to proxy \"%1\" using:").arg(proxyHost.toHtmlEscaped()));
                    passwordDialog.m_infoLabel->setWordWrap(true);

                    if (drawer::execDrawer(this,
                                           tr("Proxy Authentication Required"),
                                           dialog,
                                           {{tr("OK"), int(QMessageBox::Ok), true},
                                            {tr("Cancel"), int(QMessageBox::Cancel), false}},
                                           int(QMessageBox::Cancel)) == QMessageBox::Ok) {
                        auth->setUser(passwordDialog.m_userNameLineEdit->text());
                        auth->setPassword(passwordDialog.m_passwordLineEdit->text());
                    } else {
                        *auth = QAuthenticator();
                    }
                });

        connect(page, &QWebEnginePage::registerProtocolHandlerRequested, this,
                [this](QWebEngineRegisterProtocolHandlerRequest request) {
                    const auto answer = drawer::question(window(),
                                                         tr("Permission Request"),
                                                         tr("Allow %1 to open all %2 links?").arg(request.origin().host(), request.scheme()),
                                                         QMessageBox::Yes | QMessageBox::No,
                                                         QMessageBox::No);
                    if (answer == QMessageBox::Yes) {
                        request.accept();
                    } else {
                        request.reject();
                    }
                });

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        connect(page, &QWebEnginePage::fileSystemAccessRequested, this,
                [this](QWebEngineFileSystemAccessRequest request) {
                    QString accessType;
                    switch (request.accessFlags()) {
                        case QWebEngineFileSystemAccessRequest::Read:
                            accessType = tr("read");
                            break;
                        case QWebEngineFileSystemAccessRequest::Write:
                            accessType = tr("write");
                            break;
                        case QWebEngineFileSystemAccessRequest::Read | QWebEngineFileSystemAccessRequest::Write:
                            accessType = tr("read and write");
                            break;
                        default:
                            accessType = tr("access");
                            break;
                    }

                    const auto answer = drawer::question(window(),
                                                         tr("File system access request"),
                                                         tr("Give %1 %2 access to %3?")
                                                             .arg(request.origin().host(), accessType, request.filePath().toString()),
                                                         QMessageBox::Yes | QMessageBox::No,
                                                         QMessageBox::No);
                    if (answer == QMessageBox::Yes) {
                        request.accept();
                    } else {
                        request.reject();
                    }
                });
#endif

        connect(page, &QWebEnginePage::geometryChangeRequested, this, &WebView::geometryChangeRequested);
        connect(page, &QWebEnginePage::windowCloseRequested, this, &WebView::windowCloseRequested);
    }
#else
    void WebView::initializeMobile() {
        m_quickView = new QQuickWidget(this);
        m_quickView->setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_quickView->setClearColor(Qt::transparent);
        m_quickView->setFocusPolicy(Qt::StrongFocus);
        m_quickView->setSource(QUrl(QStringLiteral("qrc:/ui/web/MobileWebView.qml")));
        m_quickRoot = m_quickView->rootObject();
        m_viewWidget = m_quickView;

        if (!m_quickRoot) {
            setHealthState(HealthState::Unsupported, tr("Mobile web backend unavailable"));
            return;
        }

        applyWebLocalization();

        connect(m_quickRoot, SIGNAL(loadStarted()), this, SLOT(handleMobileLoadStarted()));
        connect(m_quickRoot, SIGNAL(loadProgressChanged(int)), this, SLOT(handleMobileLoadProgressChanged(int)));
        connect(m_quickRoot, SIGNAL(loadFinished(bool)), this, SLOT(handleMobileLoadFinished(bool)));
        connect(m_quickRoot, SIGNAL(currentUrlNotified(QString)), this, SLOT(handleMobileCurrentUrlNotified(QString)));
        connect(m_quickRoot, SIGNAL(titleChanged(QString)), this, SLOT(handleMobileTitleChanged(QString)));
        connect(m_quickRoot, SIGNAL(webFocusChanged(bool)), this, SLOT(handleMobileFocusChanged(bool)));
        connect(m_quickRoot, SIGNAL(textInputActiveChanged(bool)), this, SLOT(handleMobileTextInputChanged(bool)));
        connect(m_quickRoot, SIGNAL(viewportMetricsChanged(qreal,qreal,bool)), this, SLOT(handleMobileViewportChanged(qreal,qreal,bool)));
    }

    void WebView::handleMobileLoadStarted() {
        m_loadProgress = 0;
        startLoadTimeoutWatch();
        emit loadStarted();
    }

    void WebView::handleMobileLoadProgressChanged(const int progress) {
        m_loadProgress = progress;
        emit loadProgress(progress);
    }

    void WebView::handleMobileLoadFinished(const bool ok) {
        stopLoadTimeoutWatch();
        m_loadProgress = ok ? 100 : -1;
        if (ok) {
            m_consecutiveFailureCount = 0;
            if (!m_currentUrl.isValid() || m_currentUrl.isEmpty()) {
                ++m_consecutiveFailureCount;
                showFallbackPlaceholder(HealthState::Unsupported,
                                        m_consecutiveFailureCount > 1 ? tr("Hosted app still blank") : tr("Hosted app blank page"),
                                        tr("This mobile backend returned an empty page. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher."),
                                        m_requestedUrl);
            } else {
                setHealthState(HealthState::Normal, tr("Hosted app live"));
            }
            applyWebLocalization();
            applyZoom();
        } else if (m_requestedUrl.isValid()) {
            ++m_consecutiveFailureCount;
            showFallbackPlaceholder(HealthState::Unsupported,
                                    m_consecutiveFailureCount > 1 ? tr("Hosted app retry failed") : tr("Hosted app unavailable"),
                                    m_consecutiveFailureCount > 1
                                        ? tr("This page still could not be shown on the current mobile backend. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher.")
                                        : tr("This page could not be shown on the current mobile backend. Use reload to retry, home to return to the app start page, settings to inspect configuration, or close to go back to the launcher."),
                                    m_requestedUrl);
        }
        emit loadFinished(ok);
    }

    void WebView::handleMobileCurrentUrlNotified(const QString &urlText) {
        m_currentUrl = QUrl::fromUserInput(urlText);
        m_restartPlaceholderVisible = m_currentUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) == 0;
        if (m_loadProgress <= 0 && m_currentUrl.isValid() && !m_currentUrl.isEmpty() && !m_restartPlaceholderVisible) {
            startLoadTimeoutWatch();
        }
        m_canGoBack = canGoBack();
        m_canGoForward = canGoForward();
        emit urlChanged(m_currentUrl);
    }

    void WebView::handleMobileTitleChanged(const QString &titleText) {
        emit titleChanged(titleText);
    }

    void WebView::handleMobileFocusChanged(const bool focused) {
        m_mobileWebContentFocused = focused;
        if (!focused) {
            m_mobileTextInputActive = false;
        }
        emit mobileFocusChanged(m_mobileWebContentFocused, m_mobileTextInputActive);
    }

    void WebView::handleMobileTextInputChanged(const bool active) {
        m_mobileTextInputActive = active;
        if (active) {
            m_mobileWebContentFocused = true;
        }
        emit mobileFocusChanged(m_mobileWebContentFocused, m_mobileTextInputActive);
    }

    void WebView::handleMobileViewportChanged(const qreal width, const qreal height, const bool keyboardVisible) {
        const QRect viewport(0, 0, qMax(0, qRound(width)), qMax(0, qRound(height)));
        if (viewport == m_mobileViewport) {
            emit mobileViewportChanged(m_mobileViewport, keyboardVisible);
            return;
        }

        m_mobileViewport = viewport;
        emit mobileViewportChanged(m_mobileViewport, keyboardVisible);
    }
#endif
}
