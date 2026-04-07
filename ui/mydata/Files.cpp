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
#include <QFileDialog>
#include <QScroller>
#include <QStorageInfo>

#include "FairWindSK.hpp"
#include "FileInfoListModel.hpp"
#include "ui/IconUtils.hpp"
#include "ui/DrawerDialogHost.hpp"



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



		m_fileSystemModel = new QFileSystemModel(this);
        m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
	    m_fileSystemModel->setRootPath(QDir::homePath());

		ui->listView_Files->resize(0, 0);
		ui->listView_Files->adjustSize();
		ui->listView_Files->setModel(m_fileSystemModel);
        ui->listView_Files->setSelectionMode(QAbstractItemView::ExtendedSelection);
        ui->tableView_Search->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView_Search->setSelectionMode(QAbstractItemView::ExtendedSelection);



		const auto index = m_fileSystemModel->index(QDir::currentPath());

		ui->listView_Files->setRootIndex(index);

		connect(ui->listView_Files, &QAbstractItemView::doubleClicked, this, &Files::onFileViewItemDoubleClicked);
		connect(ui->listView_Files, &QAbstractItemView::clicked, this, &Files::onFileViewItemClicked);

		connect(ui->toolButton_Up, &QToolButton::clicked, this, &Files::onUpClicked);
		connect(ui->lineEdit_Path, &QLineEdit::returnPressed, this, &Files::onPathReturnPressed);

		connect(ui->toolButton_Filters, &QToolButton::clicked, this, &Files::onFiltersClicked);
		connect(ui->toolButton_Search, &QToolButton::clicked, this, &Files::onSearchClicked);
		connect(ui->lineEdit_Search,&QLineEdit::returnPressed, this, &Files::onSearchClicked);

		connect(ui->toolButton_Cut, &QToolButton::clicked, this, &Files::onCutClicked);
		connect(ui->toolButton_Copy, &QToolButton::clicked, this, &Files::onCopyClicked);
		connect(ui->toolButton_Paste, &QToolButton::clicked, this, &Files::onPasteClicked);

		connect(ui->toolButton_Open, &QToolButton::clicked, this, &Files::onOpenClicked);

		connect(ui->toolButton_NewFolder, &QToolButton::clicked, this, &Files::onNewFolderClicked);

		connect(ui->toolButton_Delete, &QToolButton::clicked, this, &Files::onDeleteClicked);
		connect(ui->toolButton_Rename, &QToolButton::clicked, this, &Files::onRenameClicked);

		connect(ui->toolButton_Home, &QToolButton::clicked, this, &Files::onHome);

		m_fileListModel = new FileInfoListModel(this);
		ui->tableView_Search->setModel(m_fileListModel);

		connect(&m_searchingWatcher, &QFutureWatcher<QFileInfo>::finished, this, &Files::searchFinished);
		connect(&m_searchingWatcher, &QFutureWatcher<QFileInfo>::progressValueChanged, this, &Files::searchProgressValueChanged);

		connect(ui->tableView_Search, &QAbstractItemView::doubleClicked, this, &Files::onSearchViewItemDoubleClicked);
		connect(ui->tableView_Search, &QAbstractItemView::clicked, this, &Files::onSearchViewItemClicked);

	    ui->listView_Files->setAttribute(Qt::WA_AcceptTouchEvents,true);
		ui->tableView_Search->setAttribute(Qt::WA_AcceptTouchEvents,true);
        QScroller::grabGesture(ui->listView_Files->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(ui->listView_Files->viewport(), QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(ui->tableView_Search->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(ui->tableView_Search->viewport(), QScroller::LeftMouseButtonGesture);

        retintToolButtons();
		
		onHome();
	}

    void Files::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
        }
    }

    void Files::retintToolButtons() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const QColor fallbackIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        const QColor buttonIconColor = fairwindsk::ui::comfortIconColor(configuration, preset, fallbackIconColor);
        for (auto *button : findChildren<QToolButton *>()) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(32, 32));
        }
    }



	QString Files::getCurrentDir() const {
		if (!m_currentDir) {
			return {};
		}

		return m_currentDir->absolutePath();
	}

	void Files::setCurrentDir(const QString& path) {
        const QFileInfo pathInfo(path);
        const QString absolutePath = pathInfo.exists() && pathInfo.isDir()
            ? pathInfo.absoluteFilePath()
            : QDir(path).absolutePath();
        const QDir targetDir(absolutePath);
        if (!targetDir.exists()) {
            showWarning(tr("The selected directory does not exist."));
            return;
        }

		if (m_currentDir) {
			delete m_currentDir;
			m_currentDir = nullptr;
		}

		QDir::setCurrent(absolutePath);
		m_currentDir = new QDir(absolutePath);

		ui->lineEdit_Path->setText(m_currentDir->absolutePath());
		ui->lineEdit_Path->update();

		const auto index = m_fileSystemModel->index(m_currentDir->absolutePath());


		ui->listView_Files->setRootIndex(index);
        ui->listView_Files->clearSelection();
        ui->tableView_Search->clearSelection();
        m_currentFilePath.clear();

		ui->groupBox_ItemInfo->hide();
	}



	/***********************
	 * Open the selected
	 ***********************/
	void Files::onOpenClicked() {

		// Check if the selected file is not empty
		if (!m_currentFilePath.isEmpty()) {

			const auto fileInfo = QFileInfo(m_currentFilePath);

			if (fileInfo.isFile()) {
				// View the selected file
				viewFile(m_currentFilePath);
			} else {
				// Select the current path
				setCurrentDir(m_currentFilePath);
			}
		}

	}

	bool Files::setCurrentFilePath(const QString& path) {

		// By default, the file path is a directory
		bool result = false;

		// Get the file info sand check if it is a file
		if (const auto fileInfo =QFileInfo(path); fileInfo.isFile()) {

			// Set the group box title
			ui->groupBox_ItemInfo->setTitle(fileInfo.fileName());

			// Initialize the permission string
			QString permissions = "";

			// Get the NTFS Flag (needed by Windows)
			const QStorageInfo storageInfo(QDir::current());
			const bool isNTFS = storageInfo.fileSystemType().compare("NTFS") == 0;

			// Check if is NTFS
			if (isNTFS) {
				// turn checking on, Windows only
				qt_ntfs_permission_lookup++;
			}

			// Check if the file is readable
			if (fileInfo.isReadable())
				permissions += tr("Read ");

			// Check if the file is Writable
			if (fileInfo.isWritable())
				permissions += tr("Write ");

			// Check if the file is executable
			if (fileInfo.isExecutable())
				permissions += tr("Execute ");

			// Check if is NTFS
			if (isNTFS) {
				// turn it off again, Windows only
				qt_ntfs_permission_lookup--;
			}

			// Set the permission label
			ui->label_Permissions->setText(permissions.trimmed());

			// Set the file size label
			ui->label_Size->setText(format_bytes(fileInfo.size()));

			// Define the mime type database
			const QMimeDatabase mimedb;

			// Set the mime type label
			ui->label_Type->setText(mimedb.mimeTypeForFile(fileInfo).comment());

			// Show the group box
			ui->groupBox_ItemInfo->show();

			// Return true
			result = true;
		} else {

			// Hide the group box
			ui->groupBox_ItemInfo->hide();


		}

		// Set the current file path
		m_currentFilePath = path;

		// Return the result
		return result;
	}

	void Files::onFileViewItemClicked(const QModelIndex &index)  {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileSystemModel->filePath(index); !setCurrentFilePath(path)) {

			// Hide the file group box
			ui->groupBox_ItemInfo->hide();
		}
	}

	void Files::onFileViewItemDoubleClicked(const QModelIndex &index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileSystemModel->filePath(index); setCurrentFilePath(path)) {

			// View the file
			viewFile(path);

		} else {

			// Select the current path
			setCurrentDir(path);

		}

	}

	void Files::onSearchViewItemClicked(const QModelIndex& index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path =  m_fileListModel->getAbsolutePath(index); !setCurrentFilePath(path)) {

			// Reset the current file path
			m_currentFilePath = "";

			// Hide the file group box
			ui->groupBox_ItemInfo->hide();
		}


	}

	void Files::onSearchViewItemDoubleClicked(const QModelIndex& index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileListModel->getAbsolutePath(index); setCurrentFilePath(path)) {

			// View the file
			viewFile(path);
        } else if (!path.isEmpty()) {
            setCurrentDir(path);
            setSearchResultsVisible(false);
            ui->widget_Searching->hide();
            ui->listView_Files->show();
		}

	}



	void Files::viewFile(const QString& path) {
		if (m_fileViewer) {
			onFileViewerCloseClicked();
		}

		qDebug() << "Files::viewFile " << path;

		const QMimeDatabase db;
		const auto type = db.mimeTypeForFile(path);
		qDebug() << "Mime type:" << type.name();

		ui->group_ToolBar->hide();
		ui->group_Main->hide();
		m_fileViewer = new FileViewer(path);
		connect(m_fileViewer, &FileViewer::askedToBeClosed, this, &Files::onFileViewerCloseClicked);
		ui->group_Content->layout()->addWidget(m_fileViewer);
	}



	void Files::onFileViewerCloseClicked() {
		if (m_fileViewer) {
			m_fileViewer->close();
			delete m_fileViewer;
			m_fileViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
		}
	}



	QStringList Files::getSelection() const {
        const auto *selectionModel = activeSelectionModel();
        if (!selectionModel) {
            return {};
        }
		QModelIndexList list = selectionModel->selectedRows();
        if (list.isEmpty()) {
            list = selectionModel->selectedIndexes();
        }
		QStringList path_list;
		foreach (const QModelIndex &index, list) {
            if (ui->tableView_Search->isVisible()) {
                const QString path = m_fileListModel->getAbsolutePath(index);
                if (!path.isEmpty()) {
                    path_list.append(path);
                }
            } else {
			    path_list.append(m_fileSystemModel->filePath(index));
            }
		}
        path_list.removeDuplicates();
        return path_list;
}



	void Files::onFiltersClicked() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		ui->lineEdit_Search->clear();
		setSearchResultsVisible(false);
		ui->widget_Searching->hide();
		ui->listView_Files->show();
	}

	void Files::onSearchClicked() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		if (const auto key = ui->lineEdit_Search->text(); key.isEmpty()) {
			setSearchResultsVisible(false);
			ui->listView_Files->show();
		} else {

			ui->listView_Files->hide();
			setSearchResultsVisible(false);
			ui->label_Searching->setText(tr("Searching..."));
			ui->progressBar_Searching->setValue(0);
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
			if (count % 1000 == 0) {

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

			m_fileListModel->setQFileInfoList(results);
			ui->tableView_Search->resizeColumnsToContents();
			ui->widget_Searching->hide();
			setSearchResultsVisible(true);
		} else {
			ui->label_Searching->setText(tr("Not found!"));
			ui->progressBar_Searching->setValue(100);
			setSearchResultsVisible(false);
		}
	}

	void Files::setSearchResultsVisible(const bool visible) {
		ui->tableView_Search->setVisible(visible);
		if (!visible) {
			m_fileListModel->setQFileInfoList({});
		}
	}





	void Files::onHome() {
		setCurrentDir(QDir::homePath());
		ui->lineEdit_Path->setText(m_currentDir->absolutePath());

		QDir::setCurrent(m_currentDir->absolutePath());

	}

	void Files::onPathReturnPressed() {
		setCurrentDir(ui->lineEdit_Path->text());
	}


	void Files::showWarning(const QString &message) const {
		drawer::warning(const_cast<Files *>(this), tr("Files"), message);
	}

	void Files::onUpClicked() {
		if (QDir dir = QDir::current(); dir.cdUp()) {
			setCurrentDir(dir.absolutePath());
		}
	}

	void Files::onNewFolderClicked() {
		bool ok = false;
		const QString folderName = drawer::getText(this,
		                               tr("Create a new folder"),
		                               tr("Enter folder name:"),
		                               tr("New folder"),
		                               &ok);

		if (ok && !folderName.isEmpty()) {
            const QString absolutePath = QDir(currentDestinationDir()).filePath(folderName);
			QDir dir;
			if (!dir.exists(absolutePath)) {
				if (!dir.mkpath(absolutePath)) {
                    showWarning(tr("Unable to create the selected folder."));
                }
			} else {
                showWarning(tr("A file or folder with the same name already exists."));
			}
            refreshCurrentView();
		}
	}

	void Files::onDeleteClicked()
	{

			QStringList paths_to_delete = getSelection();
			QMessageBox::StandardButton choice
				= drawer::warning(this,
				                  tr("Confirm delete"),
				                  tr("Are you sure you want to delete the selected items?"),
				                  QMessageBox::Ok | QMessageBox::Cancel,
				                  QMessageBox::Cancel);
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
                refreshCurrentView();
			}

	}

	void Files::onRenameClicked() {

			QStringList selectedItems =getSelection();
			foreach (QString selectedItem, selectedItems) {
				if (!selectedItem.isEmpty()) {
					bool ok = false;
					QFileInfo fileInfo(selectedItem);
					QString newName = drawer::getText(this,
					                                  tr("Rename"),
					                                  tr("New name:"),
					                                  fileInfo.fileName(),
					                                  &ok);
					if (ok && !newName.isEmpty()) {
                        const QString renamedPath = fileInfo.dir().filePath(newName);
                        if (QFileInfo::exists(renamedPath)) {
                            showWarning(tr("A file or folder with the same name already exists."));
                            continue;
                        }
						if (!QFile::rename(selectedItem, renamedPath)) {
                            showWarning(tr("Unable to rename the selected item."));
						}
					}
				}
			}
            refreshCurrentView();

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
            const QString destinationDir = currentDestinationDir();
			foreach (QString path, m_itemsToCopy) {
				QFileInfo item(path);
				if (item.isFile()) {
					QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
                    if (QFileInfo::exists(newPath)) {
                        showWarning(tr("Skipping %1 because it already exists in the destination.").arg(item.fileName()));
                        continue;
                    }
					if (!QFile::copy(path, newPath)) {
                        showWarning(tr("Unable to copy %1.").arg(item.fileName()));
                    }
				} else {
					copyOrMoveDirectorySubtree(path, destinationDir, false, false);
				}
			}
			foreach (QString path, m_itemsToMove) {
				QFileInfo item(path);
				if (item.isFile()) {
					QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
                    if (QFileInfo::exists(newPath)) {
                        showWarning(tr("Skipping %1 because it already exists in the destination.").arg(item.fileName()));
                        continue;
                    }
					if (!QFile::copy(path, newPath) || !QFile::remove(path)) {
                        showWarning(tr("Unable to move %1.").arg(item.fileName()));
                    }
				} else {
					copyOrMoveDirectorySubtree(path, destinationDir, true, false);
				}
			}
            m_itemsToCopy.clear();
            m_itemsToMove.clear();
            refreshCurrentView();

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

		if (m_fileViewer) {
			delete m_fileViewer;
			m_fileViewer = nullptr;
		}

		if (ui) {
			delete ui;
			ui = nullptr;
		}
	}

    QItemSelectionModel *Files::activeSelectionModel() const {
        if (ui->tableView_Search->isVisible()) {
            return ui->tableView_Search->selectionModel();
        }
        return ui->listView_Files->selectionModel();
    }

    QString Files::currentDestinationDir() const {
        const QFileInfo selectedInfo(m_currentFilePath);
        if (selectedInfo.exists() && selectedInfo.isDir()) {
            return selectedInfo.absoluteFilePath();
        }
        if (m_currentDir) {
            return m_currentDir->absolutePath();
        }
        return QDir::homePath();
    }

    void Files::refreshCurrentView() {
        if (m_fileSystemModel) {
            const QString rootPath = m_fileSystemModel->rootPath();
            m_fileSystemModel->setRootPath(QString());
            m_fileSystemModel->setRootPath(rootPath);
        }
        if (m_currentDir) {
            setCurrentDir(m_currentDir->absolutePath());
        }
        if (ui->tableView_Search->isVisible() && !ui->lineEdit_Search->text().trimmed().isEmpty()) {
            onSearchClicked();
        }
    }
} // fairwindsk::ui::mydata
