//
// Created by Raffaele Montella on 14/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QPushButton>
#include <QWebEngineProfile>


#include "Web.hpp"
#include "WebPage.hpp"
#include "ui/topbar/TopBar.hpp"


namespace fairwindsk::ui::web {
    Web::Web(QWidget *parent, fairwindsk::AppItem *appItem, QWebEngineProfile *profile): QWidget(parent), ui(new Ui::Web) {
        m_appItem = appItem;

        ui->setupUi((QWidget *)this);

        m_NavigationBar = new NavigationBar();
        m_NavigationBar->setVisible(false);
        ui->verticalLayout_NavigationBar->addWidget(m_NavigationBar);

        connect(m_NavigationBar, &NavigationBar::home, this, &Web::onHomeClicked);
        connect(m_NavigationBar, &NavigationBar::back, this, &Web::onBackClicked);
        connect(m_NavigationBar, &NavigationBar::forward, this, &Web::onForwardClicked);
        connect(m_NavigationBar, &NavigationBar::reload, this, &Web::onReloadClicked);
        connect(m_NavigationBar, &NavigationBar::settings, this, &Web::onSettingsClicked);

        m_webView = new WebView(profile,(QWidget *)this);

        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        ui->verticalLayout_WebView->addWidget(m_webView);

        if (m_appItem) {
            qDebug() << "::::::::::::: " << m_appItem->getUrl();
            m_webView->load(QUrl(m_appItem->getUrl()));
        }


    }

    void Web::toggleNavigationBar() {
        auto status = !m_NavigationBar->isVisible();
        m_NavigationBar->setVisible(status);
    }

    Web::~Web() {

        if (m_NavigationBar) {
            delete m_NavigationBar;
            m_NavigationBar = nullptr;
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

    void Web::onHomeClicked()  {
        m_webView->setUrl(m_appItem->getUrl());
    }


    void Web::onBackClicked() {
        m_webView->back();
    }

    void Web::onForwardClicked()  {
        m_webView->forward();
    }

    void Web::onReloadClicked()  {
        m_webView->reload();
    }

    void Web::onSettingsClicked()  {
        QString settingsUrl = m_appItem->getSettingsUrl(FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl()+"/admin/#/serverConfiguration/plugins/");

        qDebug() << "------------> settingsUrl: " << settingsUrl;
        m_webView->setUrl(settingsUrl);
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
