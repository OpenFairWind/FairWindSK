//
// Created by Raffaele Montella on 06/03/25.
//

#include "JsonTreeItem.hpp"

#include <QJsonObject>
#include <QJsonArray>

namespace fairwindsk::ui::mydata
{
    JsonTreeItem::JsonTreeItem(JsonTreeItem *parent)
    {
        m_parent = parent;
    }

    JsonTreeItem::~JsonTreeItem()
    {
        qDeleteAll(m_children);
    }

    void JsonTreeItem::appendChild(JsonTreeItem *item)
    {
        m_children.append(item);
    }

    JsonTreeItem *JsonTreeItem::child(int row)
    {
        return m_children.value(row);
    }

    JsonTreeItem *JsonTreeItem::parent()
    {
        return m_parent;
    }

    int JsonTreeItem::childCount() const
    {
        return m_children.count();
    }

    int JsonTreeItem::row() const
    {
        if (m_parent)
            return m_parent->m_children.indexOf(const_cast<JsonTreeItem*>(this));

        return 0;
    }

    void JsonTreeItem::setKey(const QString &key)
    {
        m_key = key;
    }

    void JsonTreeItem::setValue(const QVariant &value)
    {
        m_value = value;
    }

    void JsonTreeItem::setType(const QJsonValue::Type &type)
    {
        m_type = type;
    }

    JsonTreeItem* JsonTreeItem::load(const QJsonValue& value, JsonTreeItem* parent)
    {
        auto *rootItem = new JsonTreeItem(parent);
        rootItem->setKey("root");

        if (value.isObject()) {
            const QStringList &keys = value.toObject().keys();
            for (const QString &key : keys) {
                QJsonValue v = value.toObject().value(key);
                JsonTreeItem *child = load(v, rootItem);
                child->setKey(key);
                child->setType(v.type());
                rootItem->appendChild(child);
            }
        } else if (value.isArray()) {
            int index = 0;
            const QJsonArray &array = value.toArray();
            for (const QJsonValue &val : array) {
                JsonTreeItem *child = load(val, rootItem);
                child->setKey(QString::number(index));
                child->setType(val.type());
                rootItem->appendChild(child);
                ++index;
            }
        } else {
            rootItem->setValue(value.toVariant());
            rootItem->setType(value.type());
        }

        return rootItem;
    }

}