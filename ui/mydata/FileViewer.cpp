//
// Created by Raffaele Montella on 06/03/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_JsonViewer.h" resolved

#include "FileViewer.hpp"

#include "ui_FileViewer.h"

#include "FairWindSK.hpp"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QSaveFile>
#include <QSet>
#include <QUrl>
#include <QVBoxLayout>

#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"
#include "ui/web/WebView.hpp"

namespace fairwindsk::ui::mydata {

    FileViewer::FileViewer(const QString& path, QWidget *parent): QWidget(parent), ui(new Ui::FileViewer), m_path(path) {
        ui->setupUi(this);

        retintToolButtons();

        connect(ui->toolButton_Close, &QToolButton::clicked, this, &FileViewer::onCloseClicked);
        connect(ui->toolButton_Save, &QToolButton::clicked, this, &FileViewer::onSaveClicked);
        connect(ui->plainTextEdit->document(), &QTextDocument::modificationChanged, this, [this](const bool modified) {
            ui->toolButton_Save->setEnabled(m_isEditableTextMode && modified);
            ui->toolButton_Save->setToolTip(modified ? tr("Save changes") : tr("No pending changes"));
        });

        ui->label_filePath->setText(path);
        ui->label_filePath->setToolTip(path);
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        ui->toolButton_Close->setToolTip(tr("Close file viewer"));
        ui->toolButton_Save->setToolTip(tr("No pending changes"));

        auto *previewLayout = new QVBoxLayout(ui->widgetPreviewHost);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(0);
        m_previewView = new fairwindsk::ui::web::WebView(
            FairWindSK::getInstance() ? FairWindSK::getInstance()->getWebEngineProfile() : nullptr,
            ui->widgetPreviewHost);
        previewLayout->addWidget(m_previewView);

        loadFile();
    }

    void FileViewer::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
        }
    }

    void FileViewer::retintToolButtons() const {
        const QColor iconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        fairwindsk::ui::applyTintedButtonIcon(ui->toolButton_Close, iconColor);
        fairwindsk::ui::applyTintedButtonIcon(ui->toolButton_Save, iconColor);
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
        ui->plainTextEdit->document()->setModified(false);
        ui->stackedWidget->setCurrentWidget(ui->page_Editor);
        ui->toolButton_Save->setVisible(true);
        ui->toolButton_Save->setEnabled(false);
        m_isEditableTextMode = true;
    }

    void FileViewer::loadPreview() {
        ui->toolButton_Save->setVisible(false);
        ui->toolButton_Save->setEnabled(false);
        ui->stackedWidget->setCurrentWidget(ui->page_Preview);
        m_isEditableTextMode = false;
        if (m_previewView) {
            m_previewView->load(QUrl::fromLocalFile(m_path));
        }
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
        if (!confirmDiscardChanges()) {
            return;
        }
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
            return;
        }
        ui->plainTextEdit->document()->setModified(false);
    }

    void FileViewer::showError(const QString &message) const {
        drawer::warning(const_cast<FileViewer *>(this), tr("File Viewer"), message);
    }

    bool FileViewer::confirmDiscardChanges() const {
        if (!m_isEditableTextMode || !ui->plainTextEdit->document()->isModified()) {
            return true;
        }
        const QMessageBox::StandardButton answer = drawer::warning(
            const_cast<FileViewer *>(this),
            tr("Unsaved changes"),
            tr("Discard the current file changes?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
        return answer == QMessageBox::Discard;
    }

    FileViewer::~FileViewer() {
        delete ui;
    }
}
