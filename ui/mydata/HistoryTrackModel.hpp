//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_HISTORYTRACKMODEL_HPP
#define FAIRWINDSK_UI_MYDATA_HISTORYTRACKMODEL_HPP

#include <QAbstractTableModel>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QJsonDocument>

namespace fairwindsk::ui::mydata {

    struct HistoryTrackPoint {
        QDateTime timestamp;
        QGeoCoordinate coordinate;
    };

    class HistoryTrackModel final : public QAbstractTableModel {
        Q_OBJECT

    public:
        explicit HistoryTrackModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        bool reload(const QString &duration, const QString &resolution, QString *message = nullptr);
        bool importDocument(const QJsonDocument &document, QString *message = nullptr);
        QJsonDocument exportDocument() const;
        bool hasPoints() const;
        HistoryTrackPoint pointAtRow(int row) const;
        bool updatePointAtRow(int row, const HistoryTrackPoint &point);
        void appendPoint(const HistoryTrackPoint &point);
        bool removePointAtRow(int row);

    private:
        bool applyHistoryPayload(const QJsonDocument &document, QString *message);

        QList<HistoryTrackPoint> m_points;
        QJsonDocument m_sourceDocument;
        QString m_activePath;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_HISTORYTRACKMODEL_HPP
