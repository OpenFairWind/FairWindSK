//
// Created by Codex on 24/03/26.
//

#include "SignalKAppView.hpp"

#include <QProgressBar>
#include <QVBoxLayout>

#include "AppItem.hpp"
#include "FairWindSK.hpp"
#include "WebView.hpp"
#include "ui_SignalKAppView.h"

namespace fairwindsk::ui::web {

    QString SignalKAppView::detectAppUrlByKeyword(const QString &keyword) {
        const auto fairWindSK = FairWindSK::getInstance();
        if (!fairWindSK || keyword.trimmed().isEmpty()) {
            return {};
        }

        const auto hashes = fairWindSK->getAppsHashes();
        for (const auto &hash : hashes) {
            auto *appItem = fairWindSK->getAppItemByHash(hash);
            if (!appItem || !appItem->getActive()) {
                continue;
            }

            const QStringList candidates = {
                appItem->getName(),
                appItem->getDisplayName(),
                appItem->getUrl()
            };

            for (const auto &candidate : candidates) {
                if (candidate.contains(keyword, Qt::CaseInsensitive)) {
                    return appItem->getUrl();
                }
            }
        }

        return {};
    }

    SignalKAppView::SignalKAppView(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::SignalKAppView) {
        ui->setupUi(this);

        m_progressBar = ui->progressBar;
        m_progressBar->setVisible(false);
        m_progressBar->setRange(0, 100);

        auto *layout = new QVBoxLayout(ui->widgetHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        auto *profile = FairWindSK::getInstance() ? FairWindSK::getInstance()->getWebEngineProfile() : nullptr;
        m_webView = new WebView(profile, this);
        m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_webView);

        connect(m_webView, &WebView::loadStarted, this, [this]() {
            m_progressBar->setValue(0);
            m_progressBar->setVisible(true);
        });
        connect(m_webView, &WebView::loadProgress, this, [this](const int progress) {
            m_progressBar->setValue(progress);
        });
        connect(m_webView, &WebView::loadFinished, this, [this](const bool ok) {
            m_progressBar->setVisible(false);
            emit loadFinished(ok);
        });
    }

    SignalKAppView::~SignalKAppView() {
        delete ui;
    }

    bool SignalKAppView::hasAppByKeyword(const QString &keyword) {
        return !detectAppUrlByKeyword(keyword).isEmpty();
    }

    bool SignalKAppView::loadAppByKeyword(const QString &keyword) {
        const QString url = detectAppUrlByKeyword(keyword);
        if (url.isEmpty()) {
            return false;
        }

        loadUrl(QUrl::fromUserInput(url));
        return true;
    }

    void SignalKAppView::loadUrl(const QUrl &url) {
        if (!m_webView || !url.isValid()) {
            return;
        }

        if (m_webView->url() == url) {
            emit loadFinished(true);
            return;
        }

        m_webView->load(url);
    }

    void SignalKAppView::setHtml(const QString &html, const QUrl &baseUrl) {
        if (!m_webView) {
            return;
        }

        m_webView->setHtml(html, baseUrl);
    }

    void SignalKAppView::clear() {
        setHtml(QString());
    }

    void SignalKAppView::runJavaScript(const QString &script) {
        if (m_webView) {
            m_webView->runJavaScript(script);
        }
    }

    QUrl SignalKAppView::url() const {
        return m_webView ? m_webView->url() : QUrl();
    }
}
