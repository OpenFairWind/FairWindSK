//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Navpage.h" resolved

#include "NavPage.hpp"
#include "ui_Navpage.h"

#include <QColumnView>
#include <QLayout>
#include <QListView>
#include <QProcess>
#include <QTableView>
#include <QTreeView>

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {

NavPage::NavPage(QFileSystemModel *model, Files *root, QWidget *parent)
	: QWidget(parent), ui(new Ui::NavPage), m_files(root), m_model(model)
{
	ui->setupUi(this);

	m_currentDir = nullptr;

	QListView *view = new QListView();
	view->setWordWrap(true);
	view->setViewMode(QListView::IconMode);
	view->setIconSize(QSize(48, 48));
	view->setGridSize(QSize(128, 72));
	view->setUniformItemSizes(true);
	view->setMovement(QListView::Static);
	view->setResizeMode(QListView::Adjust);
	view->setLayoutMode(QListView::Batched);
	view->setBatchSize(10);
	m_dirView = static_cast<QAbstractItemView *>(view);
	m_driveInfo = new DriveInfo(this);
	m_itemInfo = nullptr;

	setDirView();

	ui->verticalLayout->addWidget(m_dirView);
	ui->verticalLayout->addWidget(m_driveInfo);

	connect(m_dirView, &QAbstractItemView::activated, this, &NavPage::onItemViewActivated);
	connect(m_dirView, &QAbstractItemView::clicked, this, &NavPage::onItemViewClicked);
}

NavPage::~NavPage()
{

	delete m_dirView;
	delete m_itemInfo;
	delete m_driveInfo;
	delete m_currentDir;

	delete ui;
}

int NavPage::setCurrentDir(QString path)
{
	if (m_currentDir) {
		delete m_currentDir;
	}
	m_currentDir = new QDir(path);
	auto index = m_model->index(path);
	qDebug() << index;
	m_dirView->setRootIndex(index);
	m_driveInfo->refreshDrive(path);
	return 0;
}

void NavPage::setDirView()
{
	if (m_dirView->objectName().isEmpty()) {
		m_dirView->setObjectName("dirView");
	}

	m_dirView->resize(0, 0);
	m_dirView->adjustSize();
	QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	sizePolicy.setHorizontalStretch(1);
	sizePolicy.setVerticalStretch(1);
	//	sizePolicy.setHeightForWidth(dirView->sizePolicy().hasHeightForWidth());
	m_dirView->setSizePolicy(sizePolicy);

	m_dirView->setAcceptDrops(true);

	m_dirView->setModel(m_model);
	qDebug() << m_dirView;
	const auto index = m_model->index(QDir::currentPath());
	qDebug() << index;
	m_dirView->setRootIndex(index);
}

void NavPage::onItemViewActivated(const QModelIndex &index)
{
	qDebug() << index;

	const auto path = m_model->filePath(index);

	if (m_files->selectFileSystemItem(path, Files::CDSource::Navpage, true))
		setCurrentDir(path);
	else {
		qDebug() << "Should open the file here.";

		const QFileInfo info(path);
		
		m_files->addPageToTabWidget(path,info.fileName());
	}
}

QStringList NavPage::getSelection()
{
	QModelIndexList list = m_dirView->selectionModel()->selectedIndexes();
	QStringList path_list;
	foreach (const QModelIndex &index, list) {
		path_list.append(m_model->filePath(index));
	}
	qDebug() << path_list.join(",");
	return path_list;
}

void NavPage::onItemViewClicked(const QModelIndex &index)
{
	QFileInfo fileInfo = m_model->fileInfo(index);
	if (m_itemInfo) {
		ui->verticalLayout->removeWidget(m_itemInfo);
		delete m_itemInfo;
	}
	m_itemInfo = new DirItemInfo(&fileInfo, this);
	m_itemInfo->refresh();
	ui->verticalLayout->addWidget(m_itemInfo);
	qDebug() << index;
}
} // fairwindsk::ui::mydata
