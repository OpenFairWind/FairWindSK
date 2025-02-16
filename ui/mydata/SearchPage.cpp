//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_SearchPage.h" resolved

#include "SearchPage.hpp"
#include "ui_SearchPage.h"

#include <QDir>
#include <QDirIterator>

namespace fairwindsk::ui::mydata {
SearchPage::SearchPage(const QString& key, QWidget *parent) :
    QWidget(parent), ui(new Ui::SearchPage) {
    ui->setupUi(this);


    m_key = key;

    m_fileInfoListModel = new FileInfoListModel();
    ui->resultView->setModel(m_fileInfoListModel);

    doSearch();

}

    void SearchPage::doSearch() {

    Qt::CaseSensitivity caseSensitivity = ui->caseSensitive->isChecked() ? Qt::CaseSensitive
                                                                         : Qt::CaseInsensitive;
    m_results.clear();

    const QString oldTitle = this->windowTitle();
    this->setWindowTitle(tr("Searching...please wait")); //Inform the user

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;

    if (ui->searchHidden->isChecked())
        filters |= QDir::Hidden;
    if (ui->searchSystem->isChecked())
        filters |= QDir::System;

    QDirIterator dirItems(QDir::currentPath(), filters, QDirIterator::Subdirectories);

    while (dirItems.hasNext()) {
        if (const QFileInfo fileInfo = dirItems.nextFileInfo(); fileInfo.fileName().contains(m_key, caseSensitivity)) {
            m_results.append(fileInfo);
            qDebug() << fileInfo;
        }
    }

    static_cast<FileInfoListModel *>(ui->resultView->model())->setQFileInfoList(m_results);
    ui->resultView->resizeColumnsToContents();

    this->setWindowTitle(oldTitle);
}

SearchPage::~SearchPage() {
    if (m_fileInfoListModel)
    {
        delete m_fileInfoListModel;

        m_fileInfoListModel = nullptr;
    }

    delete ui;
}
} // fairwindsk::ui::mydata
