//
// Created by Raffaele Montella on 14/01/22.
//

#ifndef FAIRWINDSK_WEB_HPP
#define FAIRWINDSK_WEB_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include <QtWidgets/QProgressBar>
#include "ui_Web.h"

#include "WebView.hpp"
#include "DownloadManagerWidget.hpp"
#include "NavigationBar.hpp"

namespace Ui { class Web; }

namespace fairwindsk::ui::web {

    class AppItem;

    class Web : public QWidget {
    Q_OBJECT

    public:
        explicit Web(QWidget *parent = nullptr, fairwindsk::AppItem *appItem = nullptr, QWebEngineProfile *profile= nullptr);

        ~Web() override;

        void toggleNavigationBar();

        DownloadManagerWidget &downloadManagerWidget() { return m_downloadManagerWidget; }

        signals:
        void removeApp(QString name);

    public slots:
        void onHomeClicked();
        void onBackClicked();
        void onForwardClicked();
        void onReloadClicked();
        void onSettingsClicked();
        void onCloseClicked();

    private slots:
        void handleWebViewLoadProgress(int);

    private:
        Ui::Web *ui;

        WebView *m_webView = nullptr;
        fairwindsk::AppItem *m_appItem = nullptr;

        NavigationBar *m_NavigationBar;
        DownloadManagerWidget m_downloadManagerWidget;


        QProgressBar *m_progressBar = nullptr;

    };
} // fairwindsk::ui::web

#endif //FAIRWIND_MAINPAGE_HPP
