//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBPAGE_HPP
#define FAIRWINDSK_WEBPAGE_HPP

#include <QWebEnginePage>
#include <QWebEngineRegisterProtocolHandlerRequest>
#include <QWebEngineCertificateError>

namespace fairwindsk::ui::web {


    class WebPage : public QWebEnginePage
    {
    Q_OBJECT

    public:
        explicit WebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

        ~WebPage() override;

    signals:
        void createCertificateErrorDialog(QWebEngineCertificateError error);

    protected:
        void javaScriptAlert(const QUrl &securityOrigin, const QString &message) override;
        bool javaScriptConfirm(const QUrl &securityOrigin, const QString &message) override;
        bool javaScriptPrompt(const QUrl &securityOrigin,
                              const QString &message,
                              const QString &defaultValue,
                              QString *result) override;

    private slots:
        void handleCertificateError(QWebEngineCertificateError error);
        void handleSelectClientCertificate(QWebEngineClientCertificateSelection clientCertSelection);
    };

}

#endif //FAIRWINDSK_WEBPAGE_HPP
