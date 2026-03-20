//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP

#include <QAbstractTableModel>
#include <QJsonObject>
#include <QString>

namespace fairwindsk::ui::mydata {

    enum class ResourceKind {
        Waypoint,
        Route,
        Track
    };

    QString resourceKindToCollection(ResourceKind kind);
    QString resourceKindToTitle(ResourceKind kind);

    class ResourceModel final : public QAbstractTableModel {
        Q_OBJECT

    public:
        explicit ResourceModel(ResourceKind kind, QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        void reload();
        bool saveResource(const QString &id, const QJsonObject &resource);
        bool deleteResource(const QString &id);

        QString resourceIdAtRow(int row) const;
        QJsonObject resourceAtRow(int row) const;
        ResourceKind kind() const;

    private:
        ResourceKind m_kind;
        QList<QString> m_ids;
        QMap<QString, QJsonObject> m_resources;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP
