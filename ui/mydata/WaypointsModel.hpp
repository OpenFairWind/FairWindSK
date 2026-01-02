//
// Created by Raffaele Montella on 23/04/25.
//

#ifndef WAYPOINTSMODEL_H
#define WAYPOINTSMODEL_H

#include <QAbstractTableModel>

#include <signalk/Waypoint.hpp>

#include "Files.hpp"
#include "Units.hpp"

namespace fairwindsk::ui::mydata {

    class WaypointsModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit WaypointsModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        void sort(int column, Qt::SortOrder order) override;

        enum Columns { Type, Name, Description, Latitude, Longitude, Distance, Bearing, Timestamp};

    signals:
        void editCompleted(const QString &);

    private:
        QMap<QString, signalk::Waypoint> m_waypoints;
        Units *m_units;
        QGeoCoordinate m_position;


};

} // fairwindsk::ui::mydata

#endif //WAYPOINTSMODEL_H
