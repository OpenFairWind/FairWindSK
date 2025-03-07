//
// Created by Raffaele Montella on 06/03/25.
//

#include "JsonItemModel.hpp"

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonObject>

#include "JsonTreeItem.hpp"

namespace fairwindsk::ui::mydata {

    JsonItemModel::JsonItemModel(QObject *parent): QAbstractItemModel(parent), m_rootItem{new JsonTreeItem} {
        m_headers.append("Key");
        m_headers.append("Value");
    }

JsonItemModel::JsonItemModel(const QJsonDocument &doc, QObject *parent): QAbstractItemModel(parent), m_rootItem{new JsonTreeItem} {
    // Append header lines and return on empty document
    m_headers.append("Key");
    m_headers.append("Value");
    if (doc.isNull())
        return;

    // Reset the model. Root can either be a value or an array.
    beginResetModel();
    delete m_rootItem;
    if (doc.isArray()) {
        m_rootItem = JsonTreeItem::load(QJsonValue(doc.array()));
        m_rootItem->setType(QJsonValue::Array);

    } else {
        m_rootItem = JsonTreeItem::load(QJsonValue(doc.object()));
        m_rootItem->setType(QJsonValue::Object);
    }
    endResetModel();
}

JsonItemModel::~JsonItemModel() {
    delete m_rootItem;
}

QVariant JsonItemModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid())
        return {};

    JsonTreeItem *item = itemFromIndex(index);

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == 0)
            return item->key();
        if (index.column() == 1)
            return item->value();
        break;
    case Qt::EditRole:
        if (index.column() == 1)
            return item->value();
        break;
    default:
        break;
    }
    return {};
}

QVariant JsonItemModel::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal)
        return m_headers.value(section);
    else
        return {};
}

QModelIndex JsonItemModel::index(const int row, const int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    JsonTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = itemFromIndex(parent);

    JsonTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return {};
}

QModelIndex JsonItemModel::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return {};

    JsonTreeItem *childItem = itemFromIndex(index);
    JsonTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int JsonItemModel::rowCount(const QModelIndex &parent) const {
    JsonTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = itemFromIndex(parent);

    return parentItem->childCount();
}

}
