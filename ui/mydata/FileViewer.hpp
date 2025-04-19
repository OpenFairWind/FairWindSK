//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef JSONVIEWER_H
#define JSONVIEWER_H

#include <QWidget>

namespace fairwindsk::ui::mydata
{
    QT_BEGIN_NAMESPACE
    namespace Ui { class FileViewer; }
    QT_END_NAMESPACE

    class FileViewer : public QWidget {
        Q_OBJECT

        public:
        explicit FileViewer(const QString& path, QWidget *parent = nullptr);
        ~FileViewer() override;

        signals:
        void askedToBeClosed();

        public slots:
            void onCloseClicked();

    private:
        Ui::FileViewer *ui;
    };
}

#endif //JSONVIEWER_H
