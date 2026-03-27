//
// Created by Codex on 27/03/26.
//

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

#include "FairWindSK.hpp"
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

        connect(ui->pushButton_syncFromServer, &QPushButton::clicked,
                this, &Units::onSyncFromServerClicked);

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

    void Units::applyLocalOverride(const QString &category, const bool enabled, const QString &targetUnit) {
        if (enabled) {
            setLocalOverrideForCategory(category, targetUnit);
        } else {
            clearLocalOverrideForCategory(category);
        }
    }

    void Units::syncLocalOverridesFromServer() {
        const auto items = fairwindsk::Units::getInstance()->getSignalKUnitPreferenceItems();
        for (const auto &item : items) {
            setLocalOverrideForCategory(item.category, item.targetUnit);
        }
    }

    void Units::rebuildRows() {
        clearRows();

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        const bool hasConnectedServer = signalKClient && !signalKClient->server().isEmpty();
        if (hasConnectedServer) {
            fairwindsk::Units::getInstance()->refreshSignalKPreferences();
        }

        const auto unitItems = fairwindsk::Units::getInstance()->getSignalKUnitPreferenceItems();
        const auto presetName = fairwindsk::Units::getInstance()->getSignalKActivePresetName();

        ui->label_presetValue->setText(
            presetName.isEmpty() ? tr("Unavailable") : presetName
        );
        ui->pushButton_syncFromServer->setEnabled(hasConnectedServer && !unitItems.isEmpty());
        ui->label_emptyState->setVisible(unitItems.isEmpty());

        if (unitItems.isEmpty()) {
            if (hasConnectedServer) {
                ui->label_emptyState->setText(tr("The Signal K server is connected, but it did not return any unit preference categories."));
            } else {
                ui->label_emptyState->setText(tr("No server unit preferences are available. Connect to a Signal K server and reopen Settings to manage local unit overrides."));
            }
            return;
        }

        int row = 0;
        for (const auto &item : unitItems) {
            UnitRowWidgets rowWidgets;
            rowWidgets.labelCategory = new QLabel(displayLabelForCategory(item.category), this);
            rowWidgets.checkBoxOverride = new QCheckBox(tr("Override"), this);
            rowWidgets.comboBoxUnit = new QComboBox(this);
            rowWidgets.labelServerUnit = new QLabel(this);

            for (const auto &option : item.options) {
                rowWidgets.comboBoxUnit->addItem(option.label, option.key);
            }

            const auto localOverride = localOverrideForCategory(item.category);
            const bool overrideEnabled = !localOverride.isEmpty();
            const QString effectiveTargetUnit = overrideEnabled ? localOverride : item.targetUnit;
            const int comboIndex = rowWidgets.comboBoxUnit->findData(effectiveTargetUnit);
            rowWidgets.comboBoxUnit->setCurrentIndex(comboIndex >= 0 ? comboIndex : 0);
            rowWidgets.comboBoxUnit->setEnabled(overrideEnabled);

            rowWidgets.checkBoxOverride->setChecked(overrideEnabled);
            rowWidgets.labelServerUnit->setText(tr("Server: %1").arg(item.symbol.isEmpty() ? item.targetUnit : item.symbol));

            connect(rowWidgets.checkBoxOverride, &QCheckBox::toggled, this,
                    [this, category = item.category, comboBox = rowWidgets.comboBoxUnit](bool checked) {
                        comboBox->setEnabled(checked);
                        applyLocalOverride(category, checked, comboBox->currentData().toString());
                    });

            connect(rowWidgets.comboBoxUnit, &QComboBox::currentIndexChanged, this,
                    [this, category = item.category, checkBox = rowWidgets.checkBoxOverride, comboBox = rowWidgets.comboBoxUnit](int) {
                        if (checkBox->isChecked()) {
                            applyLocalOverride(category, true, comboBox->currentData().toString());
                        }
                    });

            ui->gridLayout_Items->addWidget(rowWidgets.labelCategory, row, 0);
            ui->gridLayout_Items->addWidget(rowWidgets.checkBoxOverride, row, 1);
            ui->gridLayout_Items->addWidget(rowWidgets.comboBoxUnit, row, 2);
            ui->gridLayout_Items->addWidget(rowWidgets.labelServerUnit, row, 3);

            m_rowWidgets.insert(item.category, rowWidgets);
            ++row;
        }

        ui->gridLayout_Items->setRowStretch(row, 1);
    }

    void Units::onSyncFromServerClicked() {
        syncLocalOverridesFromServer();
        rebuildRows();
    }
}
