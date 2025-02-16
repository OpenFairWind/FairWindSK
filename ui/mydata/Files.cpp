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
#include <ui_ImageViewer.h>

#include "ImageViewer.hpp"
#include "NavPage.hpp"
#include "SearchPage.hpp"
#include "Utils.hpp"

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {
Files::Files(QWidget *parent) :
    QWidget(parent), ui(new Ui::Files) {
	    ui->setupUi(this);

	    m_visitedPaths.clear();



	    m_fileSystemModel = new QFileSystemModel();

	    m_fileSystemModel->setRootPath(QDir::rootPath());


	    ui->treeView->setModel(m_fileSystemModel);

	    QHeaderView *header = ui->treeView->header();
	    for (int i = 1; i < header->count(); ++i) {
	        header->hideSection(i);
	    }

	    ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

	    ui->treeView->resizeColumnToContents(0);

	    connect(ui->treeView, &QTreeView::clicked, this, &Files::onTreeViewClicked);
		connect(ui->toolButton_Back, &QToolButton::clicked, this, &Files::onBackClicked);
		connect(ui->toolButton_Up, &QToolButton::clicked, this, &Files::onUpClicked);
		connect(ui->tabWidget, &QTabWidget::currentChanged, this, &Files::onTabWidgetCurrentChanged);
		connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &Files::onTabWidgetTabCloseRequested);
		connect(ui->addressBar, &QLineEdit::returnPressed, this, &Files::onAddressBarReturnPressed);

		connect(ui->toolButton_Search, &QToolButton::clicked, this, &Files::onSearchClicked);

		connect(ui->toolButton_Cut, &QToolButton::clicked, this, &Files::onCutClicked);
		connect(ui->toolButton_Copy, &QToolButton::clicked, this, &Files::onCopyClicked);
		connect(ui->toolButton_Paste, &QToolButton::clicked, this, &Files::onPasteClicked);

		connect(ui->toolButton_NewFolder, &QToolButton::clicked, this, &Files::onNewFolderClicked);
		connect(ui->toolButton_NewFile, &QToolButton::clicked, this, &Files::onNewFileClicked);

		connect(ui->toolButton_Delete, &QToolButton::clicked, this, &Files::onDeleteClicked);
		connect(ui->toolButton_Rename, &QToolButton::clicked, this, &Files::onRenameClicked);

		connect(ui->toolButton_NewTab, &QToolButton::clicked, this, &Files::onNewTabClicked);
		connect(ui->toolButton_CloseTab, &QToolButton::clicked, this, &Files::onCloseTabClicked);

		connect(ui->toolButton_Home, &QToolButton::clicked, this, &Files::onHome);

		onNewTabClicked();
	}



    int Files::addPageToTabWidget(const QString& dir, const QString &label) {

		const QFileInfo fileInfo(dir);

		if (fileInfo.exists()) {
			if (fileInfo.isDir()) {
				const auto navPage = new NavPage(m_fileSystemModel, this);
				navPage->setCurrentDir(dir);
				ui->tabWidget->addTab(navPage, label);
				ui->tabWidget->setCurrentWidget(navPage);
				m_visitedPaths.push_back(dir);
			} else if (fileInfo.isFile()) {
				const QMimeDatabase db;
				const QMimeType mime = db.mimeTypeForFile(dir, QMimeDatabase::MatchContent);
				qDebug() << mime.name();            // Name of the MIME type ("audio/mpeg").
				qDebug() << mime.suffixes();        // Known suffixes for this MIME type ("mp3", "mpga").
				qDebug() << mime.preferredSuffix(); // Preferred suffix for this MIME type ("mp3").

				if (mime.name().indexOf("image/")>=0)
				{
					const auto imageViewer = new ImageViewer(dir, this);
					ui->tabWidget->addTab(imageViewer, label);
					ui->tabWidget->setCurrentWidget(imageViewer);

				}
			}
		}
		return 0;
	}

	void Files::addSearchPage() {

		auto *searchPage = new SearchPage(ui->lineEdit_Search->text(), this);
		ui->tabWidget->addTab(searchPage, tr("Search"));
		ui->tabWidget->setCurrentWidget(searchPage);
	}

	bool Files::selectFileSystemItem(const QString &path, const CDSource source, const bool suppress_warning) {
		const auto dir = new QDir(path);
		const QString message = "This is not a folder.";
		if (dir->exists()) {
			QDir::setCurrent(path);

			if (source != CDSource::Navtree) {
				locateInTree(path);
			}
			if (source != CDSource::Navbar) {
				ui->addressBar->setText(dir->absolutePath());
				ui->addressBar->update();
			}
			if (source != CDSource::Tabchange) {
				if (const auto currentPage = ui->tabWidget->currentWidget(); currentPage != nullptr) {

					qDebug() << "selectFileSystemItem " << currentPage->metaObject()->className();

					if (currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::NavPage") ){
						const auto navPage = static_cast<NavPage *>(currentPage);
						navPage->setCurrentDir(dir->absolutePath());
						ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), dir->dirName());
						ui->tabWidget->update();

					} else if (currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::SearchPage") ){

						addPageToTabWidget(dir->absolutePath(), dir->dirName());
					}
				}
			}

			m_visitedPaths.push_back(path);

			delete dir;
			return true;
		} else {
			if (!suppress_warning)
				showWarning(message);
			delete dir;
			return false;
		}
	}

	void Files::locateInTree(const QString &path) {
		QDir dir(path);
		while (dir.cdUp()) {
			auto index = m_fileSystemModel->index(dir.path());
			ui->treeView->expand(index);
		}
		const auto index = m_fileSystemModel->index(path);
		ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
		ui->treeView->update();
	}

	void Files::onNewTabClicked() {
		addPageToTabWidget(QDir::homePath(), tr("Home"));
	}

	void Files::onHome() {
		if (const auto currentPage = ui->tabWidget->currentWidget(); currentPage != nullptr) {

			qDebug() << currentPage->metaObject()->className();

			if ( currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::NavPage") ) {
				const auto navPage = static_cast<NavPage *>(currentPage);
				navPage->setCurrentDir(QDir::homePath());
				ui->addressBar->setText(navPage->getCurrentDir());
				locateInTree(navPage->getCurrentDir());
				QDir::setCurrent(navPage->getCurrentDir());
			} else if (currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::SearchPage")) {

			}
		}
	}

	void Files::onTabWidgetCurrentChanged([[maybe_unused]] int index) {
		if (const auto currentPage = ui->tabWidget->currentWidget(); currentPage != nullptr) {

			qDebug() << currentPage->metaObject()->className();

			if ( currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::NavPage") ) {
				const auto navPage = static_cast<NavPage *>(currentPage);
				ui->addressBar->setText(navPage->getCurrentDir());
				locateInTree(navPage->getCurrentDir());
				QDir::setCurrent(navPage->getCurrentDir());
			} else if (currentPage->metaObject()->className() == QString("fairwindsk::ui::mydata::SearchPage")) {

			}
		}

	}

	void Files::onTabWidgetTabCloseRequested(const int index) {
		auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget());
		ui->tabWidget->removeTab(index);
		delete currentPage;
	}

	void Files::onAddressBarReturnPressed() {
		selectFileSystemItem(ui->addressBar->text(), CDSource::Navbar);
	}

	void Files::onTreeViewClicked(const QModelIndex &index) {
		selectFileSystemItem(m_fileSystemModel->filePath(index), CDSource::Navtree);
	}

	void Files::showWarning(const QString &message) {
		auto alert = new QMessageBox();
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
		if (const auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget())) {
			QStringList paths_to_delete = currentPage->getSelection();
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
	}

	void Files::onRenameClicked() {
		if (const auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget())) {
			QStringList selectedItems = currentPage->getSelection();
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
	}

	void Files::onCutClicked() {
		if (auto currentpage = static_cast<NavPage *>(ui->tabWidget->currentWidget())) {
			m_itemsToCopy.clear();
			m_itemsToMove = currentpage->getSelection();
			qDebug() << "Cut successfully";
		}
	}

	void Files::onCopyClicked() {
		if (const auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget())) {
			m_itemsToMove.clear();
			m_itemsToCopy = currentPage->getSelection();
			qDebug() << "Copied successfully";
		}
	}

	void Files::onPasteClicked() {
		if (const auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget())) {
			foreach (QString path, m_itemsToCopy) {
				QFileInfo item(path);
				qDebug() << "Name:" << item.fileName();
				qDebug() << "path: " << path;
				if (item.isFile()) {
					QString newPath = QDir::current().absoluteFilePath(item.fileName());
					qDebug() << "new path:" << newPath;
					qDebug() << "Copy result" << QFile::copy(path, newPath);
				} else {
					Utils::copyOrMoveDirectorySubtree(path, QDir::currentPath(), false, false);
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
					Utils::copyOrMoveDirectorySubtree(path, QDir::currentPath(), true, false);
				}
			}
		}
	}

	void Files::onCloseTabClicked() {
		const auto currentPage = static_cast<NavPage *>(ui->tabWidget->currentWidget());
		const int current_index = ui->tabWidget->currentIndex();
		ui->tabWidget->removeTab(current_index);
		delete currentPage;
	}

	void Files::onSearchClicked() {
		addSearchPage();
	}

	Files::~Files() {
	    delete ui;
	}
} // fairwindsk::ui::mydata
