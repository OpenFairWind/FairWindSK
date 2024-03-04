//
// Created by Raffaele Montella on 03/03/24.
//

#include "Browser.hpp"
#include "BrowserWindow.hpp"

#include <QWebEngineSettings>

using namespace Qt::StringLiterals;

namespace fairwindsk::ui::web {
    Browser::Browser() {
        // Quit application if the download manager window is the only remaining window
        m_downloadManagerWidget.setAttribute(Qt::WA_QuitOnClose, false);

        QObject::connect(
                QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                &m_downloadManagerWidget, &DownloadManagerWidget::downloadRequested);
    }

    BrowserWindow *Browser::createHiddenWindow(bool offTheRecord) {
        if (!offTheRecord && !m_profile) {
            const QString name = u"simplebrowser."_s + QLatin1StringView(qWebEngineChromiumVersion());
            m_profile.reset(new QWebEngineProfile(name));
            m_profile->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
            m_profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
            m_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
            m_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
            QObject::connect(m_profile.get(), &QWebEngineProfile::downloadRequested,
                             &m_downloadManagerWidget, &DownloadManagerWidget::downloadRequested);
        }
        auto profile = !offTheRecord ? m_profile.get() : QWebEngineProfile::defaultProfile();
        auto mainWindow = new BrowserWindow(this, profile, false);
        m_windows.append(mainWindow);
        QObject::connect(mainWindow, &QObject::destroyed, [this, mainWindow]() {
            m_windows.removeOne(mainWindow);
        });
        return mainWindow;
    }

    BrowserWindow *Browser::createWindow(bool offTheRecord) {
        auto *mainWindow = createHiddenWindow(offTheRecord);
        mainWindow->show();
        return mainWindow;
    }

    BrowserWindow *Browser::createDevToolsWindow() {
        auto profile = m_profile ? m_profile.get() : QWebEngineProfile::defaultProfile();
        auto mainWindow = new BrowserWindow(this, profile, true);
        m_windows.append(mainWindow);
        QObject::connect(mainWindow, &QObject::destroyed, [this, mainWindow]() {
            m_windows.removeOne(mainWindow);
        });
        mainWindow->show();
        return mainWindow;
    }
}