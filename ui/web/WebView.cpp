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
                    QMessageBox::StandardButton btn = QMessageBox::question(window(), status,
                                                                            tr("Render process exited with code: %1\n"
                                                                               "Do you want to reload the page ?").arg(statusCode));
                    if (btn == QMessageBox::Yes)
                        QTimer::singleShot(0, this, &WebView::reload);
                });

        // Create the web page
        m_webPage = new WebPage(profile);

        // Set the web page
        setPage(m_webPage);
    }

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

    void WebView::setPage(WebPage *page)
    {
        if (auto oldPage = qobject_cast<WebPage *>(QWebEngineView::page())) {
            disconnect(oldPage, &WebPage::createCertificateErrorDialog, this,
                       &WebView::handleCertificateError);
            disconnect(oldPage, &QWebEnginePage::authenticationRequired, this,
                       &WebView::handleAuthenticationRequired);
            disconnect(oldPage, &QWebEnginePage::featurePermissionRequested, this,
                       &WebView::handleFeaturePermissionRequested);
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
        connect(page, &WebPage::createCertificateErrorDialog, this, &WebView::handleCertificateError);
        connect(page, &QWebEnginePage::authenticationRequired, this,
                &WebView::handleAuthenticationRequired);
        connect(page, &QWebEnginePage::featurePermissionRequested, this,
                &WebView::handleFeaturePermissionRequested);
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
        QDialog dialog(window());
        dialog.setModal(true);
        dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::CertificateErrorDialog certificateDialog;
        certificateDialog.setupUi(&dialog);
        certificateDialog.m_iconLabel->setText(QString());
        const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, window()));
        certificateDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));
        certificateDialog.m_errorLabel->setText(error.description());
        dialog.setWindowTitle(tr("Certificate Error"));

        if (dialog.exec() == QDialog::Accepted)
            error.acceptCertificate();
        else
            error.rejectCertificate();
    }

    void WebView::handleAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth) {
        QDialog dialog(window());
        dialog.setModal(true);
        dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::PasswordDialog passwordDialog;
        passwordDialog.setupUi(&dialog);

        passwordDialog.m_iconLabel->setText(QString());
        const QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, window()));
        passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));

        QString introMessage(tr("Enter username and password for \"%1\" at %2")
                                     .arg(auth->realm(),
                                          requestUrl.toString().toHtmlEscaped()));
        passwordDialog.m_infoLabel->setText(introMessage);
        passwordDialog.m_infoLabel->setWordWrap(true);

        if (dialog.exec() == QDialog::Accepted) {
            auth->setUser(passwordDialog.m_userNameLineEdit->text());
            auth->setPassword(passwordDialog.m_passwordLineEdit->text());
        } else {
            // Set authenticator null if dialog is cancelled
            *auth = QAuthenticator();
        }
    }

    void WebView::handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
        const QString title = tr("Permission Request");
        if (const QString question = questionForFeature(feature).arg(securityOrigin.host()); !question.isEmpty() && QMessageBox::question(window(), title, question) == QMessageBox::Yes)
            page()->setFeaturePermission(securityOrigin, feature,
                                         QWebEnginePage::PermissionGrantedByUser);
        else
            page()->setFeaturePermission(securityOrigin, feature,
                                         QWebEnginePage::PermissionDeniedByUser);
    }

    void WebView::handleProxyAuthenticationRequired(const QUrl &, QAuthenticator *auth, const QString &proxyHost) {
        QDialog dialog(window());
        dialog.setModal(true);
        dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        Ui::PasswordDialog passwordDialog;
        passwordDialog.setupUi(&dialog);

        passwordDialog.m_iconLabel->setText(QString());
        QIcon icon(window()->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, window()));
        passwordDialog.m_iconLabel->setPixmap(icon.pixmap(32, 32));

        QString introMessage = tr("Connect to proxy \"%1\" using:");
        introMessage = introMessage.arg(proxyHost.toHtmlEscaped());
        passwordDialog.m_infoLabel->setText(introMessage);
        passwordDialog.m_infoLabel->setWordWrap(true);

        if (dialog.exec() == QDialog::Accepted) {
            auth->setUser(passwordDialog.m_userNameLineEdit->text());
            auth->setPassword(passwordDialog.m_passwordLineEdit->text());
        } else {
            // Set authenticator null if dialog is cancelled
            *auth = QAuthenticator();
        }
    }

//! [registerProtocolHandlerRequested]
    void WebView::handleRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request) {
        const auto answer = QMessageBox::question(window(), tr("Permission Request"),
                                            tr("Allow %1 to open all %2 links?")
                                                    .arg(request.origin().host(), request.scheme()));
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

        const auto answer = QMessageBox::question(window(), tr("File system access request"),
                                            tr("Give %1 %2 access to %3?")
                                                    .arg(request.origin().host(), accessType, request.filePath().toString()));
        if (answer == QMessageBox::Yes)
            request.accept();
        else
            request.reject();
    }

    WebView::~WebView() {
        // Check the web page is allocated
        if (m_webPage)
        {
            // Delete the web page
            delete m_webPage;

            // Set the web page pointer to null
            m_webPage = nullptr;
        }
    }

#endif // QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
}