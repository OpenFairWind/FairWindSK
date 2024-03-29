//
// Created by Raffaele Montella on 14/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QPushButton>
#include <QWebEngineProfile>

#include <IterableLayoutAdapter.hpp>
#include "Web.hpp"
#include "WebPage.hpp"
#include "ui/topbar/TopBar.hpp"


namespace fairwindsk::ui::web {
    Web::Web(QWidget *parent, fairwindsk::AppItem *appItem, QWebEngineProfile *profile): QWidget(parent), ui(new Ui::Web) {
        m_appItem = appItem;

        ui->setupUi((QWidget *)this);

        showButtons(false);

        connect(ui->toolButton_Home, &QToolButton::clicked, this, &Web::toolButton_home_clicked);


        m_webView = new WebView(profile,(QWidget *)this);

        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        ui->verticalLayout_WebView->addWidget(m_webView);

        if (m_appItem) {
            m_webView->load(QUrl(m_appItem->getUrl()));
        }


    }

    void Web::toggleButtons() {
        showButtons(!ui->toolButton_Home->isVisible());
    }

    void Web::showButtons(bool show) {
        for (auto widget : IterableLayoutAdapter<>(ui->verticalLayout_Buttons)) {
            if (show) {
                widget->show();
            } else {
                widget->hide();
            }

        }
    }


    Web::~Web() {
        /*
        if (m_webPage) {
            delete m_webPage;
            m_webPage = nullptr;
        }
        */
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
