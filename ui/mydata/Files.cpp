//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Files.h" resolved

#include "Files.hpp"
#include "ui_Files.h"

#include <QFileSystemModel>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QScroller>
#include <QStorageInfo>
#include <ui_ImageViewer.h>
#include <ui_TextViewer.h>

#include "FileInfoListModel.hpp"



#ifdef Q_OS_WIN
	// See https://doc.qt.io/qt-6/qfileinfo.html#details
	extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#else
int qt_ntfs_permission_lookup = 0; //dummy
#endif

namespace fairwindsk::ui::mydata {
	Files::Files(QWidget *parent) : QWidget(parent), ui(new Ui::Files) {
	    ui->setupUi(this);

		ui->tableView_Search->hide();
		ui->listView_Files->show();
		ui->widget_Searching->hide();

		ui->groupBox_ItemInfo->hide();

		m_currentDir = nullptr;

	    m_visitedPaths.clear();

	    m_fileSystemModel = new QFileSystemModel();

	    m_fileSystemModel->setRootPath(QDir::homePath());

		ui->listView_Files->resize(0, 0);
		ui->listView_Files->adjustSize();
		ui->listView_Files->setModel(m_fileSystemModel);



		const auto index = m_fileSystemModel->index(QDir::currentPath());

		ui->listView_Files->setRootIndex(index);

		connect(ui->listView_Files, &QAbstractItemView::doubleClicked, this, &Files::onFileViewItemDoubleClicked);
		connect(ui->listView_Files, &QAbstractItemView::clicked, this, &Files::onItemViewClicked);

		connect(ui->toolButton_Back, &QToolButton::clicked, this, &Files::onBackClicked);
		connect(ui->toolButton_Up, &QToolButton::clicked, this, &Files::onUpClicked);
		connect(ui->lineEdit_Path, &QLineEdit::returnPressed, this, &Files::onPathReturnPressed);

		connect(ui->toolButton_Filters, &QToolButton::clicked, this, &Files::onFiltersClicked);
		connect(ui->toolButton_Search, &QToolButton::clicked, this, &Files::onSearchClicked);
		connect(ui->lineEdit_Search,&QLineEdit::returnPressed, this, &Files::onSearchClicked);

		connect(ui->toolButton_Cut, &QToolButton::clicked, this, &Files::onCutClicked);
		connect(ui->toolButton_Copy, &QToolButton::clicked, this, &Files::onCopyClicked);
		connect(ui->toolButton_Paste, &QToolButton::clicked, this, &Files::onPasteClicked);

		connect(ui->toolButton_NewFolder, &QToolButton::clicked, this, &Files::onNewFolderClicked);

		connect(ui->toolButton_Delete, &QToolButton::clicked, this, &Files::onDeleteClicked);
		connect(ui->toolButton_Rename, &QToolButton::clicked, this, &Files::onRenameClicked);

		connect(ui->toolButton_Home, &QToolButton::clicked, this, &Files::onHome);

		m_fileListModel = new FileInfoListModel();
		ui->tableView_Search->setModel(m_fileListModel);

		connect(&m_searchingWatcher, &QFutureWatcher<QList<QFileInfo>>::finished, this, &Files::searchFinished);
		connect(&m_searchingWatcher, &QFutureWatcher<QList<QFileInfo>>::progressValueChanged, this, &Files::searchProgressValueChanged);

		connect(ui->tableView_Search, &QAbstractItemView::doubleClicked, this, &Files::onSearchViewItemDoubleClicked);

		ui->listView_Files->setAttribute(Qt::WA_AcceptTouchEvents,true);
		ui->tableView_Search->setAttribute(Qt::WA_AcceptTouchEvents,true);
		
		onHome();
	}



	QString Files::getCurrentDir() const {
		return m_currentDir->absolutePath();
	}

	void Files::setCurrentDir(const QString& path) {
		if (m_currentDir) {
			delete m_currentDir;
			m_currentDir = nullptr;
		}

		m_currentDir = new QDir(path);
		const auto index = m_fileSystemModel->index(path);



		ui->listView_Files->setRootIndex(index);

		ui->groupBox_ItemInfo->hide();
	}

	void Files::onImageViewerCloseClicked() {
		if (m_imageViewer) {
			m_imageViewer->close();
			delete m_imageViewer;
			m_imageViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
		}
	}
	void Files::onFileViewItemDoubleClicked(const QModelIndex &index) {

		const auto path = m_fileSystemModel->filePath(index);

		if ( selectFileSystemItem(path, Files::CDSource::Navpage))
			setCurrentDir(path);
		else {

			viewFile(path);
		}

		qDebug() << path;
	}

	void Files::onSearchViewItemDoubleClicked(const QModelIndex& index) {


		const auto path = m_fileListModel->getAbsolutePath(index);

		qDebug() << path;


		viewFile(path);

	}

