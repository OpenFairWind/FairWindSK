//
// Created by Codex on 27/03/26.
//

#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>

#include "FairWindSK.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "Units.hpp"
#include "ui_Units.h"

namespace fairwindsk::ui::settings {
    namespace {
        const QString kUnitOverrideRoot = QStringLiteral("unitPreferences");
        const QString kUnitOverrideNode = QStringLiteral("overrides");
    }

    Units::Units(Settings *settings, QWidget *parent)
        : QWidget(parent),
          ui(new Ui::Units),
          m_settings(settings) {
        ui->setupUi(this);

        refreshServerData();
        rebuildRows();
    }

    Units::~Units() {
        clearRows();
        delete ui;
    }

    void Units::clearRows() {
        while (ui->gridLayout_Items->count() > 0) {
            auto *item = ui->gridLayout_Items->takeAt(0);
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        m_rowWidgets.clear();
    }

    QString Units::displayLabelForCategory(const QString &category) {
        QString label = category;
        label.replace('_', ' ');
        for (int i = 0; i < label.size(); ++i) {
            const bool capitalize = i == 0 || label.at(i - 1).isSpace();
            if (capitalize) {
                label[i] = label.at(i).toUpper();
            }
        }
        if (label == QStringLiteral("Datetime")) {
            return QObject::tr("Date/Time");
        }
        return label;
    }

    void Units::syncLegacyUnitsForCategory(nlohmann::json &root, const QString &category, const QString &targetUnit) {
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

    QString Units::localOverrideForCategory(const QString &category) const {
        const auto &root = m_settings->getConfiguration()->getRoot();
        const auto unitPreferencesIt = root.find(kUnitOverrideRoot.toStdString());
        if (unitPreferencesIt == root.end() || !unitPreferencesIt->is_object()) {
            return {};
        }

        const auto overridesIt = unitPreferencesIt->find(kUnitOverrideNode.toStdString());
        if (overridesIt == unitPreferencesIt->end() || !overridesIt->is_object()) {
            return {};
        }

        const auto valueIt = overridesIt->find(category.toStdString());
        if (valueIt == overridesIt->end() || !valueIt->is_string()) {
            return {};
        }

        return QString::fromStdString(valueIt->get<std::string>());
    }

    bool Units::hasLocalOverrides() const {
        const auto &root = m_settings->getConfiguration()->getRoot();
        const auto unitPreferencesIt = root.find(kUnitOverrideRoot.toStdString());
        if (unitPreferencesIt == root.end() || !unitPreferencesIt->is_object()) {
            return false;
        }

        const auto overridesIt = unitPreferencesIt->find(kUnitOverrideNode.toStdString());
        return overridesIt != unitPreferencesIt->end() && overridesIt->is_object() && !overridesIt->empty();
    }

    void Units::setLocalOverrideForCategory(const QString &category, const QString &targetUnit) {
        auto &root = m_settings->getConfiguration()->getRoot();
        root[kUnitOverrideRoot.toStdString()][kUnitOverrideNode.toStdString()][category.toStdString()] = targetUnit.toStdString();
        syncLegacyUnitsForCategory(root, category, targetUnit);
    }

    void Units::clearLocalOverrideForCategory(const QString &category) {
        auto &root = m_settings->getConfiguration()->getRoot();
        const auto unitPreferencesKey = kUnitOverrideRoot.toStdString();
        const auto overridesKey = kUnitOverrideNode.toStdString();
        if (root.contains(unitPreferencesKey) && root[unitPreferencesKey].is_object()) {
            auto &unitPreferences = root[unitPreferencesKey];
            if (unitPreferences.contains(overridesKey) && unitPreferences[overridesKey].is_object()) {
                unitPreferences[overridesKey].erase(category.toStdString());
            }
        }
    }

    void Units::applyLocalOverride(const QString &category, const QString &serverTargetUnit, const QString &targetUnit) {
        if (targetUnit != serverTargetUnit) {
            setLocalOverrideForCategory(category, targetUnit);
        } else {
            clearLocalOverrideForCategory(category);
            auto &root = m_settings->getConfiguration()->getRoot();
            syncLegacyUnitsForCategory(root, category, serverTargetUnit);
        }
        if (m_settings) {
            m_settings->markDirty(FairWindSK::RuntimeUnits, 0);
        }
    }

    QString Units::canonicalUnitToken(const QString &value) {
        const QString normalized = value.trimmed().toLower();
        if (normalized == QStringLiteral("m")) {
            return QStringLiteral("meter");
        }
        if (normalized == QStringLiteral("km")) {
            return QStringLiteral("kilometer");
        }
        if (normalized == QStringLiteral("nmi") || normalized == QStringLiteral("nm")) {
            return QStringLiteral("naut-mile");
        }
        if (normalized == QStringLiteral("ft")) {
            return QStringLiteral("foot");
        }
        if (normalized == QStringLiteral("°") || normalized == QStringLiteral("deg")) {
            return QStringLiteral("degree");
        }
        if (normalized == QStringLiteral("rad")) {
            return QStringLiteral("radian");
        }
        return normalized;
    }

    int Units::comboIndexForCategoryUnit(QComboBox *comboBox,
                                         const fairwindsk::Units::UnitPreferenceItem &item,
                                         const QString &targetUnit,
                                         const QString &symbol) const {
        if (!comboBox) {
            return -1;
        }

        if (!targetUnit.isEmpty()) {
            const int exactIndex = comboBox->findData(targetUnit);
            if (exactIndex >= 0) {
                return exactIndex;
            }
        }

        const QString canonicalTargetUnit = canonicalUnitToken(targetUnit);
        const QString canonicalSymbol = canonicalUnitToken(symbol);

        for (int i = 0; i < item.options.size(); ++i) {
            const auto &option = item.options.at(i);
            const QString optionKey = canonicalUnitToken(option.key);
            const QString optionSymbol = canonicalUnitToken(option.symbol);
            if (!targetUnit.isEmpty() &&
                (optionKey == canonicalTargetUnit ||
                 optionSymbol == canonicalTargetUnit ||
                 option.symbol.compare(targetUnit, Qt::CaseInsensitive) == 0 ||
                 option.label.startsWith(targetUnit + QStringLiteral(" "), Qt::CaseInsensitive) ||
                 option.label == targetUnit)) {
                return i;
            }
            if (!symbol.isEmpty() &&
                (optionKey == canonicalSymbol ||
                 optionSymbol == canonicalSymbol ||
                 option.symbol.compare(symbol, Qt::CaseInsensitive) == 0 ||
                 option.label.contains(QStringLiteral("(%1)").arg(symbol), Qt::CaseInsensitive))) {
                return i;
            }
        }

        return -1;
    }

    Units::PresetInfo Units::parsePresetInfo(const QString &name, const QJsonObject &presetObject) {
        PresetInfo info;
        info.name = name;
        info.displayName = presetObject.value(QStringLiteral("name")).toString();
        if (info.displayName.isEmpty()) {
            info.displayName = name;
        }

        const auto categoriesValue = presetObject.value(QStringLiteral("categories"));
        if (categoriesValue.isObject()) {
            const auto categories = categoriesValue.toObject();
            for (auto categoryIt = categories.begin(); categoryIt != categories.end(); ++categoryIt) {
                if (!categoryIt.value().isObject()) {
                    continue;
                }
                auto item = fairwindsk::Units::UnitPreferenceItem{};
                const auto categoryObject = categoryIt.value().toObject();
                item.category = categoryIt.key();
                item.baseUnit = categoryObject.value(QStringLiteral("baseUnit")).toString();
                item.targetUnit = categoryObject.value(QStringLiteral("targetUnit")).toString();
                item.displayFormat = categoryObject.value(QStringLiteral("displayFormat")).toString();
                item.symbol = categoryObject.value(QStringLiteral("symbol")).toString();
                info.categories.insert(item.category, item);
            }
        }

        return info;
    }

    void Units::refreshServerData() {
        m_presets.clear();
        m_serverActivePresetName.clear();

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        if (!signalKClient || signalKClient->url().isEmpty()) {
            return;
        }

        fairwindsk::Units::getInstance()->refreshSignalKPreferences();

        const auto configObject = signalKClient->getUnitPreferencesConfig();
        if (configObject.contains(QStringLiteral("activePreset")) && configObject.value(QStringLiteral("activePreset")).isString()) {
            m_serverActivePresetName = configObject.value(QStringLiteral("activePreset")).toString();
        }

        const auto activePresetObject = signalKClient->getUnitPreferencesActive();
        QString activePresetKey = activePresetObject.value(QStringLiteral("id")).toString(
            activePresetObject.value(QStringLiteral("key")).toString()
        );
        if (activePresetKey.isEmpty()) {
            activePresetKey = m_serverActivePresetName;
        }
        if (!activePresetKey.isEmpty()) {
            m_presets.insert(activePresetKey, parsePresetInfo(activePresetKey, activePresetObject));
        }

        const auto presetsDocument = signalKClient->getUnitPreferencesPresets();
        if (presetsDocument.isObject()) {
            const auto presetsObject = presetsDocument.object();
            const QStringList groups{QStringLiteral("builtIn"), QStringLiteral("custom")};
            for (const auto &groupName : groups) {
                if (!presetsObject.contains(groupName) || !presetsObject.value(groupName).isArray()) {
                    continue;
                }
                const auto presetArray = presetsObject.value(groupName).toArray();
                for (const auto &presetValue : presetArray) {
                    if (!presetValue.isObject()) {
                        continue;
                    }
                    const auto presetObject = presetValue.toObject();
                    const QString presetName = presetObject.value(QStringLiteral("name")).toString();
                    if (presetName.isEmpty()) {
                        continue;
                    }

                    auto info = m_presets.value(presetName);
                    info.name = presetName;
                    info.displayName = presetObject.value(QStringLiteral("displayName")).toString(presetObject.value(QStringLiteral("name")).toString());
                    m_presets.insert(presetName, info);
                }
            }
        }
    }

    void Units::rebuildRows() {
        clearRows();

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        const bool hasConnectedServer = signalKClient && !signalKClient->url().isEmpty();
        const auto activeItems = fairwindsk::Units::getInstance()->getSignalKUnitPreferenceItems();
        ui->label_currentPresetValue->setText(m_presets.contains(m_serverActivePresetName)
                                                  ? m_presets.value(m_serverActivePresetName).displayName
                                                  : tr("Unavailable"));

        ui->label_emptyState->setVisible(activeItems.isEmpty());
        if (activeItems.isEmpty()) {
            ui->label_emptyState->setText(
                hasConnectedServer
                    ? tr("The Signal K server is connected, but it did not return any unit preference categories.")
                    : tr("No server unit preferences are available.")
            );
            return;
        }

        const QMap<QString, fairwindsk::Units::UnitPreferenceItem> serverPresetItems =
            m_presets.contains(m_serverActivePresetName) ? m_presets.value(m_serverActivePresetName).categories : QMap<QString, fairwindsk::Units::UnitPreferenceItem>{};

        int row = 0;
        for (const auto &item : activeItems) {
            UnitRowWidgets rowWidgets;
            rowWidgets.labelCategory = new QLabel(displayLabelForCategory(item.category), this);
            rowWidgets.comboBoxUnit = new QComboBox(this);
            rowWidgets.labelServerUnit = new QLabel(this);

            for (const auto &option : item.options) {
                rowWidgets.comboBoxUnit->addItem(option.label, option.key);
            }

            const auto presetItem = serverPresetItems.contains(item.category)
                ? serverPresetItems.value(item.category)
                : fairwindsk::Units::UnitPreferenceItem{};
            const QString presetTargetUnit = presetItem.targetUnit.isEmpty()
                ? item.targetUnit
                : presetItem.targetUnit;
            const QString localOverrideTargetUnit = localOverrideForCategory(item.category);
            int comboIndex = -1;
            if (!localOverrideTargetUnit.isEmpty()) {
                comboIndex = comboIndexForCategoryUnit(rowWidgets.comboBoxUnit, item, localOverrideTargetUnit, QString());
            }
            if (comboIndex < 0) {
                comboIndex = comboIndexForCategoryUnit(rowWidgets.comboBoxUnit, item, presetTargetUnit, presetItem.symbol);
            }
            if (comboIndex < 0) {
                comboIndex = comboIndexForCategoryUnit(rowWidgets.comboBoxUnit, item, item.targetUnit, item.symbol);
            }
            rowWidgets.comboBoxUnit->setCurrentIndex(comboIndex >= 0 ? comboIndex : 0);

            QString serverUnitText = item.symbol.isEmpty() ? item.targetUnit : item.symbol;
            if (serverPresetItems.contains(item.category)) {
                const auto &serverItem = serverPresetItems.value(item.category);
                serverUnitText = serverItem.symbol.isEmpty() ? serverItem.targetUnit : serverItem.symbol;
            }
            rowWidgets.labelServerUnit->setText(serverUnitText);

            connect(rowWidgets.comboBoxUnit, &QComboBox::currentIndexChanged, this,
                    [this, category = item.category, serverTargetUnit = presetTargetUnit.isEmpty() ? item.targetUnit : presetTargetUnit, comboBox = rowWidgets.comboBoxUnit](int) {
                        if (m_isUpdatingUi) {
                            return;
                        }
                        applyLocalOverride(category, serverTargetUnit, comboBox->currentData().toString());
                    });

            ui->gridLayout_Items->addWidget(rowWidgets.labelCategory, row, 0);
            ui->gridLayout_Items->addWidget(rowWidgets.comboBoxUnit, row, 1);
            ui->gridLayout_Items->addWidget(rowWidgets.labelServerUnit, row, 2);

            m_rowWidgets.insert(item.category, rowWidgets);
            ++row;
        }

        ui->gridLayout_Items->setRowStretch(row, 1);
    }
}
