//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef JSONTREEITEM_H
#define JSONTREEITEM_H

#include <QJsonValue>
#include <QJsonDocument>
#include <QAbstractItemModel>


namespace fairwindsk::ui::mydata
{
    class JsonTreeItem
    {
    public:
        explicit JsonTreeItem(JsonTreeItem *parent = nullptr);
        ~JsonTreeItem();
        void appendChild(JsonTreeItem *item);
        JsonTreeItem *child(int row);
        JsonTreeItem *parent();
        int childCount() const;
        int row() const;
        void setKey(const QString& key);
        void setValue(const QVariant& value);
        void setType(const QJsonValue::Type& type);
        QString key() const { return m_key; };
        QVariant value() const { return m_value; };
        QJsonValue::Type type() const { return m_type; };

        static JsonTreeItem* load(const QJsonValue& value, JsonTreeItem *parent = nullptr);

    private:
        QString m_key;
        QVariant m_value;
        QJsonValue::Type m_type;
        QList<JsonTreeItem*> m_children;
        JsonTreeItem *m_parent = nullptr;
    };
}

#endif //JSONTREEITEM_H
