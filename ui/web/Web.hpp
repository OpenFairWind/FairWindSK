//
// Created by Raffaele Montella on 14/01/22.
//

#ifndef FAIRWINDSK_WEB_HPP
#define FAIRWINDSK_WEB_HPP

#include <QMargins>
#include <QWidget>

#include <FairWindSK.hpp>
#include <QtWidgets/QProgressBar>
#include "ui_Web.h"

#include "WebView.hpp"
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include "DownloadManagerWidget.hpp"
#endif
#include "NavigationBar.hpp"

namespace Ui { class Web; }

namespace fairwindsk::ui::web {

    class AppItem;

    class Web : public QWidget {
    Q_OBJECT

    public:
        explicit Web(QWidget *parent = nullptr, fairwindsk::AppItem *appItem = nullptr, fairwindsk::WebProfileHandle *profile = nullptr);

        ~Web() override;

        void toggleNavigationBar();
        void releaseMobileFocus();
        void applyMobileShellMetrics(const QMargins &safeAreaMargins,
                                     int keyboardInset,
                                     bool keyboardVisible,
                                     bool compactMode);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        DownloadManagerWidget &downloadManagerWidget() { return m_downloadManagerWidget; }
#endif

        signals:
        void removeApp(QString name);
        void statusSummaryChanged(const QString &summary, bool degraded);
        void mobileFocusChanged(bool webContentFocused, bool textInputLikely);
        void mobileViewportChanged(const QRect &viewport, bool keyboardVisible);

    public slots:
        void onHomeClicked();
        void onBackClicked();
        void onForwardClicked();
        void onReloadClicked();
        void onZoomOutClicked();
        void onZoomInClicked();
        void onSettingsClicked();
        void onCloseClicked();

    private slots:
        void handleWebViewLoadProgress(int);
        void handleLoadStarted();
        void handleLoadFinished(bool ok);
        void syncNavigationState();

    private:
        void adjustZoomPercent(double delta);
        void applyZoomPercent(double zoomPercent);
        Ui::Web *ui = nullptr;

        WebView *m_webView = nullptr;
        fairwindsk::AppItem *m_appItem = nullptr;

        NavigationBar *m_NavigationBar = nullptr;
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        DownloadManagerWidget m_downloadManagerWidget;
#endif


        QProgressBar *m_progressBar = nullptr;
        bool m_isLoading = false;

    };
} // fairwindsk::ui::web

#endif //FAIRWIND_MAINPAGE_HPP
