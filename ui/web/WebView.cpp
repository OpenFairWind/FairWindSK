//
// Created by Raffaele Montella on 28/03/21.
//

//#include "Browser.hpp"
//#include "BrowserWindow.hpp"

#include "WebPage.hpp"
#include "WebPopupWindow.hpp"
#include "WebView.hpp"


#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QAuthenticator>
#include <QTimer>
#include <QStyle>

#include "ui/DrawerDialogHost.hpp"

using namespace Qt::StringLiterals;

namespace fairwindsk::ui::web {
    /*
     * WebView
     * Constructor
     */
    WebView::WebView(QWebEngineProfile *profile, QWidget *parent): QWebEngineView(parent)
    {
        // Set the load progress to 0 when load started
        connect(this, &QWebEngineView::loadStarted, [this]() {
            m_loadProgress = 0;
        });

        // Set load progress to the parameter
        connect(this, &QWebEngineView::loadProgress, [this](int progress) {
            m_loadProgress = progress;
        });

        // Check when the load process ended
        connect(this, &QWebEngineView::loadFinished, [this](bool success) {
            m_loadProgress = success ? 100 : -1;
        });



        // Handle the rendering termination
        connect(this, &QWebEngineView::renderProcessTerminated,
                [this](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
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
                    const QMessageBox::StandardButton btn = drawer::question(window(), status,
                                                                            tr("Render process exited with code: %1\n"
                                                                               "Do you want to reload the page ?").arg(statusCode),
                                                                            QMessageBox::Yes | QMessageBox::No,
                                                                            QMessageBox::No);
                    if (btn == QMessageBox::Yes)
                        QTimer::singleShot(0, this, &WebView::reload);
                });

        // Create the web page
        m_webPage = new WebPage(profile);

        // Set the web page
        setPage(m_webPage);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    inline QString questionForFeature(QWebEnginePage::Feature feature) {
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
            default: ;
        }
        return {};
    }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    inline QString questionForPermission(QWebEnginePermission::PermissionType permissionType) {
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

    void WebView::setPage(WebPage *page)
    {
        auto oldPage = qobject_cast<WebPage *>(QWebEngineView::page());
        if (oldPage) {
            disconnect(oldPage, &WebPage::createCertificateErrorDialog, this,
                       &WebView::handleCertificateError);
            disconnect(oldPage, &QWebEnginePage::authenticationRequired, this,
                       &WebView::handleAuthenticationRequired);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            disconnect(oldPage, &QWebEnginePage::permissionRequested, this,
                       &WebView::handlePermissionRequested);
#else
            disconnect(oldPage, &QWebEnginePage::featurePermissionRequested, this,
                       &WebView::handleFeaturePermissionRequested);
#endif
            disconnect(oldPage, &QWebEnginePage::proxyAuthenticationRequired, this,
                       &WebView::handleProxyAuthenticationRequired);
            disconnect(oldPage, &QWebEnginePage::registerProtocolHandlerRequested, this,
                       &WebView::handleRegisterProtocolHandlerRequested);
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
            disconnect(oldPage, &QWebEnginePage::fileSystemAccessRequested, this,
                       &WebView::handleFileSystemAccessRequested);
#endif
        }

        QWebEngineView::setPage(page);
        m_webPage = page;

        if (oldPage && oldPage != page) {
            oldPage->deleteLater();
        }

        connect(page, &WebPage::createCertificateErrorDialog, this, &WebView::handleCertificateError);
        connect(page, &QWebEnginePage::authenticationRequired, this,
                &WebView::handleAuthenticationRequired);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        connect(page, &QWebEnginePage::permissionRequested, this,
                &WebView::handlePermissionRequested);
#else
        connect(page, &QWebEnginePage::featurePermissionRequested, this,
                &WebView::handleFeaturePermissionRequested);
#endif
        connect(page, &QWebEnginePage::proxyAuthenticationRequired, this,
                &WebView::handleProxyAuthenticationRequired);
        connect(page, &QWebEnginePage::registerProtocolHandlerRequested, this,
                &WebView::handleRegisterProtocolHandlerRequested);
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        connect(page, &QWebEnginePage::fileSystemAccessRequested, this,
                &WebView::handleFileSystemAccessRequested);
#endif
    }

    int WebView::loadProgress() const {
        return m_loadProgress;
    }


    void WebView::handleCertificateError(QWebEngineCertificateError error) {
        auto *dialog = new QDialog(window());
        dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::CertificateErrorDialog certificateDialog;
        certificateDialog.setupUi(dialog);
        certificateDialog.buttonBox->hide();
        certificateDialog.m_iconLabel->setText(QString());
        const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, window()));
        certificateDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));
        certificateDialog.m_errorLabel->setText(error.description());
        dialog->setWindowTitle(tr("Certificate Error"));

        if (drawer::execDrawer(this, tr("Certificate Error"), dialog,
                               {{tr("Continue"), QDialog::Accepted, false},
                                {tr("Cancel"), QDialog::Rejected, true}},
                               QDialog::Rejected) == QDialog::Accepted)
            error.acceptCertificate();
        else
            error.rejectCertificate();
    }

    void WebView::handleAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth) {
        auto *dialog = new QDialog(window());
        dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::PasswordDialog passwordDialog;
        passwordDialog.setupUi(dialog);
        passwordDialog.buttonBox->hide();

        passwordDialog.m_iconLabel->setText(QString());
        const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, window()));
        passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));

        QString introMessage(tr("Enter username and password for \"%1\" at %2")
                                     .arg(auth->realm(),
                                          requestUrl.toString().toHtmlEscaped()));
        passwordDialog.m_infoLabel->setText(introMessage);
        passwordDialog.m_infoLabel->setWordWrap(true);

        if (drawer::execDrawer(this, tr("Authentication Required"), dialog,
                               {{tr("OK"), QDialog::Accepted, true},
                                {tr("Cancel"), QDialog::Rejected, false}},
                               QDialog::Rejected) == QDialog::Accepted) {
            auth->setUser(passwordDialog.m_userNameLineEdit->text());
            auth->setPassword(passwordDialog.m_passwordLineEdit->text());
        } else {
            // Set authenticator null if dialog is cancelled
            *auth = QAuthenticator();
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    void WebView::handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
        const QString title = tr("Permission Request");
        if (const QString question = questionForFeature(feature).arg(securityOrigin.host()); !question.isEmpty()
            && drawer::question(window(), title, question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            page()->setFeaturePermission(securityOrigin, feature,
                                         QWebEnginePage::PermissionGrantedByUser);
        else
            page()->setFeaturePermission(securityOrigin, feature,
                                         QWebEnginePage::PermissionDeniedByUser);
    }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    void WebView::handlePermissionRequested(QWebEnginePermission permission) {
        if (!permission.isValid()) {
            return;
        }

        const QString title = tr("Permission Request");
        const QString question = questionForPermission(permission.permissionType()).arg(permission.origin().host());
        if (!question.isEmpty() && drawer::question(window(), title, question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            permission.grant();
        } else {
            permission.deny();
        }
    }
#endif

    void WebView::handleProxyAuthenticationRequired(const QUrl &, QAuthenticator *auth, const QString &proxyHost) {
        auto *dialog = new QDialog(window());
        dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::PasswordDialog passwordDialog;
        passwordDialog.setupUi(dialog);
        passwordDialog.buttonBox->hide();

        passwordDialog.m_iconLabel->setText(QString());
        QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, window()));
        passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));

        QString introMessage = tr("Connect to proxy \"%1\" using:");
        introMessage = introMessage.arg(proxyHost.toHtmlEscaped());
        passwordDialog.m_infoLabel->setText(introMessage);
        passwordDialog.m_infoLabel->setWordWrap(true);

        if (drawer::execDrawer(this, tr("Proxy Authentication Required"), dialog,
                               {{tr("OK"), QDialog::Accepted, true},
                                {tr("Cancel"), QDialog::Rejected, false}},
                               QDialog::Rejected) == QDialog::Accepted) {
            auth->setUser(passwordDialog.m_userNameLineEdit->text());
            auth->setPassword(passwordDialog.m_passwordLineEdit->text());
        } else {
            // Set authenticator null if dialog is cancelled
            *auth = QAuthenticator();
        }
    }

