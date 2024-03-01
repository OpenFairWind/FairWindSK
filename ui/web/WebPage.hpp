//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBPAGE_HPP
#define FAIRWINDSK_WEBPAGE_HPP


#include <QWebEnginePage>
#include <QWebEngineRegisterProtocolHandlerRequest>

namespace fairwindsk::ui::web {
    class WebPage : public QWebEnginePage {
    Q_OBJECT

    public:
        WebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

    protected:
        bool certificateError(const QWebEngineCertificateError &error); // override;

    private slots:

        void handleAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);

        void handleFeaturePermissionRequested(const QUrl &securityOrigin, Feature feature);

        void handleProxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost);

        void handleRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);

#if !defined(QT_NO_SSL) || QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)

        void handleSelectClientCertificate(QWebEngineClientCertificateSelection clientCertSelection);

#endif
    };
}

#endif //FAIRWINDSK_WEBPAGE_HPP