	void Files::viewFile(const QString& path) {
		qDebug() << "Files::viewFile " << path;

		const QMimeDatabase db;
		const auto type = db.mimeTypeForFile(path);
		qDebug() << "Mime type:" << type.name();

		if (type.name() == "image/png" || type.name() == "image/jpeg" || type.name() == "image/gif" || type.name() == "image/bitmap") {
			ui->group_ToolBar->hide();
			ui->group_Main->hide();
			m_imageViewer = new ImageViewer(path);
			connect(m_imageViewer, &ImageViewer::askedToBeClosed, this, &Files::onImageViewerCloseClicked);
			ui->group_Content->layout()->addWidget(m_imageViewer);
		} else if (type.name() == "application/json") {
			ui->group_ToolBar->hide();
			ui->group_Main->hide();
			m_jsonViewer = new JsonViewer(path);
			connect(m_jsonViewer, &JsonViewer::askedToBeClosed, this, &Files::onJsonViewerCloseClicked);
			ui->group_Content->layout()->addWidget(m_jsonViewer);
		} else if (type.name() == "application/pdf") {
			ui->group_ToolBar->hide();
			ui->group_Main->hide();
			m_pdfViewer = new PdfViewer(path);
			connect(m_pdfViewer, &PdfViewer::askedToBeClosed, this, &Files::onPdfViewerCloseClicked);
			ui->group_Content->layout()->addWidget(m_pdfViewer);
		} else if (type.name() == "text/html" || type.name() == "text/plain" || type.name() == "application/xml") {
			ui->group_ToolBar->hide();
			ui->group_Main->hide();
			m_textViewer = new TextViewer(path);
			connect(m_textViewer, &TextViewer::askedToBeClosed, this, &Files::onTextViewerCloseClicked);
			ui->group_Content->layout()->addWidget(m_textViewer);
		}
	}

	void Files::onPdfViewerCloseClicked() {
		if (m_pdfViewer) {
			m_pdfViewer->close();
			delete m_pdfViewer;
			m_pdfViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
		}
	}

	void Files::onJsonViewerCloseClicked() {
		if (m_textViewer) {
			m_textViewer->close();
			delete m_textViewer;
			m_textViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
		}
	}

	void Files::onTextViewerCloseClicked() {
		if (m_textViewer) {
			m_textViewer->close();
			delete m_textViewer;
			m_textViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
		}
	}

	QStringList Files::getSelection() const {

		QModelIndexList list = ui->listView_Files->selectionModel()->selectedIndexes();
		QStringList path_list;
		foreach (const QModelIndex &index, list) {
			path_list.append(m_fileSystemModel->filePath(index));
		}

		qDebug() << path_list.join(",");

		return path_list;
}

	void Files::onItemViewClicked(const QModelIndex &index) const {
		if (const QFileInfo fileInfo = m_fileSystemModel->fileInfo(index); fileInfo.isFile())
		{
			ui->groupBox_ItemInfo->setTitle(fileInfo.fileName());

			QString permissions = "";

			const bool isNTFS = (new QStorageInfo(QDir::current()))->fileSystemType().compare("NTFS") == 0;
			if (isNTFS)
				qt_ntfs_permission_lookup++; // turn checking on, Windows only
			if (fileInfo.isReadable())
				permissions += tr("Read ");
			if (fileInfo.isWritable())
				permissions += tr("Write ");
			if (fileInfo.isExecutable())
				permissions += tr("Execute ");
			if (isNTFS)
				qt_ntfs_permission_lookup--; // turn it off again, Windows only
			ui->label_Permissions->setText(permissions.trimmed());
			ui->label_Size->setText(format_bytes(fileInfo.size()));

			const QMimeDatabase mimedb;
			ui->label_Type->setText(mimedb.mimeTypeForFile(fileInfo).comment());
			ui->groupBox_ItemInfo->show();
		} else {
			ui->groupBox_ItemInfo->hide();
		}


	}

