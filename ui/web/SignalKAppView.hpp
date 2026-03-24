//
// Created by Codex on 24/03/26.
//

#ifndef FAIRWINDSK_UI_WEB_SIGNALKAPPVIEW_HPP
#define FAIRWINDSK_UI_WEB_SIGNALKAPPVIEW_HPP

#include <QUrl>
#include <QWidget>

class QProgressBar;
class QWebEnginePage;
namespace Ui { class SignalKAppView; }

namespace fairwindsk::ui::web {

    class WebView;

    class SignalKAppView final : public QWidget {
        Q_OBJECT

    public:
        explicit SignalKAppView(QWidget *parent = nullptr);
        ~SignalKAppView() override;

        bool loadAppByKeyword(const QString &keyword);
        void loadUrl(const QUrl &url);
        void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
        void clear();
        void runJavaScript(const QString &script);
        QWebEnginePage *page() const;
        QUrl url() const;

    signals:
        void loadFinished(bool ok);

    private:
        static QString detectAppUrlByKeyword(const QString &keyword);

        ::Ui::SignalKAppView *ui = nullptr;
        WebView *m_webView = nullptr;
        QProgressBar *m_progressBar = nullptr;
    };
}

#endif // FAIRWINDSK_UI_WEB_SIGNALKAPPVIEW_HPP
