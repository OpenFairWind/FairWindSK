//
// Created by Raffaele Montella on 03/03/24.
//

#ifndef FAIRWINDSK_DOWNLOADWIDGET_HPP
#define FAIRWINDSK_DOWNLOADWIDGET_HPP


#include "ui_downloadwidget.h"

#include <QFrame>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QWebEngineDownloadRequest;
QT_END_NAMESPACE

namespace fairwindsk::ui::web {
// Displays one ongoing or finished download (QWebEngineDownloadRequest).
    class DownloadWidget final : public QFrame, public Ui::DownloadWidget {
    Q_OBJECT

    public:
        // Precondition: The QWebEngineDownloadRequest has been accepted.
        explicit DownloadWidget(QWebEngineDownloadRequest *download, QWidget *parent = nullptr);

    signals:

        // This signal is emitted when the user indicates that they want to remove
        // this download from the downloads list.
        void removeClicked(DownloadWidget *self);

    private slots:

        void updateWidget();

    private:
        QString withUnit(qreal bytes);

        QWebEngineDownloadRequest *m_download;
        QElapsedTimer m_timeAdded;
    };
}

#endif //FAIRWINDSK_DOWNLOADWIDGET_HPP
