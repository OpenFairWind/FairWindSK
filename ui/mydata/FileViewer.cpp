//
// Created by Raffaele Montella on 06/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_JsonViewer.h" resolved

#include "FileViewer.hpp"

#include "ui_FileViewer.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QSaveFile>
#include <QSet>
#include <QUrl>

#include "ui/DrawerDialogHost.hpp"

namespace fairwindsk::ui::mydata {

    FileViewer::FileViewer(const QString& path, QWidget *parent): QWidget(parent), ui(new Ui::FileViewer), m_path(path) {
        ui->setupUi(this);

        connect(ui->toolButton_Close, &QToolButton::clicked, this, &FileViewer::onCloseClicked);
        connect(ui->toolButton_Save, &QToolButton::clicked, this, &FileViewer::onSaveClicked);

        ui->label_filePath->setText(path);
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

        loadFile();
    }

    void FileViewer::loadFile() {
        const QFileInfo fileInfo(m_path);
        const QMimeDatabase mimeDatabase;
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo, QMimeDatabase::MatchContent);

        if (isEditableTextFile(mimeType)) {
            loadEditableTextFile();
            return;
        }

        loadPreview();
    }

    void FileViewer::loadEditableTextFile() {
        QFile file(m_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            showError(tr("Unable to open %1 for reading.").arg(m_path));
            loadPreview();
            return;
        }

        ui->plainTextEdit->setPlainText(QString::fromUtf8(file.readAll()));
        ui->stackedWidget->setCurrentWidget(ui->page_Editor);
        ui->toolButton_Save->setVisible(true);
    }

    void FileViewer::loadPreview() {
        ui->toolButton_Save->setVisible(false);
        ui->stackedWidget->setCurrentWidget(ui->page_Preview);
        ui->webEngineView->load(QUrl::fromLocalFile(m_path));
    }

    bool FileViewer::isEditableTextFile(const QMimeType &mimeType) const {
        static const QSet<QString> editableMimeTypes = {
                "application/json",
                "application/xml",
                "application/x-yaml",
                "application/javascript",
                "application/ecmascript",
                "application/x-shellscript",
                "inode/x-empty"
        };

        static const QSet<QString> editableSuffixes = {
                "txt", "log", "json", "xml", "yaml", "yml", "csv", "md", "markdown",
                "ini", "conf", "cfg", "sh", "js", "ts", "cpp", "hpp", "c", "h", "qml", "html", "htm"
        };

        if (mimeType.name().startsWith("text/") || editableMimeTypes.contains(mimeType.name())) {
            return true;
        }

        return editableSuffixes.contains(QFileInfo(m_path).suffix().toLower());
    }

    void FileViewer::onCloseClicked() {
        emit askedToBeClosed();
    }

    void FileViewer::onSaveClicked() {
        QSaveFile file(m_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            showError(tr("Unable to open %1 for writing.").arg(m_path));
            return;
        }

        const QByteArray content = ui->plainTextEdit->toPlainText().toUtf8();
        if (file.write(content) != content.size() || !file.commit()) {
            showError(tr("Unable to save %1.").arg(m_path));
        }
    }

    void FileViewer::showError(const QString &message) const {
        drawer::warning(const_cast<FileViewer *>(this), tr("File Viewer"), message);
    }

    FileViewer::~FileViewer() {
        delete ui;
    }
}
