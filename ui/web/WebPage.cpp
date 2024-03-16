//
// Created by Raffaele Montella on 28/03/21.
//

/*
#include "browserwindow.h"
#include "tabwidget.h"
*/

#include "WebPage.hpp"
#include "WebView.hpp"
#include <QTimer>

namespace fairwindsk::ui::web {
    WebPage::WebPage(QWebEngineProfile *profile, QObject *parent)
            : QWebEnginePage(profile, parent)
    {
        connect(this, &QWebEnginePage::selectClientCertificate, this, &WebPage::handleSelectClientCertificate);
        connect(this, &QWebEnginePage::certificateError, this, &WebPage::handleCertificateError);
    }

    void WebPage::handleCertificateError(QWebEngineCertificateError error)
    {
        error.defer();
        QTimer::singleShot(0, this,
                           [this, error]() mutable { emit createCertificateErrorDialog(error); });
    }

    void WebPage::handleSelectClientCertificate(QWebEngineClientCertificateSelection selection)
    {
        // Just select one.
        selection.select(selection.certificates().at(0));
    }

    WebPage::~WebPage() = default;

}