//! [registerProtocolHandlerRequested]
    void WebView::handleRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request) {
        const auto answer = drawer::question(window(), tr("Permission Request"),
                                             tr("Allow %1 to open all %2 links?")
                                                     .arg(request.origin().host(), request.scheme()),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);
        if (answer == QMessageBox::Yes)
            request.accept();
        else
            request.reject();
    }
//! [registerProtocolHandlerRequested]

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    void WebView::handleFileSystemAccessRequested(QWebEngineFileSystemAccessRequest request) {
        QString accessType;
        switch (request.accessFlags()) {
            case QWebEngineFileSystemAccessRequest::Read:
                accessType = "read";
                break;
            case QWebEngineFileSystemAccessRequest::Write:
                accessType = "write";
                break;
            case QWebEngineFileSystemAccessRequest::Read | QWebEngineFileSystemAccessRequest::Write:
                accessType = "read and write";
                break;
            default:
                Q_UNREACHABLE();
        }

        const auto answer = drawer::question(window(), tr("File system access request"),
                                             tr("Give %1 %2 access to %3?")
                                                     .arg(request.origin().host(), accessType, request.filePath().toString()),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);
        if (answer == QMessageBox::Yes)
            request.accept();
        else
            request.reject();
    }

    WebView::~WebView() {
        if (m_webPage) {
            delete m_webPage;
            m_webPage = nullptr;
        }
    }

#endif // QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
}
