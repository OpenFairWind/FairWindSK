//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP

#include <QModelIndex>
#include <QWidget>

#include "ResourceModel.hpp"

class QLineEdit;
class QSortFilterProxyModel;
class QTableView;
class QToolButton;

namespace fairwindsk::ui::mydata {

    class ResourceTab final : public QWidget {
        Q_OBJECT

    public:
        explicit ResourceTab(ResourceKind kind, QWidget *parent = nullptr);

    private slots:
        void onAddClicked();
        void onEditClicked();
        void onDeleteClicked();
        void onImportClicked();
        void onExportClicked();
        void onRefreshClicked();
        void onSearchTextChanged(const QString &text);

    private:
        QModelIndex currentSourceIndex() const;
        void selectResource(const QString &id);
        void showError(const QString &message) const;

        ResourceKind m_kind;
        ResourceModel *m_model;
        QSortFilterProxyModel *m_proxyModel;
        QLineEdit *m_searchEdit;
        QTableView *m_tableView;
        QToolButton *m_addButton;
        QToolButton *m_editButton;
        QToolButton *m_deleteButton;
        QToolButton *m_importButton;
        QToolButton *m_exportButton;
        QToolButton *m_refreshButton;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP
