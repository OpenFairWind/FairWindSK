//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef JSONVIEWER_H
#define JSONVIEWER_H

#include <QWidget>
#include <QMimeType>

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
            void onSaveClicked();

    private:
        void loadFile();
        void loadEditableTextFile();
        void loadPreview();
        bool isEditableTextFile(const QMimeType &mimeType) const;
        void showError(const QString &message) const;

        Ui::FileViewer *ui;
        QString m_path;
    };
}

#endif //JSONVIEWER_H
