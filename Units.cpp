//
// Created by Raffaele Montella on 02/06/21.
//

#include <Units.hpp>
#include <fstream>
#include <QFile>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include "FairWindSK.hpp"

namespace fairwindsk {
/*
 * Units - Public Constructor
 */
    Units::Units() {

        QString data;
        QString fileName(":/resources/json/units.json");

        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly)) {

            data = file.readAll();
            m_units= nlohmann::json::parse(data.toStdString());
        }

        file.close();

        mConverters["K"]["F"] = [](double value) { return value * 1.8 - 459.67; };
        mConverters["K"]["C"] = [](double value) { return value - 273.15; };
        mConverters["rad"]["deg"] = [](double value) { return value * 57.2958; };
        mConverters["ms-1"]["kn"] = [](double value) { return value * 1.94384; };
        mConverters["m"]["m"] = [](double value) { return value; };
        mConverters["m"]["nm"] = [](double value) { return value * 0.000539957; };
        mConverters["m"]["km"] = [](double value) { return value * 0.001; };
        mConverters["m"]["ft"] = [](double value) { return value * 3.28084; };
        mConverters["v"]["v"] = [](double value) { return value; };
        mConverters["L"]["L"] = [](double value) { return value; };
        mConverters["L"]["gal"] = [](double value) { return value * 0.264172; };
	mConverters["A"]["A"] = [](double value) { return value; };
	mConverters["mt"]["mt"] = [](double value) { return value; };
        mConverters["mt"]["ftm"] = [](double value) { return value * 0.547; };
        mConverters["mt"]["ft"] = [](double value) { return value * 3.28084; };
        mConverters["rm"]["rm"] = [](double value) { return value; };
        mConverters["rm"]["rft"] = [](double value) { return value * 3.28084; };

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
        QString result = unit;
        if (m_units["types"].contains(unit.toStdString())) {
            result = QString::fromStdString(m_units["types"][unit.toStdString()]["label"].get<std::string>());
        }
        return result;
    }

    QString Units::format(const QString &unit, double value) {
        QString result;

        if (unit == "deg") {
            result = QString{"%1"}.arg(value, 4, 'f', 1, '0');
        }
        else if (unit == "kn") {
            if (value >= 0.0 and value <=10.0) {
                result = QString{"%1"}.arg(value, 3, 'f', 2, '0');
            }
            else {
                result = QString{"%1"}.arg(value, 3, 'f', 1, '0');
            }
        }
        else if (unit == "nm") {
            if (value >= 0.0 and value <=10.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 2 );
            }
            else if (value >= 10.0 and value <=100.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 1 );
            }
            else {
                result = QString{"%1"}.arg(value, 4, 'f', 0 );
            }
        }
        else if (unit == "mt") {
            if (value >= 0.0 and value <=10.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 2 );
            }
            else if (value >= 10.0 and value <=100.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 1 );
            }
            else {
                result = QString{"%1"}.arg(value, 4, 'f', 0 );
            }
        }
        else if (unit == "rm") {
            if (value >= 0.0 and value <=10.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 2 );
            }
            else if (value >= 10.0 and value <=100.0) {
                result = QString{"%1"}.arg(value, 4, 'f', 1 );
            }
            else {
                result = QString{"%1"}.arg(value, 4, 'f', 0 );
            }
        }
	

        return result;
    }

    void Units::refreshSignalKPreferences() {
        m_signalKPreferencesLoaded = false;
        m_categoryDisplayUnits.clear();
        m_pathPatternCategories.clear();
        m_displayUnitsCache.clear();
        m_attemptedDisplayUnitsPaths.clear();
        loadSignalKPreferences();
    }

    void Units::loadSignalKPreferences() {
        if (m_signalKPreferencesLoaded) {
            return;
        }

        m_signalKPreferencesLoaded = true;

        const auto fairWindSK = FairWindSK::getInstance();
        if (!fairWindSK) {
            return;
        }

        const auto client = fairWindSK->getSignalKClient();
        if (!client || client->server().isEmpty()) {
            return;
        }

        const auto activeUrl = QUrl(client->server().toString() + "/signalk/v1/unitpreferences/active");
        const auto activePreset = client->signalkGet(activeUrl);
        if (activePreset.contains("categories") && activePreset["categories"].isObject()) {
            const auto categories = activePreset["categories"].toObject();
            for (auto it = categories.begin(); it != categories.end(); ++it) {
                if (!it.value().isObject()) {
                    continue;
                }
                auto displayUnits = parseDisplayUnits(it.value().toObject());
                displayUnits.category = it.key();
                if (displayUnits.valid) {
                    m_categoryDisplayUnits.insert(it.key(), displayUnits);
                }
            }
        }

        const auto categoriesUrl = QUrl(client->server().toString() + "/signalk/v1/unitpreferences/default-categories");
        const auto categoriesObject = client->signalkGet(categoriesUrl);
        if (categoriesObject.contains("categories") && categoriesObject["categories"].isObject()) {
            const auto categories = categoriesObject["categories"].toObject();
            for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
                if (!categoryIt.value().isObject()) {
                    continue;
                }
                const auto categoryObject = categoryIt.value().toObject();
                if (!categoryObject.contains("paths") || !categoryObject["paths"].isArray()) {
                    continue;
                }
                const auto paths = categoryObject["paths"].toArray();
                for (const auto &pathValue : paths) {
                    if (pathValue.isString()) {
                        m_pathPatternCategories.insert(pathValue.toString(), categoryIt.key());
                    }
                }
            }
        }
    }

    Units::DisplayUnitsInfo Units::parseDisplayUnits(const QJsonObject &jsonObject) const {
        DisplayUnitsInfo info;
        info.category = jsonObject.value("category").toString();
        info.targetUnit = jsonObject.value("targetUnit").toString();
        info.formula = jsonObject.value("formula").toString();
        info.inverseFormula = jsonObject.value("inverseFormula").toString();
        info.symbol = jsonObject.value("symbol").toString();
        info.displayFormat = jsonObject.value("displayFormat").toString();
        info.valid = !info.targetUnit.isEmpty() || !info.symbol.isEmpty() || !info.formula.isEmpty();
        return info;
    }

    QString Units::pathPatternToRegex(const QString &pathPattern) {
        QString escaped = QRegularExpression::escape(pathPattern);
        escaped.replace(QStringLiteral("\\*"), QStringLiteral("[^.]+"));
        return QStringLiteral("^%1$").arg(escaped);
    }

    QString Units::categoryForPath(const QString &path) const {
        for (auto it = m_pathPatternCategories.constBegin(); it != m_pathPatternCategories.constEnd(); ++it) {
            const QRegularExpression regex(pathPatternToRegex(it.key()));
            if (regex.match(path).hasMatch()) {
                return it.value();
            }
        }
        return {};
    }

    Units::DisplayUnitsInfo Units::displayUnitsForPath(const QString &path) {
        if (path.isEmpty()) {
            return {};
        }

        loadSignalKPreferences();

        if (m_displayUnitsCache.contains(path)) {
            return m_displayUnitsCache.value(path);
        }

        if (!m_attemptedDisplayUnitsPaths.contains(path)) {
            m_attemptedDisplayUnitsPaths.insert(path);

            const auto fairWindSK = FairWindSK::getInstance();
            const auto client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
            if (client && !client->server().isEmpty()) {
                QString pathComponent = path;
                pathComponent.replace('.', '/');
                const auto metaUrl = QUrl(client->server().toString() + "/signalk/v1/api/vessels/self/" + pathComponent + "/meta");
                const auto metaObject = client->signalkGet(metaUrl);
                if (metaObject.contains("displayUnits") && metaObject["displayUnits"].isObject()) {
                    auto info = parseDisplayUnits(metaObject["displayUnits"].toObject());
                    if (info.valid) {
                        m_displayUnitsCache.insert(path, info);
                        return info;
                    }
                }
            }

            const QString category = categoryForPath(path);
            if (!category.isEmpty() && m_categoryDisplayUnits.contains(category)) {
                auto info = m_categoryDisplayUnits.value(category);
                if (info.category.isEmpty()) {
                    info.category = category;
                }
                m_displayUnitsCache.insert(path, info);
                return info;
            }
        }

        return {};
    }

    QString Units::normalizeSourceUnit(const QString &unit) {
        if (unit == "ms-1") {
            return "m/s";
        }
        if (unit == "mt" || unit == "rm") {
            return "m";
        }
        if (unit == "deg") {
            return "deg";
        }
        return unit;
    }

    bool Units::isNumericDisplayFormat(const QString &displayFormat) {
        static const QRegularExpression pattern(QStringLiteral("^0(?:\\.0+)?$"));
        return displayFormat.isEmpty() || pattern.match(displayFormat).hasMatch();
    }

    double Units::evaluateFormula(const QString &formula, const double value, bool *ok) {
        if (ok) {
            *ok = false;
        }
        if (formula.trimmed().isEmpty()) {
            if (ok) {
                *ok = true;
            }
            return value;
        }

        QString jsExpression = formula;
        jsExpression.replace('^', QStringLiteral("**"));
        jsExpression.remove('_');

        QJSEngine engine;
        engine.globalObject().setProperty(QStringLiteral("value"), value);
        const QJSValue result = engine.evaluate(jsExpression);
        if (result.isError() || !result.isNumber()) {
            return value;
        }

        if (ok) {
            *ok = true;
        }
        return result.toNumber();
    }

    QString Units::formatNumericValue(const double value, const QString &displayFormat) {
        if (!isNumericDisplayFormat(displayFormat)) {
            return QString::number(value, 'f', 2);
        }

        const int dotIndex = displayFormat.indexOf('.');
        const int decimals = dotIndex >= 0 ? displayFormat.size() - dotIndex - 1 : 0;
        return QString::number(value, 'f', decimals);
    }

    double Units::convertSignalKValue(const QString &path,
                                      const double value,
                                      const QString &fallbackSourceUnit,
                                      const QString &fallbackTargetUnit) {
        const auto info = displayUnitsForPath(path);
        if (info.valid && !info.formula.isEmpty() && isNumericDisplayFormat(info.displayFormat)) {
            bool ok = false;
            const double converted = evaluateFormula(info.formula, value, &ok);
            if (ok) {
                return converted;
            }
        }

        const QString normalizedSourceUnit = normalizeSourceUnit(fallbackSourceUnit);
        return convert(normalizedSourceUnit, fallbackTargetUnit, value);
    }

    QString Units::formatSignalKValue(const QString &path,
                                      const double value,
                                      const QString &fallbackSourceUnit,
                                      const QString &fallbackTargetUnit) {
        const auto info = displayUnitsForPath(path);
        if (info.valid && !info.formula.isEmpty() && isNumericDisplayFormat(info.displayFormat)) {
            bool ok = false;
            const double converted = evaluateFormula(info.formula, value, &ok);
            if (ok) {
                return formatNumericValue(converted, info.displayFormat);
            }
        }

        const QString normalizedSourceUnit = normalizeSourceUnit(fallbackSourceUnit);
        const double converted = convert(normalizedSourceUnit, fallbackTargetUnit, value);
        return format(fallbackTargetUnit, converted);
    }

    QString Units::getSignalKUnitLabel(const QString &path, const QString &fallbackUnit) {
        const auto info = displayUnitsForPath(path);
        if (info.valid) {
            if (!info.symbol.isEmpty()) {
                return info.symbol;
            }
            if (!info.targetUnit.isEmpty()) {
                return getLabel(info.targetUnit);
            }
        }
        return getLabel(fallbackUnit);
    }

    nlohmann::json &Units::getUnits() {
        return m_units;
    }
}
