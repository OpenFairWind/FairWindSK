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

    signals:
        void createCertificateErrorDialog(QWebEngineCertificateError error);

    private slots:
        void handleCertificateError(QWebEngineCertificateError error);
        void handleSelectClientCertificate(QWebEngineClientCertificateSelection clientCertSelection);
    };

}

#endif //FAIRWINDSK_WEBPAGE_HPP
