//
// Created by Codex on 21/03/26.
//

#include "ResourceTab.hpp"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include "ResourceDialog.hpp"

namespace fairwindsk::ui::mydata {
    ResourceTab::ResourceTab(const ResourceKind kind, QWidget *parent)
        : QWidget(parent),
          m_kind(kind),
          m_model(new ResourceModel(kind, this)),
          m_proxyModel(new QSortFilterProxyModel(this)),
          m_searchEdit(new QLineEdit(this)),
          m_tableView(new QTableView(this)),
          m_addButton(new QToolButton(this)),
          m_editButton(new QToolButton(this)),
          m_deleteButton(new QToolButton(this)),
          m_refreshButton(new QToolButton(this)) {
        auto *mainLayout = new QVBoxLayout(this);
        auto *toolbarLayout = new QHBoxLayout();
        mainLayout->addLayout(toolbarLayout);

        m_searchEdit->setPlaceholderText(tr("Search %1").arg(resourceKindToTitle(kind).toLower()));
        connect(m_searchEdit, &QLineEdit::textChanged, this, &ResourceTab::onSearchTextChanged);
        toolbarLayout->addWidget(m_searchEdit, 1);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &ResourceTab::onRefreshClicked);
        toolbarLayout->addWidget(m_refreshButton);

        m_addButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_addButton->setToolTip(tr("Add"));
        connect(m_addButton, &QToolButton::clicked, this, &ResourceTab::onAddClicked);
        toolbarLayout->addWidget(m_addButton);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit"));
        connect(m_editButton, &QToolButton::clicked, this, &ResourceTab::onEditClicked);
        toolbarLayout->addWidget(m_editButton);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete"));
        connect(m_deleteButton, &QToolButton::clicked, this, &ResourceTab::onDeleteClicked);
        toolbarLayout->addWidget(m_deleteButton);

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setFilterKeyColumn(-1);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxyModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setSortingEnabled(true);
        m_tableView->sortByColumn(0, Qt::AscendingOrder);
        m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        connect(m_tableView, &QTableView::doubleClicked, this, &ResourceTab::onEditClicked);
        mainLayout->addWidget(m_tableView);
    }

    QModelIndex ResourceTab::currentSourceIndex() const {
        const QModelIndex proxyIndex = m_tableView->currentIndex();
        if (!proxyIndex.isValid()) {
            return {};
        }

        return m_proxyModel->mapToSource(proxyIndex);
    }

    void ResourceTab::selectResource(const QString &id) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                const QModelIndex sourceIndex = m_model->index(row, 0);
                const QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
                if (proxyIndex.isValid()) {
                    m_tableView->selectRow(proxyIndex.row());
                    m_tableView->scrollTo(proxyIndex);
                }
                break;
            }
        }
    }

    void ResourceTab::showError(const QString &message) const {
        QMessageBox::warning(const_cast<ResourceTab *>(this), resourceKindToTitle(m_kind), message);
    }

    void ResourceTab::onAddClicked() {
        ResourceDialog dialog(m_kind, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        const QString id = dialog.resourceId();
        if (!m_model->saveResource(id, dialog.resourceObject())) {
            showError(tr("Unable to create the selected resource."));
            return;
        }

        selectResource(id);
    }

    void ResourceTab::onEditClicked() {
        const QModelIndex index = currentSourceIndex();
        if (!index.isValid()) {
            showError(tr("Select a resource first."));
            return;
        }

        const QString id = m_model->resourceIdAtRow(index.row());
        ResourceDialog dialog(m_kind, this);
        dialog.setResource(id, m_model->resourceAtRow(index.row()));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        if (!m_model->saveResource(id, dialog.resourceObject())) {
            showError(tr("Unable to update the selected resource."));
            return;
        }

        selectResource(id);
    }

    void ResourceTab::onDeleteClicked() {
        const QModelIndex index = currentSourceIndex();
        if (!index.isValid()) {
            showError(tr("Select a resource first."));
            return;
        }

        const QString id = m_model->resourceIdAtRow(index.row());
        const QString name = m_model->resourceAtRow(index.row())["name"].toString();
        const QString singular = resourceKindToTitle(m_kind).toLower().chopped(1);
        const auto choice = QMessageBox::question(
                this,
                resourceKindToTitle(m_kind),
                tr("Delete %1 \"%2\"?").arg(singular, name));
        if (choice != QMessageBox::Yes) {
            return;
        }

        if (!m_model->deleteResource(id)) {
            showError(tr("Unable to delete the selected resource."));
        }
    }

    void ResourceTab::onRefreshClicked() {
        m_model->reload();
    }

    void ResourceTab::onSearchTextChanged(const QString &text) {
        const QRegularExpression expression(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption);
        m_proxyModel->setFilterRegularExpression(expression);
    }
}
