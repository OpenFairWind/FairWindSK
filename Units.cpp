//
// Created by Raffaele Montella on 02/06/21.
//

#include <Units.hpp>


namespace fairwindsk {
/*
 * Units - Public Constructor
 */
    Units::Units() {
        mConverters["K"]["F"] = [](double value) { return value * 1.8 - 459.67; };
        mConverters["K"]["C"] = [](double value) { return value - 273.15; };
        mConverters["rad"]["deg"] = [](double value) { return value * 57.2958; };
        mConverters["ms-1"]["kn"] = [](double value) { return value * 1.94384; };
        mConverters["m"]["m"] = [](double value) { return value; };
        mConverters["m"]["nm"] = [](double value) { return value * 0.000539957; };
        mConverters["m"]["km"] = [](double value) { return value * 0.001; };
        mConverters["m"]["ft"] = [](double value) { return value * 0.546807; };
        mConverters["v"]["v"] = [](double value) { return value; };
        mConverters["L"]["L"] = [](double value) { return value; };
        mConverters["L"]["gal"] = [](double value) { return value * 0.264172; };
        mConverters["A"]["A"] = [](double value) { return value; };

        mLabels["F"] = "°F";
        mLabels["C"] = "°C";
        mLabels["deg"] = "°";
        mLabels["kn"] = "kn";
        mLabels["km"] = "km";
        mLabels["ft"] = "ft";
        mLabels["m"] = "m";
        mLabels["A"] = "A";
        mLabels["v"] = "v";
        mLabels["L"] = "L";
        mLabels["gal"] = "gal";
    }

/*
 * getInstance
 * Returns the existent instance, if any, otherwise creates a new one a returns it
 */
    Units *Units::getInstance() {
        if (m_instance == nullptr) {
            m_instance = new Units();
        }
        return m_instance;
    }

/*
 * convert
 * Converts a value from a unit to another
 */
    double Units::convert(const QString &srcUnit, const QString &unit, double value) {
        if (mConverters.contains(srcUnit) && mConverters[srcUnit].contains(unit)) {
            return mConverters[srcUnit][unit](value);
        }
        return value;
    }

/*
 * getLabel
 * Returns the unit's label
 */
    QString Units::getLabel(const QString &unit) {
        if (mLabels.contains(unit)) {
            return mLabels[unit];
        }
        return unit;
    }
}