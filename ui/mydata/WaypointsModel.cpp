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
                case 0:
                    return QString(tr("Name"));
                case 1:
                    return QString(tr("Description"));
                case 2:
                    return QString(tr("Type"));
                case 3:
                    return QString(tr("Latitude"));
                case 4:
                    return QString(tr("Longitude"));
                case 5:
                    return QString(tr("Timestamp"));
                case 6:
                    return QString(tr("Distance") + " (" + configuration->getDistanceUnits()+")");
                case 7:
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

        if (role == Qt::DisplayRole) {

            switch (col) {
                case 0:
                    return QString(m_waypoints[key].getName());
                case 1:
                    return QString(m_waypoints[key].getDescription());
                case 2:
                    return QString(m_waypoints[key].getType());
                case 3:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[0]);
                case 4:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[1]);
                case 5:
                    return QString(m_waypoints[key].getTimestamp().toString());
            case 6:

                // Get the distance to the waypoint
                value = m_position.distanceTo(m_waypoints[key].getCoordinates());

                // Convert m/s to knots
                value = m_units->convert("m",configuration->getDistanceUnits(), value);

                // Build the formatted value
                text = m_units->format(configuration->getVesselSpeedUnits(), value);

                // Return the text
                return text;
            case 7:

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
                case 0:
                    return QString(m_waypoints[key].getName());
                case 1:
                    return QString(m_waypoints[key].getDescription());
                case 2:
                    return QString(m_waypoints[key].getType());
                case 3:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[0]);
                case 4:
                    return QString(m_waypoints[key].getCoordinates().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",")[1]);
                case 5:
                    return QString(m_waypoints[key].getTimestamp().toString());
                case 6:

                    // Get the distance to the waypoint
                    value = m_position.distanceTo(m_waypoints[key].getCoordinates());

                    // Convert m/s to knots
                    value = m_units->convert("m",configuration->getDistanceUnits(), value);

                    // Build the formatted value
                    text = m_units->format(configuration->getVesselSpeedUnits(), value);

                    // Return the text
                    return text;
                case 7:

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
                case 3:
                case 4:
                    return Qt::AlignRight;
                case 6:
                case 7:
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
                case 0:
                    m_waypoints[key].setName(value.toString());
                    break;

                case 1:
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
        if (col == 0 || col == 1) {
            return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
        }

        return QAbstractTableModel::flags(index);

    }
} // fairwindsk::ui::mydata