//
// Created by Raffaele Montella on 23/04/25.
//

#include "FairWindSK.hpp"
#include "WaypointsModel.hpp"

namespace fairwindsk::ui::mydata {

    WaypointsModel::WaypointsModel(QObject *parent)
    : QAbstractTableModel(parent) {

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        // Get units converter instance
        m_units = Units::getInstance();

        m_waypoints = fairWindSK->getSignalKClient()->getWaypoints();


        auto jsonObject = fairWindSK->getSignalKClient()->signalkGet("vessels.self.navigation.position.value");

        m_position.setLatitude(jsonObject["latitude"].toDouble());
        m_position.setLongitude(jsonObject["longitude"].toDouble());

    }

    void WaypointsModel::sort(int column, Qt::SortOrder order) {
        qDebug() << "WaypointsModel::sort";
    }

    int WaypointsModel::rowCount(const QModelIndex & /*parent*/) const
    {
        return m_waypoints.keys().size();
    }

    int WaypointsModel::columnCount(const QModelIndex & /*parent*/) const {
        return 8;
    }

    QVariant WaypointsModel::headerData(int section, Qt::Orientation orientation, int role) const {

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        auto configuration = fairWindSK->getConfiguration();

        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            switch (section) {
                case WaypointsModel::Columns::Type:
                    return QString(tr("Type"));
                case WaypointsModel::Columns::Name:
                    return QString(tr("Name"));
                case WaypointsModel::Columns::Description:
                    return QString(tr("Description"));
                case WaypointsModel::Columns::Latitude:
                    return QString(tr("Latitude"));
                case WaypointsModel::Columns::Longitude:
                    return QString(tr("Longitude"));
                case WaypointsModel::Columns::Timestamp:
                    return QString(tr("Timestamp"));
                case WaypointsModel::Columns::Distance:
                    return QString(tr("Distance") + " (" + configuration->getDistanceUnits()+")");
                case WaypointsModel::Columns::Bearing:
                    return QString(tr("Bearing")  + " (°)");
                default:
                    return QString("");
            }
        }
        return QVariant();
    }

    QVariant WaypointsModel::data(const QModelIndex &index, int role) const
    {
        double value;
        QString text;



        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();



        const int row = index.row();
        const int col = index.column();

        const auto key = m_waypoints.keys()[row];

        if (role == Qt::DecorationRole) {
            auto type = m_waypoints[key].getType();

            if (col == WaypointsModel::Columns::Type) {
                if (type=="pseudoaton") {
                    return QIcon(":resources/svg/OpenBridge/chart-aton-iec.svg");
                } else if (type=="whale") {
                    return QIcon(":resources/svg/OpenBridge/whale.svg");
                }  else if (type=="alarm-mob") {
                    return QIcon(":resources/svg/OpenBridge/alarm-pob.svg");
                }
            }
            return QIcon("");
        }
        else if (role == Qt::DisplayRole) {

            switch (col) {
                case WaypointsModel::Columns::Name:
                    return QString(m_waypoints[key].getName());
                case WaypointsModel::Columns::Description:
                    return QString(m_waypoints[key].getDescription());
                case WaypointsModel::Columns::Latitude:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[0]);
                case WaypointsModel::Columns::Longitude:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[1]);
                case WaypointsModel::Columns::Timestamp:
                    return QString(m_waypoints[key].getTimestamp().toString());
                case WaypointsModel::Columns::Distance:

                    // Get the distance to the waypoint
                    value = m_position.distanceTo(m_waypoints[key].getCoordinates());

                    // Convert m/s to knots
                    value = m_units->convert("m",configuration->getDistanceUnits(), value);

                    // Build the formatted value
                    text = m_units->format(configuration->getVesselSpeedUnits(), value);

                    // Return the text
                    return text;
                case WaypointsModel::Columns::Bearing:

                    // Get the bearing to the waypoint
                    value = m_position.azimuthTo(m_waypoints[key].getCoordinates());

                    // Build the formatted value
                    text = m_units->format("deg", value);

                    // Return the text
                    return text;

                    default:
                        return QString();
            }

        } else if (role == Qt::EditRole) {

            switch (col) {
                case WaypointsModel::Columns::Name:
                    return QString(m_waypoints[key].getName());
                case WaypointsModel::Columns::Description:
                    return QString(m_waypoints[key].getDescription());
                case WaypointsModel::Columns::Latitude:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[0]);
                case WaypointsModel::Columns::Longitude:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[1]);
                case WaypointsModel::Columns::Timestamp:
                    return QString(m_waypoints[key].getTimestamp().toString());
                case WaypointsModel::Columns::Distance:

                    // Get the distance to the waypoint
                    value = m_position.distanceTo(m_waypoints[key].getCoordinates());

                    // Convert m/s to knots
                    value = m_units->convert("m",configuration->getDistanceUnits(), value);

                    // Build the formatted value
                    text = m_units->format(configuration->getVesselSpeedUnits(), value);

                    // Return the text
                    return text;
                case WaypointsModel::Columns::Bearing:

                    // Get the bearing to the waypoint
                    value = m_position.azimuthTo(m_waypoints[key].getCoordinates());

                    // Build the formatted value
                    text = m_units->format("deg", value);

                    // Return the text
                    return text;

                default:
                    return QString();
            }
        } else if (role == Qt::TextAlignmentRole)
        {
            switch (col) {
                case WaypointsModel::Columns::Latitude:
                case WaypointsModel::Columns::Longitude:
                    return Qt::AlignRight;
                case WaypointsModel::Columns::Distance:
                case WaypointsModel::Columns::Bearing:
                    return Qt::AlignCenter;
                default:
                    return Qt::AlignLeft;
            }
        }

        return QVariant();
    }

    bool WaypointsModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::EditRole) {

            const int row = index.row();
            const int col = index.column();

            auto key = m_waypoints.keys()[row];

            if (!checkIndex(index))
                return false;

            switch (col) {
                case WaypointsModel::Columns::Name:
                    m_waypoints[key].setName(value.toString());
                    break;

                case WaypointsModel::Columns::Description:
                    m_waypoints[key].setDescription(value.toString());
                    break;

                default:
                    break;
            }


            emit editCompleted(value.toString());
            return true;
        }
        return false;
    }

    Qt::ItemFlags WaypointsModel::flags(const QModelIndex &index) const
    {
        const int col = index.column();
        if (col == WaypointsModel::Columns::Name || col == WaypointsModel::Columns::Description) {
            return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
        }

        return QAbstractTableModel::flags(index);

    }
} // fairwindsk::ui::mydata