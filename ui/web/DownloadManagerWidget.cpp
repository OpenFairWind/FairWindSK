//
// Created by Raffaele Montella on 03/03/24.
//

#include "DownloadManagerWidget.hpp"

#include "Browser.hpp"
#include "BrowserWindow.hpp"
#include "DownloadWidget.hpp"

#include <QFileDialog>
#include <QDir>
#include <QWebEngineDownloadRequest>


namespace fairwindsk::ui::web {
    DownloadManagerWidget::DownloadManagerWidget(QWidget *parent)
            : QWidget(parent) {
        setupUi(this);
    }

    void DownloadManagerWidget::downloadRequested(QWebEngineDownloadRequest *download) {
        Q_ASSERT(download && download->state() == QWebEngineDownloadRequest::DownloadRequested);

        QString path = QFileDialog::getSaveFileName(this, tr("Save as"), QDir(download->downloadDirectory()).filePath(
                download->downloadFileName()));
        if (path.isEmpty())
            return;

        download->setDownloadDirectory(QFileInfo(path).path());
        download->setDownloadFileName(QFileInfo(path).fileName());
        download->accept();
        add(new DownloadWidget(download));

        show();
    }

    void DownloadManagerWidget::add(DownloadWidget *downloadWidget) {
        connect(downloadWidget, &DownloadWidget::removeClicked, this, &DownloadManagerWidget::remove);
        m_itemsLayout->insertWidget(0, downloadWidget, 0, Qt::AlignTop);
        if (m_numDownloads++ == 0)
            m_zeroItemsLabel->hide();
    }

    void DownloadManagerWidget::remove(DownloadWidget *downloadWidget) {
        m_itemsLayout->removeWidget(downloadWidget);
        downloadWidget->deleteLater();
        if (--m_numDownloads == 0)
            m_zeroItemsLabel->show();
    }
}