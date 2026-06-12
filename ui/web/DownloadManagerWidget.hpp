//
// Created by Raffaele Montella on 03/03/24.
//

#ifndef FAIRWINDSK_DOWNLOADMANAGERWIDGET_HPP
#define FAIRWINDSK_DOWNLOADMANAGERWIDGET_HPP

#include "ui_DownloadManagerWidget.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QWebEngineDownloadRequest;
QT_END_NAMESPACE



namespace fairwindsk::ui::web {

    class DownloadWidget;

    // Displays a list of downloads.
    class DownloadManagerWidget final : public QWidget, public Ui::DownloadManagerWidget
    {
    Q_OBJECT
    public:
        explicit DownloadManagerWidget(QWidget *parent = nullptr);

        // Prompts user with a "Save As" dialog. If the user doesn't cancel it, then
        // the QWebEngineDownloadRequest will be accepted and the DownloadManagerWidget
        // will be shown on the screen.
        void downloadRequested(QWebEngineDownloadRequest *webItem);

    private:
        void add(DownloadWidget *downloadWidget);
        void remove(DownloadWidget *downloadWidget);

        int m_numDownloads = 0;
    };
}


#endif //FAIRWINDSK_DOWNLOADMANAGERWIDGET_HPP
