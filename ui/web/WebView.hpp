//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBVIEW_HPP
#define FAIRWINDSK_WEBVIEW_HPP

#include <QIcon>
#include <QRect>
#include <QTimer>
#include <QUrl>
#include <QWidget>

#include "FairWindSK.hpp"

class QWebEngineView;
class QQuickWidget;

namespace fairwindsk::ui::web {

    class WebView : public QWidget {
        Q_OBJECT

    public:
        enum class HealthState {
            Normal,
            Restarting,
            Failed,
            Unsupported
        };
        Q_ENUM(HealthState)

        explicit WebView(fairwindsk::WebProfileHandle *profile, QWidget *parent = nullptr);
        ~WebView() override;

        void load(const QUrl &url);
        void setUrl(const QUrl &url);
        QUrl url() const;
        void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
        void reload();
        void stop();
        void back();
        void forward();
        bool canGoBack() const;
        bool canGoForward() const;
        void runJavaScript(const QString &script);
        double zoomPercent() const;
        void setZoomPercent(double zoomPercent);

    signals:
        void loadStarted();
        void loadProgress(int progress);
        void loadFinished(bool ok);
        void urlChanged(const QUrl &url);
        void titleChanged(const QString &title);
        void geometryChangeRequested(const QRect &newGeometry);
        void windowCloseRequested();
        void healthStateChanged(fairwindsk::ui::web::WebView::HealthState state, const QString &summary);

    private slots:
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void handleMobileLoadStarted();
        void handleMobileLoadProgressChanged(int progress);
        void handleMobileLoadFinished(bool ok);
        void handleMobileCurrentUrlNotified(const QString &url);
        void handleMobileTitleChanged(const QString &title);
#endif
        void handleConnectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void handleServerStateResynchronized(bool recoveredFromDisconnect);
        void handleLoadTimeout();

    private:
        void initializeDesktop(fairwindsk::WebProfileHandle *profile);
        void initializeMobile();
        void applyZoom();
        static QString zoomScript(double zoomPercent);
        void showSignalKRestartPlaceholder();
        void showFallbackPlaceholder(HealthState state, const QString &title, const QString &body, const QUrl &resumeUrl = QUrl());
        void setHealthState(HealthState state, const QString &summary);
        void startLoadTimeoutWatch();
        void stopLoadTimeoutWatch();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        void setDesktopPage(class WebPage *page);
#endif

        int m_loadProgress = 100;
        QWidget *m_viewWidget = nullptr;
        double m_zoomPercent = 100.0;
        QUrl m_requestedUrl;
        QUrl m_restartResumeUrl;
        bool m_restartPlaceholderVisible = false;
        HealthState m_healthState = HealthState::Normal;
        QString m_healthSummary = QStringLiteral("Web view ready");
        QTimer m_loadTimeoutTimer;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        QWebEngineView *m_desktopView = nullptr;
        class WebPage *m_webPage = nullptr;
#else
        QQuickWidget *m_quickView = nullptr;
        QObject *m_quickRoot = nullptr;
        QUrl m_currentUrl;
        bool m_canGoBack = false;
        bool m_canGoForward = false;
#endif
    };
}

#endif // FAIRWINDSK_WEBVIEW_HPP
