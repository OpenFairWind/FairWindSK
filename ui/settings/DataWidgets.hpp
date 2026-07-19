#ifndef FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP
#define FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP

#include <QWidget>

#include "Settings.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;
class QWidget;

namespace fairwindsk::ui::widgets {
    class TouchCheckBox;
    class TouchComboBox;
    class TouchSpinBox;
}

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
        QWidget *createColorControl(QWidget *parent,
                                    QPushButton **button,
                                    QToolButton **clearButton,
                                    const QString &tooltip);
        void chooseColor(QPushButton *button, const QString &title);
        void clearColor(QPushButton *button);
        QString colorButtonValue(const QPushButton *button) const;
        void setColorButtonValue(QPushButton *button, const QString &color);
        void updateColorButtons();

        Settings *m_settings = nullptr;
        bool m_populating = false;
        QListWidget *m_listWidget = nullptr;
        QLabel *m_iconPreview = nullptr;
        QLabel *m_idValueLabel = nullptr;
        QLineEdit *m_nameEdit = nullptr;
        QLineEdit *m_iconEdit = nullptr;
        QToolButton *m_chooseIconButton = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_typeComboBox = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_visualizationComboBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showIconCheckBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_showTextCheckBox = nullptr;
        QLineEdit *m_signalKPathEdit = nullptr;
        QLineEdit *m_sourceUnitEdit = nullptr;
        QLineEdit *m_defaultUnitEdit = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_updatePolicyComboBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_periodSpinBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_minPeriodSpinBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_valueTextSizeSpinBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_labelTextSizeSpinBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_trendTextSizeSpinBox = nullptr;
        QPushButton *m_valueTextColorButton = nullptr;
        QPushButton *m_labelTextColorButton = nullptr;
        QPushButton *m_trendIncreasingColorButton = nullptr;
        QPushButton *m_trendDecreasingColorButton = nullptr;
        QToolButton *m_clearValueTextColorButton = nullptr;
        QToolButton *m_clearLabelTextColorButton = nullptr;
        QToolButton *m_clearTrendIncreasingColorButton = nullptr;
        QToolButton *m_clearTrendDecreasingColorButton = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_minimumSpinBox = nullptr;
        fairwindsk::ui::widgets::TouchSpinBox *m_maximumSpinBox = nullptr;
        QLineEdit *m_dateTimeFormatEdit = nullptr;
        QPushButton *m_addButton = nullptr;
        QPushButton *m_removeButton = nullptr;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_DATAWIDGETS_HPP
