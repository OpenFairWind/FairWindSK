//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBVIEW_HPP
#define FAIRWINDSK_WEBVIEW_HPP

#include <QIcon>
#include <QWebEngineView>

class WebPage;

namespace fairwindsk::ui::web {
    class WebView : public QWebEngineView {
    Q_OBJECT

    public:
        WebView(QWidget *parent = nullptr);

        void setPage(WebPage *page);

        int loadProgress() const;

        bool isWebActionEnabled(QWebEnginePage::WebAction webAction) const;

        QIcon favIcon() const;

    public slots:

    protected:
        void contextMenuEvent(QContextMenuEvent *event) override;

        QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

    signals:

        void webActionEnabledChanged(QWebEnginePage::WebAction webAction, bool enabled);

        void favIconChanged(const QIcon &icon);

        void devToolsRequested(QWebEnginePage *source);


    private:
        void createWebActionTrigger(QWebEnginePage *page, QWebEnginePage::WebAction);

    private:
        int m_loadProgress;
    };
}

#endif //FAIRWINDSK_WEBVIEW_HPP
