//
// Created by Raffaele Montella on 28/03/21.
//

#include "WebView.hpp"

#include <QVBoxLayout>
#include <QtGlobal>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QAuthenticator>
#include <QDialog>
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
    }

    WebView::WebView(fairwindsk::WebProfileHandle *profile, QWidget *parent)
        : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

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
    }

    WebView::~WebView() = default;

    void WebView::load(const QUrl &url) {
        setUrl(url);
    }

    void WebView::setUrl(const QUrl &url) {
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

    void WebView::setZoomPercent(const double zoomPercent) {
        m_zoomPercent = qBound(25.0, zoomPercent, 400.0);
        applyZoom();
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

    void WebView::showSignalKRestartPlaceholder() {
        stop();
        m_restartPlaceholderVisible = true;
        setHtml(QStringLiteral(
                    "<!doctype html>"
                    "<html><head><meta charset=\"utf-8\">"
                    "<style>"
                    "html,body{height:100%%;margin:0;background:#05070c;color:#f8fafc;"
                    "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;}"
                    "body{display:flex;align-items:center;justify-content:center;}"
                    ".panel{max-width:36rem;padding:2rem 2.5rem;border:1px solid #2f4058;border-radius:18px;"
                    "background:linear-gradient(180deg,#0f1724,#0a1220);box-shadow:0 18px 48px rgba(0,0,0,0.45);"
                    "text-align:center;}"
                    ".title{font-size:2rem;font-weight:700;margin-bottom:0.75rem;}"
                    ".body{font-size:1.05rem;line-height:1.5;color:#c8d2df;}"
                    "</style></head>"
                    "<body><div class=\"panel\">"
                    "<div class=\"title\">%1</div>"
                    "<div class=\"body\">%2</div>"
                    "</div></body></html>")
                    .arg(tr("Signal K is restarting"),
                         tr("FairWindSK will reconnect automatically when the server is available again.")));
        applyZoom();
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void WebView::initializeDesktop(fairwindsk::WebProfileHandle *profile) {
        m_desktopView = new QWebEngineView(this);
        m_viewWidget = m_desktopView;

        connect(m_desktopView, &QWebEngineView::loadStarted, this, [this]() {
            m_loadProgress = 0;
            emit loadStarted();
        });
        connect(m_desktopView, &QWebEngineView::loadProgress, this, [this](const int progress) {
            m_loadProgress = progress;
            emit loadProgress(progress);
        });
        connect(m_desktopView, &QWebEngineView::loadFinished, this, [this](const bool success) {
            m_loadProgress = success ? 100 : -1;
            if (success) {
                const QUrl currentUrl = m_desktopView ? m_desktopView->url() : QUrl();
                if (!currentUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive)) {
                    m_restartPlaceholderVisible = true;
                } else {
                    m_restartPlaceholderVisible = false;
                }
                applyZoom();
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
                    const QMessageBox::StandardButton btn = drawer::question(window(),
                                                                            status,
                                                                            tr("Render process exited with code: %1\nDo you want to reload the page ?").arg(statusCode),
                                                                            QMessageBox::Yes | QMessageBox::No,
                                                                            QMessageBox::No);
                    if (btn == QMessageBox::Yes) {
                        QTimer::singleShot(0, this, &WebView::reload);
                    }
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
            auto *dialog = new QDialog(window());
            dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
                                   {{tr("Continue"), QDialog::Accepted, false},
                                    {tr("Cancel"), QDialog::Rejected, true}},
                                   QDialog::Rejected) == QDialog::Accepted) {
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

        if (auto *client = fairwindsk::FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::serverStateResynchronized, this,
                    [this](const bool recoveredFromDisconnect) {
                        if (!recoveredFromDisconnect || !m_restartPlaceholderVisible || !m_restartResumeUrl.isValid()) {
                            return;
                        }

                        m_restartPlaceholderVisible = false;
                        setUrl(m_restartResumeUrl);
                    },
                    Qt::UniqueConnection);
        }

        connect(page, &QWebEnginePage::authenticationRequired, this,
                [this](const QUrl &requestUrl, QAuthenticator *auth) {
                    auto *dialog = new QDialog(window());
                    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
                                           {{tr("OK"), QDialog::Accepted, true},
                                            {tr("Cancel"), QDialog::Rejected, false}},
                                           QDialog::Rejected) == QDialog::Accepted) {
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
                    auto *dialog = new QDialog(window());
                    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
                                           {{tr("OK"), QDialog::Accepted, true},
                                            {tr("Cancel"), QDialog::Rejected, false}},
                                           QDialog::Rejected) == QDialog::Accepted) {
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
            return;
        }

        connect(m_quickRoot, SIGNAL(loadStarted()), this, SIGNAL(loadStarted()));
        connect(m_quickRoot, SIGNAL(loadProgressChanged(int)), this, SLOT(handleMobileLoadProgressChanged(int)));
        connect(m_quickRoot, SIGNAL(loadFinished(bool)), this, SLOT(handleMobileLoadFinished(bool)));
        connect(m_quickRoot, SIGNAL(currentUrlNotified(QString)), this, SLOT(handleMobileCurrentUrlNotified(QString)));
        connect(m_quickRoot, SIGNAL(titleChanged(QString)), this, SLOT(handleMobileTitleChanged(QString)));
    }

    void WebView::handleMobileLoadProgressChanged(const int progress) {
        m_loadProgress = progress;
        emit loadProgress(progress);
    }

    void WebView::handleMobileLoadFinished(const bool ok) {
        m_loadProgress = ok ? 100 : -1;
        if (ok) {
            applyZoom();
        }
        emit loadFinished(ok);
    }

    void WebView::handleMobileCurrentUrlNotified(const QString &urlText) {
        m_currentUrl = QUrl::fromUserInput(urlText);
        m_canGoBack = canGoBack();
        m_canGoForward = canGoForward();
        emit urlChanged(m_currentUrl);
    }

    void WebView::handleMobileTitleChanged(const QString &titleText) {
        emit titleChanged(titleText);
    }
#endif
}
