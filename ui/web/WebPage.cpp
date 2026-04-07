//
// Created by Raffaele Montella on 28/03/21.
//

/*
#include "browserwindow.h"
#include "tabwidget.h"
*/

#include "WebPage.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "WebView.hpp"
#include <QTimer>

namespace fairwindsk::ui::web {
    namespace {
        QWidget *dialogParentForPage(WebPage *page) {
            if (!page) {
                return nullptr;
            }

            if (auto *widgetParent = qobject_cast<QWidget *>(page->parent())) {
                return widgetParent->window();
            }

            return nullptr;
        }

        QString javaScriptDialogTitle(const QUrl &securityOrigin) {
            const QString host = securityOrigin.host().trimmed();
            if (!host.isEmpty()) {
                return QObject::tr("Message from %1").arg(host);
            }
            return QObject::tr("Web Message");
        }
    }

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

    void WebPage::javaScriptAlert(const QUrl &securityOrigin, const QString &message) {
        drawer::information(dialogParentForPage(this), javaScriptDialogTitle(securityOrigin), message);
    }

    bool WebPage::javaScriptConfirm(const QUrl &securityOrigin, const QString &message) {
        return drawer::question(dialogParentForPage(this),
                                javaScriptDialogTitle(securityOrigin),
                                message,
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::No) == QMessageBox::Yes;
    }

    bool WebPage::javaScriptPrompt(const QUrl &securityOrigin,
                                   const QString &message,
                                   const QString &defaultValue,
                                   QString *result) {
        bool ok = false;
        const QString value = drawer::getText(dialogParentForPage(this),
                                              javaScriptDialogTitle(securityOrigin),
                                              message,
                                              defaultValue,
                                              &ok);
        if (ok && result) {
            *result = value;
        }
        return ok;
    }

    WebPage::~WebPage() = default;

}
