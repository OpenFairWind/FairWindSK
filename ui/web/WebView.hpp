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
        explicit WebView(QWebEngineProfile *profile, QWidget *parent = nullptr);

        void setPage(WebPage *page);

        int loadProgress() const;

        ~WebView() override;



    protected:


    signals:



    private slots:

        void handleCertificateError(QWebEngineCertificateError error);

        void handleAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);

        void handleFeaturePermissionRequested(const QUrl &securityOrigin,
                                              QWebEnginePage::Feature feature);

        void handleProxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth,
                                               const QString &proxyHost);

        void handleRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

        void handleFileSystemAccessRequested(QWebEngineFileSystemAccessRequest request);

#endif

    private:

    private:
        int m_loadProgress = 100;
        WebPage *m_webPage = nullptr;
    };
}

#endif //FAIRWINDSK_WEBVIEW_HPP