	void Files::onFiltersClicked() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		ui->lineEdit_Search->clear();
		ui->tableView_Search->hide();
		ui->widget_Searching->hide();
		ui->listView_Files->show();
	}

	void Files::onSearchClicked() {

		if (const auto key = ui->lineEdit_Search->text(); key.isEmpty()) {
			ui->tableView_Search->hide();
			ui->listView_Files->show();
		} else {

			ui->listView_Files->hide();
			ui->tableView_Search->hide();
			ui->label_Searching->setText(tr("Searching..."));
			ui->widget_Searching->show();

			auto searchPath = getCurrentDir();

			const Qt::CaseSensitivity caseSensitivity = ui->toolButton_CaseSensitive->isChecked() ? Qt::CaseSensitive:Qt::CaseInsensitive;


			QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;

			if (ui->toolButton_SearchHidden->isChecked()) {
				filters |= QDir::Hidden;
			}
			if (ui->toolButton_SearchSystem->isChecked())
				filters |= QDir::System;

			const auto future = QtConcurrent::run(Files::search, searchPath, key, caseSensitivity,filters);
			m_searchingWatcher.setFuture(future);

		}
	}

	void Files::search(QPromise<QFileInfo> &promise, const QString &searchPath, const QString &key, const Qt::CaseSensitivity caseSensitivity, const QDir::Filters filters)
	{
		qDebug() << "Files::search" << searchPath << " " << key;

		QDirIterator dirItems(searchPath, filters, QDirIterator::Subdirectories);
		int count=0;
		while (dirItems.hasNext()) {

			promise.suspendIfRequested();

			if (promise.isCanceled())
				return;

			if (const QFileInfo fileInfo = dirItems.nextFileInfo(); fileInfo.fileName().contains(key, caseSensitivity)) {
				promise.addResult(fileInfo);
			}

			count++;
			if (count % 1000) {

				promise.setProgressValue(count);
			}
		}
	}

	void Files::searchProgressValueChanged(const int progress) {
		// Update the progress bar
		ui->progressBar_Searching->setValue(progress % 100);
	}

	void Files::searchFinished() {
		if (const QList<QFileInfo> results = m_searchingWatcher.future().results<QFileInfo>(); !results.isEmpty()) {

			dynamic_cast<FileInfoListModel *>(ui->tableView_Search->model())->setQFileInfoList(results);
			ui->tableView_Search->resizeColumnsToContents();
			ui->widget_Searching->hide();
			ui->tableView_Search->show();
		} else {
			ui->label_Searching->setText(tr("Not found!"));
			ui->progressBar_Searching->setValue(100);
		}
	}



	bool Files::selectFileSystemItem(const QString &path, const CDSource source) {
		int result = false;

		if (const auto dir = new QDir(path); dir->exists()) {
			QDir::setCurrent(path);

			if (source != CDSource::Navbar) {
				ui->lineEdit_Path->setText(dir->absolutePath());
				ui->lineEdit_Path->update();
			}


			m_visitedPaths.push_back(path);

			delete dir;
			result = true;
		}

		return result;
	}





	void Files::onHome() {
		setCurrentDir(QDir::homePath());
		ui->lineEdit_Path->setText(m_currentDir->absolutePath());

		QDir::setCurrent(m_currentDir->absolutePath());

	}

	void Files::onPathReturnPressed() {
		selectFileSystemItem(ui->lineEdit_Path->text(), CDSource::Navbar);
	}


	void Files::showWarning(const QString &message) const {
		const auto alert = new QMessageBox();
		alert->setText(message);
		alert->setIcon(QMessageBox::Warning);
		alert->setWindowIcon(windowIcon());
		alert->exec();
		delete alert;
	}

	void Files::onBackClicked() {
		if (!m_visitedPaths.isEmpty()) {
			m_visitedPaths.pop_back(); // Remove current path
		}

		if (!m_visitedPaths.isEmpty()) {
			const QString prev_path = m_visitedPaths.back();
			m_visitedPaths.pop_back(); // Remove previous path
			selectFileSystemItem(prev_path, CDSource::Navbutton);
		}
	}

	void Files::onUpClicked() {
		if (QDir dir = QDir::current(); dir.cdUp())
			selectFileSystemItem(dir.absolutePath(), CDSource::Navbutton);
	}

	void Files::onNewFolderClicked() {
		bool ok;
		QString path = QInputDialog::getText(this,
		                                     tr("Create a new folder"),
		                                     tr("Enter path:"),
		                                     QLineEdit::Normal,
		                                     tr("New folder"),
		                                     &ok);

		if (ok && !path.isEmpty()) {
			QDir dir;
			if (!dir.exists(path)) {
				dir.mkdir(path);
				qDebug() << "Directory created successfully";
			} else {
				qDebug() << "Directory already exists";
			}
		}
	}

	void Files::onDeleteClicked()
	{

			QStringList paths_to_delete = getSelection();
			QMessageBox::StandardButton choice
				= QMessageBox::warning(this,
			                           "Confirm delete",
			                           "Are you sure you want to delete the selected items?",
			                           QMessageBox::Ok | QMessageBox::Cancel);
			if (choice == QMessageBox::Ok) {
				foreach (QString path, paths_to_delete) {
					if (QFileInfo fileInfo(path); fileInfo.isDir()) {
						QDir dir(path);
						if (!dir.removeRecursively()) {
							qDebug() << "Failed to delete folder: " << path;
						}
					} else {
						if (QFile file(path); !file.remove()) {
							qDebug() << "Failed to delete file: " << path;
						}
					}
				}
			}

	}

	void Files::onRenameClicked() {

			QStringList selectedItems =getSelection();
			foreach (QString selectedItem, selectedItems) {
				if (!selectedItem.isEmpty()) {
					bool ok;
					QFileInfo fileInfo(selectedItem);
					QString newName = QInputDialog::getText(this,
					                                        tr("Rename"),
					                                        tr("New name:"),
					                                        QLineEdit::Normal,
					                                        fileInfo.completeBaseName(),
					                                        &ok);
					if (ok && !newName.isEmpty()) {
						QFile item(selectedItem);
						QString itemType = fileInfo.isDir() ? "Folder" : "File";
						if (item.rename(newName)) {
							qDebug() << itemType << " renamed successfully";
						} else {
							qDebug() << "Error renaming " << itemType;
						}
					}
				}
			}

	}

	void Files::onCutClicked() {

			m_itemsToCopy.clear();
			m_itemsToMove = getSelection();
			qDebug() << "Cut successfully";

	}

	void Files::onCopyClicked() {

			m_itemsToMove.clear();
			m_itemsToCopy = getSelection();
			qDebug() << "Copied successfully";

	}

	void Files::onPasteClicked() {

			foreach (QString path, m_itemsToCopy) {
				QFileInfo item(path);
				qDebug() << "Name:" << item.fileName();
				qDebug() << "path: " << path;
				if (item.isFile()) {
					QString newPath = QDir::current().absoluteFilePath(item.fileName());
					qDebug() << "new path:" << newPath;
					qDebug() << "Copy result" << QFile::copy(path, newPath);
				} else {
					copyOrMoveDirectorySubtree(path, QDir::currentPath(), false, false);
				}
			}
			foreach (QString path, m_itemsToMove) {
				QFileInfo item(path);
				qDebug() << "Name:" << item.fileName();
				qDebug() << "path: " << path;
				if (item.isFile()) {
					QString newPath = QDir::current().absoluteFilePath(item.fileName());
					qDebug() << "new path:" << newPath;
					qDebug() << "Copy result" << QFile::copy(path, newPath);
					qDebug() << "Remove result" << QFile::remove(path);
				} else {
					copyOrMoveDirectorySubtree(path, QDir::currentPath(), true, false);
				}
			}

	}

    QString Files::format_bytes(qint64 bytes)
    {
	    qint64 higher = bytes;
        int index = 0;
        const QString units[] = {"B", "KB", "MB", "GB", "TB"};
        while (higher > 1000) {
            ++index;
            higher /= 1000;
        }
        QString result = QString::number(higher) + " " + units[index];
        return result;
    }

    void Files::copyOrMoveDirectorySubtree(const QString &from,
                                    const QString &to,
                                    const bool copyAndRemove,
                                    const bool overwriteExistingFiles)
    {
        QDirIterator diritems(from, QDirIterator::Subdirectories);
        QDir fromDir(from);
        QDir toDir(to);

        const auto absSourcePathLength = fromDir.absolutePath().length();

        toDir.mkdir(fromDir.dirName());
        toDir.cd(fromDir.dirName());

        while (diritems.hasNext()) {
            const QFileInfo fileInfo = diritems.nextFileInfo();

            if (fileInfo.fileName().compare(".") == 0
                || fileInfo.fileName().compare("..") == 0) //filters dot and dotdot
                    continue;
            const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
            qDebug() << "subPathStructure: " << subPathStructure;
            const QString constructedAbsolutePath = toDir.canonicalPath() + subPathStructure;
            qDebug() << "constructedAbsolutePath: " << constructedAbsolutePath;

            if (fileInfo.isDir()) {
                //Create directory in target folder
                toDir.mkpath(constructedAbsolutePath);
            } else if (fileInfo.isFile()) {
                //Copy File to target directory

                if (overwriteExistingFiles) {
                    //Remove file at target location, if it exists, or QFile::copy will fail
                    QFile::remove(constructedAbsolutePath);
                }
                QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
            }
        }

        if (copyAndRemove)
            fromDir.removeRecursively();
    }



	Files::~Files() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		if (m_textViewer) {
			delete m_textViewer;
			m_textViewer = nullptr;
		}

		if (m_imageViewer) {
			delete m_imageViewer;
			m_imageViewer = nullptr;
		}

		if (m_fileListModel) {
			delete m_fileListModel;
			m_fileListModel = nullptr;
		}

		if (m_fileSystemModel) {
			delete m_fileSystemModel;
			m_fileSystemModel = nullptr;
		}

		if (ui) {
			delete ui;
			ui = nullptr;
		}
	}
} // fairwindsk::ui::mydata
