//
// Created by Raffaele Montella on 14/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QPushButton>

#include "Web.hpp"
#include "ui/topbar/TopBar.hpp"


namespace fairwindsk::ui::web {

    /*
     * Web
     * Constructor
     */
    Web::Web(QWidget *parent, fairwindsk::AppItem *appItem, fairwindsk::WebProfileHandle *profile): QWidget(parent), ui(new Ui::Web) {

        // Set the application pointer
        m_appItem = appItem;

        // Setup the user interface
        ui->setupUi(this);

        // Create the navigation bar
        m_NavigationBar = new NavigationBar(this);

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
        m_webView = new WebView(profile, this);

        // Set the size policy of the web view widget
        m_webView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        // Add the web view widget to the user interface
        ui->verticalLayout_WebView->addWidget(m_webView);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setTextVisible(false);
        m_progressBar->setMaximumHeight(4);
        m_progressBar->setRange(0, 100);
        m_progressBar->setVisible(false);
        ui->verticalLayout_WebView->insertWidget(0, m_progressBar);

        connect(m_webView, &WebView::loadStarted, this, &Web::handleLoadStarted);
        connect(m_webView, &WebView::loadProgress, this, &Web::handleWebViewLoadProgress);
        connect(m_webView, &WebView::loadFinished, this, &Web::handleLoadFinished);
        connect(m_webView, &WebView::urlChanged, this, [this]() { syncNavigationState(); });

        syncNavigationState();

        // Check uf the application item is consistent
        if (m_appItem) {

            // Check if the debug is active
            if (FairWindSK::getInstance()->isDebug()) {

                // Write a message
                qDebug() << "Application URL: " << m_appItem->getUrl();
            }

            // Set the application home URL
            m_webView->load(QUrl::fromUserInput(m_appItem->getUrl()));
        }


    }

    /*
     * toggleNavigationBar
     * Hide/Show the navigation bar
     */
    void Web::toggleNavigationBar() {
        if (!m_NavigationBar) {
            return;
        }

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
        if (!m_webView || !m_appItem) {
            return;
        }

        // Set the application home page
        m_webView->setUrl(QUrl::fromUserInput(m_appItem->getUrl()));
    }


    /*
     * onBackClicked
     * Handler of the forward button
     */
    void Web::onBackClicked() {
        if (!m_webView) {
            return;
        }

        // Back the web page
        m_webView->back();
    }

    /*
     * onForwardClicked
     * Handler of the forward button
     */
    void Web::onForwardClicked()  {
        if (!m_webView) {
            return;
        }

        // Forward the web page
        m_webView->forward();
    }

    /*
     * onReloadClicked
     * Handler of the reload button
     */
    void Web::onReloadClicked()  {
        if (!m_webView) {
            return;
        }

        if (m_isLoading) {
            m_webView->stop();
        } else {
            m_webView->reload();
        }
    }

    /*
     * onCloseClicked
     * Handler of the settings button
     */
    void Web::onCloseClicked() {
        if (!m_appItem) {
            return;
        }

        const auto hash = FairWindSK::getInstance()->getAppHashById(m_appItem->getName());

        // Emit the signal
        emit removeApp(hash.isEmpty() ? m_appItem->getName() : hash);
    }

    /*
     * onSettingsClicked
     * Handler of the settings button
     */
    void Web::onSettingsClicked()  {
        if (!m_webView || !m_appItem) {
            return;
        }

        // Get th settings application URL
        const QString settingsUrl = m_appItem->getSettingsUrl(FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl()+"/admin/#/serverConfiguration/plugins/");

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "------------> settingsUrl: " << settingsUrl;
        }

        // Se the url as the settings URL
        if (!settingsUrl.isEmpty()) {
            m_webView->setUrl(QUrl::fromUserInput(settingsUrl));
        }
    }

    void Web::handleWebViewLoadProgress(int progress) {
        if (!m_progressBar) {
            return;
        }

        if (0 < progress && progress < 100) {
            m_progressBar->setVisible(true);
            m_progressBar->setValue(progress);
        } else {
            m_progressBar->setValue(0);
            m_progressBar->setVisible(false);
        }
    }

    void Web::handleLoadStarted() {
        m_isLoading = true;
        if (m_progressBar) {
            m_progressBar->setVisible(true);
            m_progressBar->setValue(0);
        }
        if (m_NavigationBar) {
            m_NavigationBar->setReloadActive(true);
        }
        syncNavigationState();
    }

    void Web::handleLoadFinished(bool ok) {
        Q_UNUSED(ok);
        m_isLoading = false;
        if (m_progressBar) {
            m_progressBar->setVisible(false);
            m_progressBar->setValue(0);
        }
        if (m_NavigationBar) {
            m_NavigationBar->setReloadActive(false);
        }
        syncNavigationState();
    }

    void Web::syncNavigationState() {
        if (!m_NavigationBar || !m_webView) {
            return;
        }

        m_NavigationBar->setBackEnabled(m_webView->canGoBack());
        m_NavigationBar->setForwardEnabled(m_webView->canGoForward());
        m_NavigationBar->setHomeEnabled(m_appItem != nullptr && !m_appItem->getUrl().isEmpty());
        m_NavigationBar->setSettingsEnabled(m_appItem != nullptr && !m_appItem->getSettingsUrl(FairWindSK::getInstance()->getConfiguration()->getSignalKServerUrl()+"/admin/#/serverConfiguration/plugins/").isEmpty());
    }
} // fairwindsk::ui::web
