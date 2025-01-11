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

    /*
     * Web
     * Constructor
     */
    Web::Web(QWidget *parent, fairwindsk::AppItem *appItem, QWebEngineProfile *profile): QWidget(parent), ui(new Ui::Web) {

        // Set the application pointer
        m_appItem = appItem;

        // Setup the user interface
        ui->setupUi((QWidget *)this);

        // Create the navigation bar
        m_NavigationBar = new NavigationBar();

        // Hide the navigation bar
        m_NavigationBar->setVisible(false);

        // Add the navigation bar to the user interface
        ui->verticalLayout_NavigationBar->addWidget(m_NavigationBar);

        // Connect the home button handler
        connect(m_NavigationBar, &NavigationBar::home, this, &Web::onHomeClicked);

        // Connect the back button handler
        connect(m_NavigationBar, &NavigationBar::back, this, &Web::onBackClicked);

        // Connect the forward button handler
        connect(m_NavigationBar, &NavigationBar::forward, this, &Web::onForwardClicked);

        // Connect the reload button handler
        connect(m_NavigationBar, &NavigationBar::reload, this, &Web::onReloadClicked);

        // Connect the settings button handler
        connect(m_NavigationBar, &NavigationBar::settings, this, &Web::onSettingsClicked);

        // Connect the close button handler
        connect(m_NavigationBar, &NavigationBar::close, this, &Web::onCloseClicked);

        // Create the web view widget
        m_webView = new WebView(profile,(QWidget *)this);

        // Set the size policy of the web view widget
        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        // Add the web view widget to the user interface
        ui->verticalLayout_WebView->addWidget(m_webView);

        // Check uf the application item is consistent
        if (m_appItem) {

            // Check if the debug is active
            if (FairWindSK::getInstance()->isDebug()) {

                // Write a message
                qDebug() << "Application URL: " << m_appItem->getUrl();
            }

            // Set the application home URL
            m_webView->load(QUrl(m_appItem->getUrl()));
        }


    }

    /*
     * toggleNavigationBar
     * Hide/Show the navigation bar
     */
    void Web::toggleNavigationBar() {

        // Get the status of the navigation bar and negate it
        auto status = !m_NavigationBar->isVisible();

        // Set the new navigation bar status
        m_NavigationBar->setVisible(status);
    }

    /*
     * ~Web
     * Destructor
     */
    Web::~Web() {

        // Check if the navigation bar is allocated
        if (m_NavigationBar) {

            // Delete the navigation bar
            delete m_NavigationBar;

            // Set the navigation bar pointer to null
            m_NavigationBar = nullptr;
        }

        // Check if the web view is allocated
        if (m_webView) {

            // Delete the web view
            delete m_webView;

            // Set the web view pointer to null
            m_webView = nullptr;
        }

        // Check if the UI is allocated
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the UI pointer to null
            ui = nullptr;
        }
    }

    /*
     * onHomeClicked
     * Handler of the home button
     */
    void Web::onHomeClicked()  {

        // Set the application home page
        m_webView->setUrl(m_appItem->getUrl());
    }


    /*
     * onBackClicked
     * Handler of the forward button
     */
    void Web::onBackClicked() {

        // Back the web page
        m_webView->back();
    }

    /*
     * onForwardClicked
     * Handler of the forward button
     */
    void Web::onForwardClicked()  {

        // Forward the web page
        m_webView->forward();
    }

    /*
     * onReloadClicked
     * Handler of the reload button
     */
    void Web::onReloadClicked()  {

        // Reload the web page
        m_webView->reload();
    }

    /*
     * onCloseClicked
     * Handler of the settings button
     */
    void Web::onCloseClicked() {

        // Emit the signal
        emit removeApp(m_appItem->getName());
    }

    /*
     * onSettingsClicked
     * Handler of the settings button
     */
    void Web::onSettingsClicked()  {

        // Get th settings application URL
        const QString settingsUrl = m_appItem->getSettingsUrl(FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl()+"/admin/#/serverConfiguration/plugins/");

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "------------> settingsUrl: " << settingsUrl;
        }

        // Se the url as the settings URL
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
