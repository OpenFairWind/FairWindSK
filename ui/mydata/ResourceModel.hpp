//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP

#include <QAbstractTableModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

class QTimer;

namespace fairwindsk::ui::mydata {

    enum class ResourceKind {
        Waypoint,
        Route,
        Region,
        Note,
        Chart
    };

    QString resourceKindToCollection(ResourceKind kind);
    QString resourceKindToTitle(ResourceKind kind);
    QString resourceKindToSingularTitle(ResourceKind kind);

    class ResourceModel final : public QAbstractTableModel {
        Q_OBJECT

    public:
        explicit ResourceModel(ResourceKind kind, QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        void reload(bool force = false);
        QString createResource(const QJsonObject &resource);
        bool updateResource(const QString &id, const QJsonObject &resource);
        bool deleteResource(const QString &id);
        bool importDocument(const QJsonDocument &document, QString *message, int *count = nullptr);
        QJsonDocument exportDocument(const QStringList &ids = {}) const;

        QString resourceIdAtRow(int row) const;
        QJsonObject resourceAtRow(int row) const;
        ResourceKind kind() const;
        QString collection() const;

    private:
        ResourceKind m_kind;
        QList<QString> m_ids;
        QMap<QString, QJsonObject> m_resources;
        QTimer *m_reloadTimer = nullptr;
        bool m_reloadInProgress = false;
        bool m_reloadPending = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCEMODEL_HPP
