//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_CONNECTION_HPP
#define FAIRWINDSK_CONNECTION_HPP

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QWidget>
#include <QTimer>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QtZeroConf/qzeroconf.h>
#endif
#include "Settings.hpp"
#include "ui/web/WebView.hpp"

class QEvent;
class QFrame;
class QLabel;
class QPushButton;
class QTextEdit;

namespace fairwindsk::ui::widgets {
    class TouchComboBox;
}

namespace fairwindsk::ui::settings {

    class Connection : public QWidget {
    Q_OBJECT

    public:
        explicit Connection(Settings *settingsWidget, QWidget *parent = nullptr);
        ~Connection() override;

    protected:
        bool event(QEvent *event) override;

    private slots:
        void onCheckRequestToken();
        void onRequestToken();
        void onCancelRequest();
        void onRemoveToken();
        void onToggleConnection();
        void handleTokenRequestReply();
        void handleTokenStatusReply();
        void onUpdateSignalKServerUrl();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        void addService(const QZeroConfService &item);
#endif

    private:
        void buildUi();
        void refreshChrome();

        QUrl currentSignalKServerUrl() const;
        void appendMessage(const QString &message) const;
        void stopTokenTimer();
        void startTokenTimer();
        void syncTokenUiState();
        void showConsole();
        void showBrowserPage(const QUrl &url);
        void abortActiveTokenReply();
        void setPendingRequestHref(const QString &href) const;
        void clearPendingRequest(bool clearToken) const;
        void applyCompletedAccessRequest(const QJsonObject &accessRequest);
        void finishTokenFlowWithError(const QString &state, const QString &message);
        void commitSignalKServerUrl(bool restartWhenActive);
        void addServerUrlOption(const QString &serverUrl) const;
        bool connectionEnabled() const;
        void setConnectionEnabled(bool enabled);
        void updateConnectionToggle();
        void setStatusTexts(const QString &state,
                            const QString &permission = {},
                            const QString &expiration = {});
        void updateStatusLabel() const;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        void startZeroConfDiscovery();
        void stopZeroConfDiscovery();
        QString signalKUrlForService(const QZeroConfService &item) const;
        bool isSignalKDiscoveryService(const QZeroConfService &item) const;
#endif

        Settings *m_settings = nullptr;

        // UI widgets (built in code)
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QFrame *m_controlFrame = nullptr;
        QLabel *m_urlLabel = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_comboBox = nullptr;
        QLabel *m_statusLabel = nullptr;
        QPushButton *m_connectButton = nullptr;
        QPushButton *m_requestTokenButton = nullptr;
        QPushButton *m_cancelButton = nullptr;
        QPushButton *m_removeTokenButton = nullptr;
        QTextEdit *m_consoleEdit = nullptr;

        // Persistent state for the combined status label
        QString m_stateText;
        QString m_permissionText;
        QString m_expirationText;

        // Token / connection internals
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QList<QZeroConf *> m_zeroConfBrowsers;
#endif
        QTimer *m_timer = nullptr;
        QNetworkAccessManager *m_networkAccessManager = nullptr;
        QPointer<QNetworkReply> m_activeTokenReply;
        bool m_committingServerUrl = false;

        // Browser drawer state
        QPointer<fairwindsk::ui::web::WebView> m_browserView;
        bool m_browserDrawerOpen = false;
    };

} // fairwindsk::ui::settings

#endif // FAIRWINDSK_CONNECTION_HPP
