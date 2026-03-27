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
        const QString kSelectedPresetKey = QStringLiteral("selectedPreset");
        const QString kLocalPresetName = QStringLiteral("local");
    }

    Units::Units(Settings *settings, QWidget *parent)
        : QWidget(parent),
          ui(new Ui::Units),
          m_settings(settings) {
        ui->setupUi(this);

        connect(ui->comboBox_preset, qOverload<int>(&QComboBox::currentIndexChanged),
                this, &Units::onPresetChanged);
        connect(ui->pushButton_saveCustomPreset, &QPushButton::clicked,
                this, &Units::onSaveCustomPresetClicked);
        connect(ui->pushButton_deletePreset, &QPushButton::clicked,
                this, &Units::onDeleteCustomPresetClicked);

        refreshServerData();
        updatePresetSelectionFromCurrentState();
        populatePresetCombo();
        rebuildRows();
    }

    Units::~Units() {
        clearRows();
        delete ui;
    }

    QSet<QString> Units::builtInPresetNames() {
        return {
            QStringLiteral("metric"),
            QStringLiteral("imperial-us"),
            QStringLiteral("imperial-uk"),
            QStringLiteral("nautical-metric"),
            QStringLiteral("nautical-imperial-us"),
            QStringLiteral("nautical-imperial-uk")
        };
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

    QString Units::configuredPresetName() const {
        const auto &root = m_settings->getConfiguration()->getRoot();
        const auto unitPreferencesIt = root.find(kUnitOverrideRoot.toStdString());
        if (unitPreferencesIt == root.end() || !unitPreferencesIt->is_object()) {
            return {};
        }

        const auto selectedPresetIt = unitPreferencesIt->find(kSelectedPresetKey.toStdString());
        if (selectedPresetIt == unitPreferencesIt->end() || !selectedPresetIt->is_string()) {
            return {};
        }

        return QString::fromStdString(selectedPresetIt->get<std::string>());
    }

    void Units::setConfiguredPresetName(const QString &presetName) {
        auto &root = m_settings->getConfiguration()->getRoot();
        root[kUnitOverrideRoot.toStdString()][kSelectedPresetKey.toStdString()] = presetName.toStdString();
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
            setConfiguredPresetName(kLocalPresetName);
        } else {
            clearLocalOverrideForCategory(category);
            auto &root = m_settings->getConfiguration()->getRoot();
            syncLegacyUnitsForCategory(root, category, serverTargetUnit);
            if (!hasLocalOverrides() && !m_serverActivePresetName.isEmpty()) {
                setConfiguredPresetName(m_serverActivePresetName);
            }
        }
    }

    QString Units::selectedPresetName() const {
        if (hasLocalOverrides()) {
            return kLocalPresetName;
        }

        const auto configured = configuredPresetName();
        if (!configured.isEmpty() && m_presets.contains(configured)) {
            return configured;
        }

        if (!m_serverActivePresetName.isEmpty()) {
            return m_serverActivePresetName;
        }

        if (!m_presets.isEmpty()) {
            return m_presets.firstKey();
        }

        return {};
    }

    QString Units::presetUnitForCategory(const QString &category) const {
        const auto presetName = selectedPresetName();
        if (presetName != kLocalPresetName && m_presets.contains(presetName) &&
            m_presets.value(presetName).categories.contains(category)) {
            return m_presets.value(presetName).categories.value(category).targetUnit;
        }

        if (m_presets.contains(m_serverActivePresetName) &&
            m_presets.value(m_serverActivePresetName).categories.contains(category)) {
            return m_presets.value(m_serverActivePresetName).categories.value(category).targetUnit;
        }

        return {};
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

        for (int i = 0; i < item.options.size(); ++i) {
            const auto &option = item.options.at(i);
            if (!targetUnit.isEmpty() &&
                (option.symbol.compare(targetUnit, Qt::CaseInsensitive) == 0 ||
                 option.label.startsWith(targetUnit + QStringLiteral(" "), Qt::CaseInsensitive) ||
                 option.label == targetUnit)) {
                return i;
            }
            if (!symbol.isEmpty() &&
                (option.symbol.compare(symbol, Qt::CaseInsensitive) == 0 ||
                 option.label.contains(QStringLiteral("(%1)").arg(symbol), Qt::CaseInsensitive))) {
                return i;
            }
        }

        return -1;
    }

    QString Units::currentEffectiveUnitForCategory(const QString &category) const {
        const auto override = localOverrideForCategory(category);
        if (!override.isEmpty()) {
            return override;
        }

        return presetUnitForCategory(category);
    }

    Units::PresetInfo Units::parsePresetInfo(const QString &name, const QJsonObject &presetObject, const bool custom) {
        PresetInfo info;
        info.name = name;
        info.displayName = presetObject.value(QStringLiteral("name")).toString();
        if (info.displayName.isEmpty()) {
            info.displayName = name;
        }
        info.custom = presetObject.value(QStringLiteral("custom")).toBool(custom);

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

        const auto activePresetObject = signalKClient->getUnitPreferencesActive();
        const auto activePresetName = activePresetObject.value(QStringLiteral("id")).toString(
            activePresetObject.value(QStringLiteral("key")).toString()
        );
        if (!activePresetName.isEmpty()) {
            m_serverActivePresetName = activePresetName;
            m_presets.insert(activePresetName, parsePresetInfo(activePresetName, activePresetObject, false));
        }

        QStringList presetNames;
        const auto presetsDocument = signalKClient->getUnitPreferencesPresets();
        if (presetsDocument.isArray()) {
            const auto presetsArray = presetsDocument.array();
            for (const auto &value : presetsArray) {
                if (value.isString()) {
                    presetNames.append(value.toString());
                } else if (value.isObject()) {
                    const auto object = value.toObject();
                    const auto name = object.value(QStringLiteral("id")).toString(
                        object.value(QStringLiteral("name")).toString(
                            object.value(QStringLiteral("key")).toString()
                        )
                    );
                    if (!name.isEmpty()) {
                        presetNames.append(name);
                    }
                }
            }
        } else if (presetsDocument.isObject()) {
            const auto presetsObject = presetsDocument.object();
            for (auto it = presetsObject.begin(); it != presetsObject.end(); ++it) {
                presetNames.append(it.key());
            }
        }

        presetNames.removeDuplicates();
        if (!m_serverActivePresetName.isEmpty() && !presetNames.contains(m_serverActivePresetName)) {
            presetNames.prepend(m_serverActivePresetName);
        }

        const auto builtIns = builtInPresetNames();
        for (const auto &presetName : presetNames) {
            if (m_presets.contains(presetName)) {
                continue;
            }
            const auto presetObject = signalKClient->getUnitPreferencesPreset(presetName);
            if (presetObject.isEmpty()) {
                continue;
            }
            m_presets.insert(presetName, parsePresetInfo(presetName, presetObject, !builtIns.contains(presetName)));
        }
    }

    void Units::populatePresetCombo() {
        m_isUpdatingUi = true;
        ui->comboBox_preset->clear();

        if (hasLocalOverrides()) {
            ui->comboBox_preset->addItem(tr("Local"), kLocalPresetName);
        }

        QStringList presetNames = m_presets.keys();
        std::sort(presetNames.begin(), presetNames.end(), [this](const QString &left, const QString &right) {
            return m_presets.value(left).displayName.localeAwareCompare(m_presets.value(right).displayName) < 0;
        });

        for (const auto &presetName : presetNames) {
            ui->comboBox_preset->addItem(m_presets.value(presetName).displayName, presetName);
        }

        const int currentIndex = ui->comboBox_preset->findData(selectedPresetName());
        ui->comboBox_preset->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
        ui->pushButton_saveCustomPreset->setEnabled(!m_presets.isEmpty());

        const auto currentPreset = selectedPresetName();
        const bool canDeletePreset = currentPreset != kLocalPresetName
            && m_presets.contains(currentPreset)
            && m_presets.value(currentPreset).custom;
        ui->pushButton_deletePreset->setEnabled(canDeletePreset);
        m_isUpdatingUi = false;
    }

    void Units::updatePresetSelectionFromCurrentState() {
        const auto configured = configuredPresetName();
        if (configured == kLocalPresetName || hasLocalOverrides()) {
            setConfiguredPresetName(kLocalPresetName);
            return;
        }

        if (!configured.isEmpty() && m_presets.contains(configured)) {
            return;
        }

        if (!m_serverActivePresetName.isEmpty()) {
            setConfiguredPresetName(m_serverActivePresetName);
        }
    }

    void Units::applyPresetSelection(const QString &presetName) {
        if (presetName.isEmpty()) {
            return;
        }

        if (presetName == kLocalPresetName) {
            setConfiguredPresetName(kLocalPresetName);
            return;
        }

        if (!m_presets.contains(presetName) || !m_presets.contains(m_serverActivePresetName)) {
            return;
        }

        const auto &selectedPreset = m_presets.value(presetName);
        const auto &serverPreset = m_presets.value(m_serverActivePresetName);

        QSet<QString> categories;
        for (auto it = serverPreset.categories.begin(); it != serverPreset.categories.end(); ++it) {
            categories.insert(it.key());
        }
        for (auto it = selectedPreset.categories.begin(); it != selectedPreset.categories.end(); ++it) {
            categories.insert(it.key());
        }

        for (const auto &category : categories) {
            const auto serverUnit = serverPreset.categories.contains(category)
                ? serverPreset.categories.value(category).targetUnit
                : QString();
            const auto selectedUnit = selectedPreset.categories.contains(category)
                ? selectedPreset.categories.value(category).targetUnit
                : serverUnit;

            if (!selectedUnit.isEmpty() && selectedUnit != serverUnit) {
                setLocalOverrideForCategory(category, selectedUnit);
            } else {
                clearLocalOverrideForCategory(category);
                auto &root = m_settings->getConfiguration()->getRoot();
                if (!serverUnit.isEmpty()) {
                    syncLegacyUnitsForCategory(root, category, serverUnit);
                }
            }
        }

        setConfiguredPresetName(presetName);
    }

    QJsonObject Units::buildPresetPayload(const QString &name) const {
        QJsonObject payload;
        payload[QStringLiteral("name")] = name;

        QJsonObject categoriesObject;
        const auto activeItems = fairwindsk::Units::getInstance()->getSignalKUnitPreferenceItems();
        for (const auto &item : activeItems) {
            QJsonObject categoryObject;
            categoryObject[QStringLiteral("category")] = item.category;
            categoryObject[QStringLiteral("baseUnit")] = item.baseUnit;
            categoryObject[QStringLiteral("targetUnit")] = currentEffectiveUnitForCategory(item.category);
            if (!item.displayFormat.isEmpty()) {
                categoryObject[QStringLiteral("displayFormat")] = item.displayFormat;
            }
            if (!item.symbol.isEmpty()) {
                categoryObject[QStringLiteral("symbol")] = item.symbol;
            }
            categoriesObject[item.category] = categoryObject;
        }

        payload[QStringLiteral("categories")] = categoriesObject;
        return payload;
    }

    bool Units::saveCustomPreset(const QString &name) {
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        if (!signalKClient || signalKClient->url().isEmpty()) {
            drawer::warning(this, tr("Preset"), tr("Signal K server is not available."));
            return false;
        }

        const auto existingPreset = signalKClient->getUnitPreferencesPreset(name);
        if (!existingPreset.isEmpty()) {
            drawer::warning(this,
                            tr("Preset"),
                            tr("A preset named \"%1\" already exists on the server. Delete it first before saving a new one.").arg(name));
            return false;
        }

        const auto response = signalKClient->putUnitPreferencesCustomPreset(name, buildPresetPayload(name));
        if (response.isEmpty()) {
            drawer::warning(this, tr("Preset"), tr("Unable to save the custom preset on the Signal K server."));
            return false;
        }

        return true;
    }

    void Units::rebuildRows() {
        clearRows();
        populatePresetCombo();

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        const bool hasConnectedServer = signalKClient && !signalKClient->url().isEmpty();
        const auto activeItems = fairwindsk::Units::getInstance()->getSignalKUnitPreferenceItems();

        ui->label_emptyState->setVisible(activeItems.isEmpty());
        if (activeItems.isEmpty()) {
            ui->label_emptyState->setText(
                hasConnectedServer
                    ? tr("The Signal K server is connected, but it did not return any unit preference categories.")
                    : tr("No server unit preferences are available.")
            );
            return;
        }

        ui->pushButton_saveCustomPreset->setEnabled(hasConnectedServer);

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
            int comboIndex = comboIndexForCategoryUnit(rowWidgets.comboBoxUnit, item, presetTargetUnit, presetItem.symbol);
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
                        if (hasLocalOverrides()) {
                            setConfiguredPresetName(kLocalPresetName);
                        }
                        populatePresetCombo();
                    });

            ui->gridLayout_Items->addWidget(rowWidgets.labelCategory, row, 0);
            ui->gridLayout_Items->addWidget(rowWidgets.comboBoxUnit, row, 1);
            ui->gridLayout_Items->addWidget(rowWidgets.labelServerUnit, row, 2);

            m_rowWidgets.insert(item.category, rowWidgets);
            ++row;
        }

        ui->gridLayout_Items->setRowStretch(row, 1);
    }

    void Units::onPresetChanged(const int) {
        if (m_isUpdatingUi) {
            return;
        }

        const auto presetName = ui->comboBox_preset->currentData().toString();
        applyPresetSelection(presetName);
        rebuildRows();
    }

    void Units::onSaveCustomPresetClicked() {
        bool ok = false;
        const auto presetName = drawer::getText(this,
                                                tr("Save custom preset"),
                                                tr("Preset name"),
                                                QString(),
                                                &ok);
        if (!ok || presetName.trimmed().isEmpty()) {
            return;
        }

        if (!saveCustomPreset(presetName.trimmed())) {
            return;
        }

        refreshServerData();
        setConfiguredPresetName(presetName.trimmed());
        rebuildRows();
    }

    void Units::onDeleteCustomPresetClicked() {
        const auto presetName = selectedPresetName();
        if (presetName.isEmpty() || presetName == kLocalPresetName || !m_presets.contains(presetName) || !m_presets.value(presetName).custom) {
            return;
        }

        const auto answer = drawer::question(this,
                                             tr("Delete custom preset"),
                                             tr("Delete preset \"%1\" from the Signal K server?").arg(m_presets.value(presetName).displayName),
                                             QMessageBox::Yes | QMessageBox::Cancel,
                                             QMessageBox::Cancel);
        if (answer != QMessageBox::Yes) {
            return;
        }

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        if (!signalKClient || signalKClient->url().isEmpty()) {
            return;
        }

        signalKClient->deleteUnitPreferencesCustomPreset(presetName);
        setConfiguredPresetName(kLocalPresetName);
        refreshServerData();
        rebuildRows();
    }
}
