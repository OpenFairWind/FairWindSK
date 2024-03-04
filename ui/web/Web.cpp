//
// Created by Raffaele Montella on 14/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QPushButton>
#include <utility>

#include "Web.hpp"
#include "ui/topbar/TopBar.hpp"


namespace fairwindsk::ui::web {
    Web::Web(QWidget *parent): QWidget(parent), ui(new Ui::Web) {

        ui->setupUi((QWidget *)this);



        m_webView = new WebView((QWidget *)this);
        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        ui->verticalLayout_WebView->addWidget(m_webView);

        connect(ui->pushButton_Home, &QPushButton::clicked, this, &Web::toolButton_home_clicked);

    }

    /*
    void Web::onAdded() {

        auto args = getFairWindApp()->getArgs();
        if (args.contains("Url")) {
            m_url = args["Url"].toString();
        }
        m_webView->load(QUrl(m_url));

    }
    */

    Web::~Web() {
        delete ui;
    }

    void Web::toolButton_home_clicked() {
        m_webView->load(m_url);
    }

    void Web::setUrl(QString url) {
        m_url = std::move(url);
        m_webView->load(m_url);
    }
} // fairwindsk::ui::web
