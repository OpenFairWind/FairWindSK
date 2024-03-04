//
// Created by Raffaele Montella on 03/03/24.
//

#ifndef FAIRWINDSK_BROWSER_HPP
#define FAIRWINDSK_BROWSER_HPP

#include "DownloadManagerWidget.hpp"

#include <QList>
#include <QWebEngineProfile>



namespace fairwindsk::ui::web {

    class BrowserWindow;

    class Browser {
    public:
        Browser();

        QList<BrowserWindow *> windows() { return m_windows; }

        BrowserWindow *createHiddenWindow(bool offTheRecord = false);

        BrowserWindow *createWindow(bool offTheRecord = false);

        BrowserWindow *createDevToolsWindow();

        DownloadManagerWidget &downloadManagerWidget() { return m_downloadManagerWidget; }

    private:
        QList<BrowserWindow *> m_windows;
        DownloadManagerWidget m_downloadManagerWidget;
        QScopedPointer<QWebEngineProfile> m_profile;

    };

}

#endif //FAIRWINDSK_BROWSER_HPP
