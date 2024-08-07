//
// Created by Raffaele Montella on 02/06/21.
//

#ifndef FAIRWINDSK_UNITS_HPP
#define FAIRWINDSK_UNITS_HPP

#include <QString>
#include <QMap>
#include <QObject>
#include <nlohmann/json.hpp>


namespace fairwindsk {
    /*
     * Units
     * This class provides the most basic units and a quick way to convert and switch between them
     */
    class Units: public QObject {
        Q_OBJECT
    public:
        static Units *getInstance();
        double convert(const QString& srcUnit, const QString& unit, double value);
        QString getLabel(const QString &unit);
        QString format(const QString& unit, double value);
        nlohmann::json &getUnits();

    private:
        Units();

        inline static Units *m_instance = nullptr;

        QMap <QString, QMap<QString, std::function<double(double)>>> mConverters;


        nlohmann::json m_units;
    };
}

#endif //FAIRWINDSK_UNITS_HPP