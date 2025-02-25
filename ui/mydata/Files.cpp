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
#include <QStorageInfo>
#include <ui_ImageViewer.h>

#include "FileInfoListModel.hpp"
#include "ImageViewer.hpp"




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

		ui->groupBox_ItemInfo->hide();

		m_currentDir = nullptr;

	    m_visitedPaths.clear();

	    m_fileSystemModel = new QFileSystemModel();

	    m_fileSystemModel->setRootPath(QDir::homePath());

		ui->listView_Files->resize(0, 0);
		ui->listView_Files->adjustSize();
		ui->listView_Files->setModel(m_fileSystemModel);

		const auto index = m_fileSystemModel->index(QDir::currentPath());

		qDebug() << index;

		ui->listView_Files->setRootIndex(index);

		connect(ui->listView_Files, &QAbstractItemView::activated, this, &Files::onItemViewActivated);
		connect(ui->listView_Files, &QAbstractItemView::clicked, this, &Files::onItemViewClicked);

		connect(ui->toolButton_Back, &QToolButton::clicked, this, &Files::onBackClicked);
		connect(ui->toolButton_Up, &QToolButton::clicked, this, &Files::onUpClicked);
		connect(ui->lineEdit_Path, &QLineEdit::returnPressed, this, &Files::onPathReturnPressed);

		connect(ui->toolButton_Filters, &QToolButton::clicked, this, &Files::onFiltersClicked);
		connect(ui->toolButton_Search, &QToolButton::clicked, this, &Files::onSearchClicked);

		connect(ui->toolButton_Cut, &QToolButton::clicked, this, &Files::onCutClicked);
		connect(ui->toolButton_Copy, &QToolButton::clicked, this, &Files::onCopyClicked);
		connect(ui->toolButton_Paste, &QToolButton::clicked, this, &Files::onPasteClicked);

		connect(ui->toolButton_NewFolder, &QToolButton::clicked, this, &Files::onNewFolderClicked);
		connect(ui->toolButton_NewFile, &QToolButton::clicked, this, &Files::onNewFileClicked);

		connect(ui->toolButton_Delete, &QToolButton::clicked, this, &Files::onDeleteClicked);
		connect(ui->toolButton_Rename, &QToolButton::clicked, this, &Files::onRenameClicked);

		connect(ui->toolButton_Home, &QToolButton::clicked, this, &Files::onHome);

		m_fileListModel = new FileInfoListModel();
		ui->tableView_Search->setModel(m_fileListModel);

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

		qDebug() << path << ":" << index;

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
	void Files::onItemViewActivated(const QModelIndex &index) {
		qDebug() << index;

		if (const auto path = m_fileSystemModel->filePath(index); selectFileSystemItem(path, Files::CDSource::Navpage))
			setCurrentDir(path);
		else {
			qDebug() << "Should open the file here." << path;
			ui->group_ToolBar->hide();
			ui->group_Main->hide();
			m_imageViewer = new ImageViewer(path);
			connect(m_imageViewer, &ImageViewer::askedToBeClosed, this, &Files::onImageViewerCloseClicked);
			ui->group_Content->layout()->addWidget(m_imageViewer);


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
		const QFileInfo fileInfo = m_fileSystemModel->fileInfo(index);

		if (fileInfo.isFile())
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

		qDebug() << "Files::onItemViewClicked";
	}

	void Files::onFiltersClicked() {
		ui->lineEdit_Search->clear();
		ui->tableView_Search->hide();
		ui->listView_Files->show();
	}

	void Files::onSearchClicked() {

		const auto key = ui->lineEdit_Search->text();

		if (key.isEmpty()) {
			ui->tableView_Search->hide();
			ui->listView_Files->show();
		} else {

			ui->listView_Files->hide();

			//const Qt::CaseSensitivity caseSensitivity = ui->caseSensitive->isChecked() ? Qt::CaseSensitive:Qt::CaseInsensitive;
			const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
			m_results.clear();

			QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;

			//if (ui->searchHidden->isChecked())
			filters |= QDir::Hidden;
			//if (ui->searchSystem->isChecked())
			filters |= QDir::System;

			auto searchPath = getCurrentDir();

			qDebug() << searchPath;

			QDirIterator dirItems(searchPath, filters, QDirIterator::Subdirectories);

			while (dirItems.hasNext()) {
				if (const QFileInfo fileInfo = dirItems.nextFileInfo(); fileInfo.fileName().contains(key, caseSensitivity)) {
					m_results.append(fileInfo);
					qDebug() << fileInfo;
				}
			}

			dynamic_cast<FileInfoListModel *>(ui->tableView_Search->model())->setQFileInfoList(m_results);
			ui->tableView_Search->resizeColumnsToContents();

			ui->tableView_Search->show();

		}



	}

	bool Files::selectFileSystemItem(const QString &path, const CDSource source) {
		int result = false;
		const auto dir = new QDir(path);

		if (dir->exists()) {
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

	void Files::onNewFileClicked() {
		bool ok;
		const QString filename = QInputDialog::getText(this,
		                                         tr("Create a new file"),
		                                         tr("Enter filename:"),
		                                         QLineEdit::Normal,
		                                         tr("New file"),
		                                         &ok);
		if (ok && !filename.isEmpty()) {
			if (QFile file(filename); file.open(QIODeviceBase::NewOnly)) {
				qDebug() << "File created successfully";
				file.close();
			} else {
				qDebug() << "Unable to create file";
			}
		}
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
