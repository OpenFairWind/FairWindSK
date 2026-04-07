//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_CONNECTION_HPP
#define FAIRWINDSK_CONNECTION_HPP

#include <QNetworkAccessManager>
#include <QPointer>
#include <QWidget>
#include <QTimer>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QtZeroConf/qzeroconf.h>
#endif
#include "Settings.hpp"
#include "ui/web/WebView.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Connection; }
    QT_END_NAMESPACE

    class Connection : public QWidget {
    Q_OBJECT

    public:
        explicit Connection(Settings *settingsWidget, QWidget *parent = nullptr);

        ~Connection() override;


    private slots:

        void onCheckRequestToken();
        void onRequestToken();
        void onCancelRequest();
        void onReadOnly();
        void onRemoveToken();
        void handleTokenRequestReply();
        void handleTokenStatusReply();

        void onUpdateSignalKServerUrl() const;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        void addService(const QZeroConfService& item) const;
#endif



    private:
        QUrl currentSignalKServerUrl() const;
        void appendMessage(const QString &message) const;
        void stopTokenTimer();
        void startTokenTimer();
        void syncTokenUiState();
        void showConsole() const;
        void showBrowserPage(const QUrl &url) const;
        void abortActiveTokenReply();
        void setPendingRequestHref(const QString &href) const;
        void clearPendingRequest(bool clearToken) const;
        void applyCompletedAccessRequest(const QJsonObject &accessRequest);
        void finishTokenFlowWithError(const QString &state, const QString &message);

        Ui::Connection *ui = nullptr;
        Settings *m_settings = nullptr;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QZeroConf m_zeroConf;
#endif
        QTimer *m_timer = nullptr;
        fairwindsk::ui::web::WebView *m_browserView = nullptr;
        QNetworkAccessManager *m_networkAccessManager = nullptr;
        QPointer<QNetworkReply> m_activeTokenReply;

    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_CONNECTION_HPP
