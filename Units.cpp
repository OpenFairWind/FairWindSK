//
// Created by Raffaele Montella on 02/06/21.
//

#include <Units.hpp>
#include <algorithm>
#include <fstream>
#include <QFile>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include "FairWindSK.hpp"

namespace fairwindsk {
    namespace {
        const QString kUnitOverrideRoot = QStringLiteral("unitPreferences");
        const QString kUnitOverrideNode = QStringLiteral("overrides");

        QString localOverrideKeyForCategory(const QString &category) {
            return category;
        }

        QString displayLabelForCategory(const QString &category) {
            QString label = category;
            label.replace('_', ' ');
            for (int i = 0; i < label.size(); ++i) {
                const bool capitalize = i == 0 || label.at(i - 1).isSpace();
                if (capitalize) {
                    label[i] = label.at(i).toUpper();
                }
            }
            if (label == QStringLiteral("Datetime")) {
                return QStringLiteral("Date/Time");
            }
            return label;
        }

        void syncLegacyUnitsForCategory(nlohmann::json &root, const QString &category, const QString &targetUnit) {
            auto &units = root["units"];

            if (category == QStringLiteral("speed")) {
                units["vesselSpeed"] = targetUnit.toStdString();
                units["windSpeed"] = targetUnit.toStdString();
            } else if (category == QStringLiteral("depth")) {
                if (targetUnit == QStringLiteral("m")) {
                    units["depth"] = "mt";
                } else if (targetUnit == QStringLiteral("ft")) {
                    units["depth"] = "ft";
                } else if (targetUnit == QStringLiteral("ftm")) {
                    units["depth"] = "ftm";
                } else {
                    units["depth"] = targetUnit.toStdString();
                }
            } else if (category == QStringLiteral("distance")) {
                if (targetUnit == QStringLiteral("m")) {
                    units["distance"] = "m";
                    units["range"] = "rm";
                } else {
                    units["distance"] = targetUnit.toStdString();
                    units["range"] = targetUnit.toStdString();
                }
            } else if (category == QStringLiteral("temperature")) {
                units["airTemperature"] = targetUnit.toStdString();
                units["waterTemperature"] = targetUnit.toStdString();
            } else if (category == QStringLiteral("pressure")) {
                units["airPressure"] = targetUnit.toStdString();
            }
        }

