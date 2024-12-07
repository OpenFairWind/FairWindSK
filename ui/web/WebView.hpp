//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBVIEW_HPP
#define FAIRWINDSK_WEBVIEW_HPP

#include <QIcon>
#include <QWebEngineView>
#include <QWebEngineCertificateError>
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <QWebEngineFileSystemAccessRequest>
#endif


#include <QWebEngineRegisterProtocolHandlerRequest>

#include "ui_CertificateErrorDialog.h"
#include "ui_PasswordDialog.h"
#include "WebPage.hpp"


namespace fairwindsk::ui::web {

    class WebView : public QWebEngineView {
    Q_OBJECT

    public:
        // Constructor
        explicit WebView(QWebEngineProfile *profile, QWidget *parent = nullptr);

        // Set the web page pointer
        void setPage(WebPage *page);

        // Get the load progress
        int loadProgress() const;

        // Destructor
        ~WebView() override;

    protected:


    signals:



    private slots:

        // Certificate error handler
        void handleCertificateError(QWebEngineCertificateError error);

        // Authentication required handler
        void handleAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);

        // Feature Permission Requested handler
        void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);

        // Proxy Authentication Required handler
        void handleProxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost);

        // Register Protocol Handler Requested handler
        void handleRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

        // File System Access Requested handler
        void handleFileSystemAccessRequested(QWebEngineFileSystemAccessRequest request);

#endif

    private:

    private:
        // Load progress (0-100)
        int m_loadProgress = 100;

        // Web page pointer
        WebPage *m_webPage = nullptr;
    };
}

#endif //FAIRWINDSK_WEBVIEW_HPP
