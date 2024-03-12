//
// Created by Raffaele Montella on 14/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QPushButton>
#include <QWebEngineProfile>

#include <utility>

#include "Web.hpp"
#include "WebPage.hpp"
#include "ui/topbar/TopBar.hpp"


namespace fairwindsk::ui::web {
    Web::Web(QWidget *parent, fairwindsk::AppItem *appItem, QWebEngineProfile *profile): QWidget(parent), ui(new Ui::Web) {

        m_appItem = appItem;

        ui->setupUi((QWidget *)this);

        m_webView = new WebView((QWidget *)this);
        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        m_webPage = new WebPage(profile);
        m_webView->setPage(m_webPage);

        ui->verticalLayout_WebView->addWidget(m_webView);

        showButtons(false);

        connect(ui->pushButton_Home, &QPushButton::clicked, this, &Web::toolButton_home_clicked);

        m_webView->load(QUrl(appItem->getUrl()));

    }

/*
    void Web::setApp(fairwindsk::AppItem *appItem) {
        m_appItem = appItem;

        m_webView->load(QUrl(appItem->getUrl()));

    }
*/
    void Web::toggleButtons() {
        showButtons(!ui->pushButton_Home->isVisible());
    }

    void Web::showButtons(bool show) {
        if (show) {
            ui->pushButton_Home->show();
            ui->pushButton_Next->show();
            ui->pushButton_Prev->show();
            ui->pushButton_Reload->show();
        } else {
            ui->pushButton_Home->hide();
            ui->pushButton_Next->hide();
            ui->pushButton_Prev->hide();
            ui->pushButton_Reload->hide();
        }
    }


    Web::~Web() {
        if (m_webPage) {
            delete m_webPage;
            m_webPage = nullptr;
        }

        if (m_webView) {
            delete m_webView;
            m_webView = nullptr;
        }

        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }

    void Web::toolButton_home_clicked() {
        goHome();
    }

    void Web::goHome() {
        m_webView->setUrl(m_appItem->getUrl());
    }

    void Web::handleWebViewLoadProgress(int progress) {
        //static QIcon stopIcon(u":process-stop.png"_s);
        //static QIcon reloadIcon(u":view-refresh.png"_s);

        if (0 < progress && progress < 100) {
            m_progressBar->setValue(progress);
        } else {
            m_progressBar->setValue(0);
        }
    }
} // fairwindsk::ui::web