        void setJsonStringValue(nlohmann::json &jsonObject, const std::string &key, const QString &value) {
            jsonObject[key] = value.toStdString();
        }
    }

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
        m_signalKActivePresetName.clear();
        m_categoryDisplayUnits.clear();
        m_definitionsByBaseUnit.clear();
        m_pathPatternCategories.clear();
        m_displayUnitsCache.clear();
        m_attemptedDisplayUnitsPaths.clear();
        loadSignalKPreferences();
    }

    void Units::loadSignalKPreferences() {
        if (m_signalKPreferencesLoaded) {
            return;
        }

        const auto fairWindSK = FairWindSK::getInstance();
        if (!fairWindSK) {
            return;
        }

        const auto client = fairWindSK->getSignalKClient();
        if (!client || client->url().isEmpty()) {
            return;
        }

        const auto activePreset = client->getUnitPreferencesActive();
        if (activePreset.contains("name") && activePreset["name"].isString()) {
            m_signalKActivePresetName = activePreset["name"].toString();
        }
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

        const auto definitionsObject = client->getUnitPreferencesDefinitions();
        if (definitionsObject.contains("definitions") && definitionsObject["definitions"].isObject()) {
            const auto definitions = definitionsObject["definitions"].toObject();
            for (auto baseUnitIt = definitions.begin(); baseUnitIt != definitions.end(); ++baseUnitIt) {
                if (!baseUnitIt.value().isObject()) {
                    continue;
                }
                const auto definitionObject = baseUnitIt.value().toObject();
                if (!definitionObject.contains("conversions") || !definitionObject["conversions"].isObject()) {
                    continue;
                }

                const auto conversions = definitionObject["conversions"].toObject();
                QMap<QString, DisplayUnitsInfo> conversionsForBaseUnit;
                for (auto conversionIt = conversions.begin(); conversionIt != conversions.end(); ++conversionIt) {
                    if (!conversionIt.value().isObject()) {
                        continue;
                    }
                    auto displayUnits = parseDisplayUnits(conversionIt.value().toObject());
                    displayUnits.baseUnit = baseUnitIt.key();
                    displayUnits.targetUnit = conversionIt.key();
                    if (displayUnits.symbol.isEmpty()) {
                        displayUnits.symbol = conversionIt.key();
                    }
                    if (displayUnits.valid) {
                        conversionsForBaseUnit.insert(conversionIt.key(), displayUnits);
                    }
                }
                if (!conversionsForBaseUnit.isEmpty()) {
                    m_definitionsByBaseUnit.insert(baseUnitIt.key(), conversionsForBaseUnit);
                }
            }
        }

        const auto categoriesObject = client->getUnitPreferencesDefaultCategories();
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

        m_signalKPreferencesLoaded = true;
    }

    Units::DisplayUnitsInfo Units::parseDisplayUnits(const QJsonObject &jsonObject) const {
        DisplayUnitsInfo info;
        info.category = jsonObject.value("category").toString();
        info.baseUnit = jsonObject.value("baseUnit").toString();
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

    Units::DisplayUnitsInfo Units::localOverrideForCategory(const QString &category) const {
        const auto fairWindSK = FairWindSK::getInstance();
        if (!fairWindSK || !fairWindSK->getConfiguration()) {
            return {};
        }

        const auto overrideTargetUnit = getLocalUnitOverride(category);
        if (overrideTargetUnit.isEmpty() || !m_categoryDisplayUnits.contains(category)) {
            return {};
        }

        const auto defaultInfo = m_categoryDisplayUnits.value(category);
        if (defaultInfo.baseUnit.isEmpty() || !m_definitionsByBaseUnit.contains(defaultInfo.baseUnit)) {
            return {};
        }

        const auto conversionsForBaseUnit = m_definitionsByBaseUnit.value(defaultInfo.baseUnit);
        if (!conversionsForBaseUnit.contains(overrideTargetUnit)) {
            return {};
        }

        auto info = conversionsForBaseUnit.value(overrideTargetUnit);
        info.category = category;
        info.baseUnit = defaultInfo.baseUnit;
        if (info.symbol.isEmpty()) {
            info.symbol = overrideTargetUnit;
        }
        info.valid = true;
        return info;
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
            if (client && !client->url().isEmpty()) {
                const auto metaObject = client->getPathMeta(path);
                if (metaObject.contains("displayUnits") && metaObject["displayUnits"].isObject()) {
                    auto info = parseDisplayUnits(metaObject["displayUnits"].toObject());
                    if (info.baseUnit.isEmpty() && metaObject.contains("units") && metaObject["units"].isString()) {
                        info.baseUnit = metaObject["units"].toString();
                    }
                    if (info.category.isEmpty()) {
                        info.category = categoryForPath(path);
                    }
                    if (!info.category.isEmpty()) {
                        const auto overrideInfo = localOverrideForCategory(info.category);
                        if (overrideInfo.valid) {
                            info = overrideInfo;
                        }
                    }
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
                const auto overrideInfo = localOverrideForCategory(category);
                if (overrideInfo.valid) {
                    info = overrideInfo;
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

    QString Units::getSignalKActivePresetName() {
        loadSignalKPreferences();
        return m_signalKActivePresetName;
    }

    QList<Units::UnitPreferenceItem> Units::getSignalKUnitPreferenceItems() {
        loadSignalKPreferences();

        QList<UnitPreferenceItem> items;
        for (auto categoryIt = m_categoryDisplayUnits.constBegin(); categoryIt != m_categoryDisplayUnits.constEnd(); ++categoryIt) {
            const auto &serverInfo = categoryIt.value();
            UnitPreferenceItem item;
            item.category = categoryIt.key();
            item.baseUnit = serverInfo.baseUnit;
            item.displayFormat = serverInfo.displayFormat;
            item.targetUnit = serverInfo.targetUnit;
            item.symbol = serverInfo.symbol;

            if (m_definitionsByBaseUnit.contains(serverInfo.baseUnit)) {
                const auto definitions = m_definitionsByBaseUnit.value(serverInfo.baseUnit);
                for (auto definitionIt = definitions.constBegin(); definitionIt != definitions.constEnd(); ++definitionIt) {
                    const auto &definition = definitionIt.value();
                    UnitOption option;
                    option.key = definitionIt.key();
                    option.symbol = definition.symbol.isEmpty() ? definitionIt.key() : definition.symbol;
                    option.label = QStringLiteral("%1 (%2)")
                            .arg(getLabel(definitionIt.key()), option.symbol);
                    item.options.append(option);
                }
            }

            if (item.options.isEmpty()) {
                UnitOption option;
                option.key = serverInfo.targetUnit;
                option.symbol = serverInfo.symbol;
                option.label = QStringLiteral("%1 (%2)")
                        .arg(getLabel(serverInfo.targetUnit), serverInfo.symbol.isEmpty() ? serverInfo.targetUnit : serverInfo.symbol);
                item.options.append(option);
            }

            items.append(item);
        }

        std::sort(items.begin(), items.end(), [](const UnitPreferenceItem &left, const UnitPreferenceItem &right) {
            return displayLabelForCategory(left.category).localeAwareCompare(displayLabelForCategory(right.category)) < 0;
        });

        return items;
    }

    QString Units::getLocalUnitOverride(const QString &category) const {
        const auto fairWindSK = FairWindSK::getInstance();
        const auto configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        if (!configuration) {
            return {};
        }

        const auto root = configuration->getRoot();
        const auto unitPreferencesIt = root.find(kUnitOverrideRoot.toStdString());
        if (unitPreferencesIt == root.end() || !unitPreferencesIt->is_object()) {
            return {};
        }

        const auto overridesIt = unitPreferencesIt->find(kUnitOverrideNode.toStdString());
        if (overridesIt == unitPreferencesIt->end() || !overridesIt->is_object()) {
            return {};
        }

        const auto key = localOverrideKeyForCategory(category).toStdString();
        const auto valueIt = overridesIt->find(key);
        if (valueIt == overridesIt->end() || !valueIt->is_string()) {
            return {};
        }

        return QString::fromStdString(valueIt->get<std::string>());
    }

    void Units::setLocalUnitOverride(const QString &category, const QString &targetUnit) {
        const auto fairWindSK = FairWindSK::getInstance();
        const auto configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        if (!configuration || category.isEmpty() || targetUnit.isEmpty()) {
            return;
        }

        auto &root = configuration->getRoot();
        auto &overrides = root[kUnitOverrideRoot.toStdString()][kUnitOverrideNode.toStdString()];
        setJsonStringValue(overrides, localOverrideKeyForCategory(category).toStdString(), targetUnit);
        syncLegacyUnitsForCategory(root, category, targetUnit);
        m_displayUnitsCache.clear();
    }

    void Units::clearLocalUnitOverride(const QString &category) {
        const auto fairWindSK = FairWindSK::getInstance();
        const auto configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        if (!configuration || category.isEmpty()) {
            return;
        }

        auto &root = configuration->getRoot();
        const auto unitPreferencesKey = kUnitOverrideRoot.toStdString();
        const auto overridesKey = kUnitOverrideNode.toStdString();
        if (root.contains(unitPreferencesKey) && root[unitPreferencesKey].is_object()) {
            auto &unitPreferences = root[unitPreferencesKey];
            if (unitPreferences.contains(overridesKey) && unitPreferences[overridesKey].is_object()) {
                unitPreferences[overridesKey].erase(localOverrideKeyForCategory(category).toStdString());
            }
        }
        if (m_categoryDisplayUnits.contains(category)) {
            syncLegacyUnitsForCategory(root, category, m_categoryDisplayUnits.value(category).targetUnit);
        }
        m_displayUnitsCache.clear();
    }

    void Units::syncLocalUnitsFromServer() {
        loadSignalKPreferences();

        for (auto categoryIt = m_categoryDisplayUnits.constBegin(); categoryIt != m_categoryDisplayUnits.constEnd(); ++categoryIt) {
            setLocalUnitOverride(categoryIt.key(), categoryIt.value().targetUnit);
        }
    }

    nlohmann::json &Units::getUnits() {
        return m_units;
    }
}
