//
// Created by Codex on 27/03/26.
//

#ifndef FAIRWINDSK_SETTINGS_UNITS_HPP
#define FAIRWINDSK_SETTINGS_UNITS_HPP

#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <QWidget>

#include "Settings.hpp"
#include "../../Units.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Units; }
    QT_END_NAMESPACE

    class Units : public QWidget {
        Q_OBJECT

    public:
        explicit Units(Settings *settings, QWidget *parent = nullptr);
        ~Units() override;

    private slots:
        void onSyncFromServerClicked();

    private:
        struct UnitRowWidgets {
            QLabel *labelCategory = nullptr;
            QComboBox *comboBoxUnit = nullptr;
            QLabel *labelServerUnit = nullptr;
        };

        void rebuildRows();
        void clearRows();
        void applyLocalOverride(const QString &category, const QString &serverTargetUnit, const QString &targetUnit);
        QString localOverrideForCategory(const QString &category) const;
        void setLocalOverrideForCategory(const QString &category, const QString &targetUnit);
        void clearLocalOverrideForCategory(const QString &category);
        void syncLocalOverridesFromServer();
        static QString displayLabelForCategory(const QString &category);
        static void syncLegacyUnitsForCategory(nlohmann::json &root, const QString &category, const QString &targetUnit);

        Ui::Units *ui;
        Settings *m_settings = nullptr;
        QMap<QString, UnitRowWidgets> m_rowWidgets;
    };
}

#endif // FAIRWINDSK_SETTINGS_UNITS_HPP
