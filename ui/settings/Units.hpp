//
// Created by Codex on 27/03/26.
//

#ifndef FAIRWINDSK_SETTINGS_UNITS_HPP
#define FAIRWINDSK_SETTINGS_UNITS_HPP

#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <QWidget>

#include "Settings.hpp"
#include "../../Units.hpp"
#include "../../signalk/Client.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Units; }
    QT_END_NAMESPACE

    class Units : public QWidget {
        Q_OBJECT

    public:
        explicit Units(Settings *settings, QWidget *parent = nullptr);
        ~Units() override;

    private:
        struct PresetInfo {
            QString name;
            QString displayName;
            QMap<QString, fairwindsk::Units::UnitPreferenceItem> categories;
        };

        struct UnitRowWidgets {
            QLabel *labelCategory = nullptr;
            QComboBox *comboBoxUnit = nullptr;
            QLabel *labelServerUnit = nullptr;
        };

        void refreshServerData();
        void rebuildRows();
        void clearRows();
        void applyLocalOverride(const QString &category, const QString &serverTargetUnit, const QString &targetUnit);
        int comboIndexForCategoryUnit(QComboBox *comboBox,
                                      const fairwindsk::Units::UnitPreferenceItem &item,
                                      const QString &targetUnit,
                                      const QString &symbol) const;
        bool hasLocalOverrides() const;
        static PresetInfo parsePresetInfo(const QString &name, const QJsonObject &presetObject);
        QString localOverrideForCategory(const QString &category) const;
        void setLocalOverrideForCategory(const QString &category, const QString &targetUnit);
        void clearLocalOverrideForCategory(const QString &category);
        static QString displayLabelForCategory(const QString &category);
        static void syncLegacyUnitsForCategory(nlohmann::json &root, const QString &category, const QString &targetUnit);

        Ui::Units *ui;
        Settings *m_settings = nullptr;
        QMap<QString, UnitRowWidgets> m_rowWidgets;
        QMap<QString, PresetInfo> m_presets;
        QString m_serverActivePresetName;
        bool m_isUpdatingUi = false;
    };
}

#endif // FAIRWINDSK_SETTINGS_UNITS_HPP
