//
// Created by Raffaele Montella on 28/03/21.
//

/*
#include "browser.h"
#include "browserwindow.h"
#include "tabwidget.h"
 */
#include "WebPage.hpp"
#include "WebPopupWindow.hpp"
#include "WebView.hpp"
#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

namespace fairwindsk::ui::web {
    WebView::WebView(QWidget *parent)
            : QWebEngineView(parent), m_loadProgress(100) {
        connect(this, &QWebEngineView::loadStarted, [this]() {
            m_loadProgress = 0;
            emit favIconChanged(favIcon());
        });
        connect(this, &QWebEngineView::loadProgress, [this](int progress) {
            m_loadProgress = progress;
        });
        connect(this, &QWebEngineView::loadFinished, [this](bool success) {
            m_loadProgress = success ? 100 : -1;
            emit favIconChanged(favIcon());
        });
        connect(this, &QWebEngineView::iconChanged, [this](const QIcon &) {
            emit favIconChanged(favIcon());
        });

        connect(this, &QWebEngineView::renderProcessTerminated,
                [this](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
                    QString status;
                    switch (termStatus) {
                        case QWebEnginePage::NormalTerminationStatus:
                            status = tr("Render process normal exit");
                            break;
                        case QWebEnginePage::AbnormalTerminationStatus:
                            status = tr("Render process abnormal exit");
                            break;
                        case QWebEnginePage::CrashedTerminationStatus:
                            status = tr("Render process crashed");
                            break;
                        case QWebEnginePage::KilledTerminationStatus:
                            status = tr("Render process killed");
                            break;
                    }
                    QMessageBox::StandardButton btn = QMessageBox::question(window(), status,
                                                                            tr("Render process exited with code: %1\n"
                                                                               "Do you want to reload the page ?").arg(
                                                                                    statusCode));
                    if (btn == QMessageBox::Yes)
                        QTimer::singleShot(0, [this] { reload(); });
                });
    }

    void WebView::setPage(WebPage *page) {
        createWebActionTrigger(page, QWebEnginePage::Forward);
        createWebActionTrigger(page, QWebEnginePage::Back);
        createWebActionTrigger(page, QWebEnginePage::Reload);
        createWebActionTrigger(page, QWebEnginePage::Stop);
        QWebEngineView::setPage(page);
    }

    int WebView::loadProgress() const {
        return m_loadProgress;
    }

    void WebView::createWebActionTrigger(QWebEnginePage *page, QWebEnginePage::WebAction webAction) {
        QAction *action = page->action(webAction);
        connect(action, &QAction::changed, [this, action, webAction] {
            emit webActionEnabledChanged(webAction, action->isEnabled());
        });
    }

    bool WebView::isWebActionEnabled(QWebEnginePage::WebAction webAction) const {
        return page()->action(webAction)->isEnabled();
    }

    QIcon WebView::favIcon() const {
        QIcon favIcon = icon();
        if (!favIcon.isNull())
            return favIcon;

        if (m_loadProgress < 0) {
            static QIcon errorIcon(QStringLiteral(":dialog-error.png"));
            return errorIcon;
        } else if (m_loadProgress < 100) {
            static QIcon loadingIcon(QStringLiteral(":view-refresh.png"));
            return loadingIcon;
        } else {
            static QIcon defaultIcon(QStringLiteral(":text-html.png"));
            return defaultIcon;
        }
    }

    QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type) {
        /*
        BrowserWindow *mainWindow = qobject_cast<BrowserWindow*>(window());
        if (!mainWindow)
            return nullptr;

        switch (type) {
            case QWebEnginePage::WebBrowserTab: {
                return mainWindow->tabWidget()->createTab();
            }
            case QWebEnginePage::WebBrowserBackgroundTab: {
                return mainWindow->tabWidget()->createBackgroundTab();
            }
            case QWebEnginePage::WebBrowserWindow: {
                return mainWindow->browser()->createWindow()->currentTab();
            }
            case QWebEnginePage::WebDialog: {
                WebPopupWindow *popup = new WebPopupWindow(page()->profile());
                connect(popup->view(), &WebView::devToolsRequested, this, &WebView::devToolsRequested);
                return popup->view();
            }
        }
         */
        return nullptr;
    }

    void WebView::contextMenuEvent(QContextMenuEvent *event) {
        /*
        QMenu *menu = page()->createStandardContextMenu();
        const QList<QAction *> actions = menu->actions();
        auto inspectElement = std::find(actions.cbegin(), actions.cend(),
                                        page()->action(QWebEnginePage::InspectElement));
        if (inspectElement == actions.cend()) {
            auto viewSource = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::ViewSource));
            if (viewSource == actions.cend())
                menu->addSeparator();

            QAction *action = new QAction(menu);
            action->setText("Open inspector in new window");
            connect(action, &QAction::triggered, [this]() { emit devToolsRequested(page()); });

            QAction *before(inspectElement == actions.cend() ? nullptr : *inspectElement);
            menu->insertAction(before, action);
        } else {
            (*inspectElement)->setText(tr("Inspect element"));
        }
        menu->popup(event->globalPos());
         */
    }
}