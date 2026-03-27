//
// Created by Raffaele Montella on 02/06/21.
//

#ifndef FAIRWINDSK_UNITS_HPP
#define FAIRWINDSK_UNITS_HPP

#include <QString>
#include <QMap>
#include <QList>
#include <QObject>
#include <QSet>
#include <functional>
#include <nlohmann/json.hpp>


namespace fairwindsk {
    /*
     * Units
     * This class provides the most basic units and a quick way to convert and switch between them
     */
    class Units: public QObject {
        Q_OBJECT
    public:
        struct UnitOption {
            QString key;
            QString symbol;
            QString label;
        };

        struct UnitPreferenceItem {
            QString category;
            QString baseUnit;
            QString targetUnit;
            QString displayFormat;
            QString symbol;
            QList<UnitOption> options;
        };

        static Units *getInstance();
        double convert(const QString& srcUnit, const QString& unit, double value);
        QString getLabel(const QString &unit);
        QString format(const QString& unit, double value);
        double convertSignalKValue(const QString &path, double value, const QString &fallbackSourceUnit = QString(), const QString &fallbackTargetUnit = QString());
        QString formatSignalKValue(const QString &path, double value, const QString &fallbackSourceUnit = QString(), const QString &fallbackTargetUnit = QString());
        QString getSignalKUnitLabel(const QString &path, const QString &fallbackUnit = QString());
        QString getSignalKActivePresetName();
        QList<UnitPreferenceItem> getSignalKUnitPreferenceItems();
        QString getLocalUnitOverride(const QString &category) const;
        void setLocalUnitOverride(const QString &category, const QString &targetUnit);
        void clearLocalUnitOverride(const QString &category);
        void syncLocalUnitsFromServer();
        void refreshSignalKPreferences();
        nlohmann::json &getUnits();

    private:
        struct DisplayUnitsInfo {
            QString category;
            QString baseUnit;
            QString targetUnit;
            QString formula;
            QString inverseFormula;
            QString symbol;
            QString longName;
            QString displayFormat;
            bool valid = false;
        };

        Units();
        void loadSignalKPreferences();
        DisplayUnitsInfo displayUnitsForPath(const QString &path);
        DisplayUnitsInfo parseDisplayUnits(const QJsonObject &jsonObject) const;
        DisplayUnitsInfo localOverrideForCategory(const QString &category) const;
        QString categoryForPath(const QString &path) const;
        static QString pathPatternToRegex(const QString &pathPattern);
        static bool isNumericDisplayFormat(const QString &displayFormat);
        static double evaluateFormula(const QString &formula, double value, bool *ok = nullptr);
        static QString formatNumericValue(double value, const QString &displayFormat);
        static QString normalizeSourceUnit(const QString &unit);

        inline static Units *m_instance = nullptr;

        QMap <QString, QMap<QString, std::function<double(double)>>> mConverters;

        nlohmann::json m_units;
        bool m_signalKPreferencesLoaded = false;
        QString m_signalKActivePresetName;
        QMap<QString, DisplayUnitsInfo> m_categoryDisplayUnits;
        QMap<QString, QMap<QString, DisplayUnitsInfo>> m_definitionsByBaseUnit;
        QMap<QString, QString> m_pathPatternCategories;
        QMap<QString, DisplayUnitsInfo> m_displayUnitsCache;
        QSet<QString> m_attemptedDisplayUnitsPaths;
    };
}

#endif //FAIRWINDSK_UNITS_HPP
