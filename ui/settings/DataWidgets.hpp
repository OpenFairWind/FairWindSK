#ifndef FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP
#define FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP

#include <QWidget>

#include "Settings.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QToolButton;

namespace fairwindsk::ui::settings {

    class DataWidgets final : public QWidget {
        Q_OBJECT

    public:
        explicit DataWidgets(Settings *settings, QWidget *parent = nullptr);
        ~DataWidgets() override = default;

        void refreshFromConfiguration();

    protected:
        void changeEvent(QEvent *event) override;

    private slots:
        void onSelectionChanged();
        void onAddWidget();
        void onRemoveWidget();
        void onChooseIcon();
        void onEditorChanged();

    private:
        void buildUi();
        void applyChrome();
        void populateFromConfiguration(const QString &selectId = QString());
        void setEditorEnabled(bool enabled);
        void setEditorFromDefinition(const fairwindsk::ui::widgets::DataWidgetDefinition &definition);
        fairwindsk::ui::widgets::DataWidgetDefinition editorDefinition() const;
        fairwindsk::ui::widgets::DataWidgetDefinition definitionForId(const QString &id) const;
        QString currentWidgetId() const;
        QListWidgetItem *itemForId(const QString &id) const;
        void persistEditor();
        void updateIconPreview();

        Settings *m_settings = nullptr;
        bool m_populating = false;
        QListWidget *m_listWidget = nullptr;
        QLabel *m_iconPreview = nullptr;
        QLabel *m_idValueLabel = nullptr;
        QLineEdit *m_nameEdit = nullptr;
        QLineEdit *m_iconEdit = nullptr;
        QToolButton *m_chooseIconButton = nullptr;
        QComboBox *m_typeComboBox = nullptr;
        QLineEdit *m_signalKPathEdit = nullptr;
        QLineEdit *m_sourceUnitEdit = nullptr;
        QLineEdit *m_defaultUnitEdit = nullptr;
        QComboBox *m_updatePolicyComboBox = nullptr;
        QSpinBox *m_periodSpinBox = nullptr;
        QSpinBox *m_minPeriodSpinBox = nullptr;
        QDoubleSpinBox *m_minimumSpinBox = nullptr;
        QDoubleSpinBox *m_maximumSpinBox = nullptr;
        QLineEdit *m_dateTimeFormatEdit = nullptr;
        QPushButton *m_addButton = nullptr;
        QPushButton *m_removeButton = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP
