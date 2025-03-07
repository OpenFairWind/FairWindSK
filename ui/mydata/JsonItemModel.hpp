//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef JSONITEMMODEL_H
#define JSONITEMMODEL_H

#include <QAbstractItemModel>

#include "JsonTreeItem.hpp"


namespace fairwindsk::ui::mydata
{
    class JsonItemModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit JsonItemModel(QObject *parent = nullptr);
        JsonItemModel(const QJsonDocument& doc, QObject *parent = nullptr);
        ~JsonItemModel();
        QVariant data(const QModelIndex &index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex & = QModelIndex()) const override { return 2; };

    private:
        JsonTreeItem *m_rootItem = nullptr;
        QStringList m_headers;
        static JsonTreeItem *itemFromIndex(const QModelIndex &index)
        {return static_cast<JsonTreeItem*>(index.internalPointer()); }
    };
}


#endif //JSONITEMMODEL_H
