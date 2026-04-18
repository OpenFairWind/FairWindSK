//
// Created by Codex on 18/04/26.
//

#include "RoutesLite.hpp"

#include <algorithm>

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

namespace fairwindsk::ui::mydata {
    namespace {
        constexpr int kRouteColumnCount = 5;
        constexpr int kRouteRowHeight = 62;
    }

    RoutesLite::RoutesLite(QWidget *parent)
        : QWidget(parent),
          m_model(new ResourceModel(ResourceKind::Route, this)),
          m_searchEdit(new QLineEdit(this)),
          m_refreshButton(new QToolButton(this)),
          m_tableWidget(new QTableWidget(this)) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        auto *titleLabel = new QLabel(tr("Routes"), this);
        titleLabel->setObjectName(QStringLiteral("routesLiteTitleLabel"));
        layout->addWidget(titleLabel);

        auto *toolbarLayout = new QHBoxLayout();
        toolbarLayout->setContentsMargins(0, 0, 0, 0);
        toolbarLayout->setSpacing(12);

        m_searchEdit->setClearButtonEnabled(true);
        m_searchEdit->setPlaceholderText(tr("Search routes by name, description, geometry, or timestamp"));
        toolbarLayout->addWidget(m_searchEdit, 1);

        m_refreshButton->setText(tr("Refresh"));
        m_refreshButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        m_refreshButton->setMinimumHeight(44);
        toolbarLayout->addWidget(m_refreshButton);

        layout->addLayout(toolbarLayout);

        configureTable();
        layout->addWidget(m_tableWidget, 1);

        connect(m_model, &QAbstractItemModel::modelReset, this, &RoutesLite::rebuildTable);
        connect(m_searchEdit, &QLineEdit::textChanged, this, &RoutesLite::onSearchTextChanged);
        connect(m_refreshButton, &QToolButton::clicked, this, &RoutesLite::onRefreshClicked);

        rebuildTable();
    }

    void RoutesLite::rebuildTable() {
        const QString filter = m_searchEdit->text().trimmed();
        const QString filterLower = filter.toLower();

        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->clearContents();
        m_tableWidget->setRowCount(0);

        int visibleRow = 0;
        for (int row = 0; row < m_model->rowCount(); ++row) {
            QStringList searchableValues;
            searchableValues.reserve(kRouteColumnCount);
            for (int column = 0; column < kRouteColumnCount; ++column) {
                searchableValues.append(m_model->data(m_model->index(row, column), Qt::DisplayRole).toString());
            }

            if (!filterLower.isEmpty()) {
                const bool matches = std::any_of(searchableValues.cbegin(), searchableValues.cend(), [&filterLower](const QString &value) {
                    return value.toLower().contains(filterLower);
                });
                if (!matches) {
                    continue;
                }
            }

            m_tableWidget->insertRow(visibleRow);
            m_tableWidget->setRowHeight(visibleRow, kRouteRowHeight);

            for (int column = 0; column < kRouteColumnCount; ++column) {
                auto *item = new QTableWidgetItem(searchableValues.at(column));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setTextAlignment(column == 3 ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignVCenter);
                m_tableWidget->setItem(visibleRow, column, item);
            }

            ++visibleRow;
        }

        m_tableWidget->setSortingEnabled(true);
        m_tableWidget->sortByColumn(0, Qt::AscendingOrder);
    }

    void RoutesLite::onSearchTextChanged(const QString &text) {
        Q_UNUSED(text)
        rebuildTable();
    }

    void RoutesLite::onRefreshClicked() {
        m_model->reload(true);
    }

    void RoutesLite::configureTable() {
        m_tableWidget->setColumnCount(kRouteColumnCount);
        m_tableWidget->setHorizontalHeaderLabels({
            tr("Name"),
            tr("Description"),
            tr("Geometry"),
            tr("Points"),
            tr("Timestamp")
        });
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setFocusPolicy(Qt::StrongFocus);
        m_tableWidget->setWordWrap(false);
        m_tableWidget->verticalHeader()->setVisible(false);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(kRouteRowHeight);
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    }
}
