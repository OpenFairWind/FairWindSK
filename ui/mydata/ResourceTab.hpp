//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP

#include <QModelIndex>
#include <QJsonObject>
#include <QWidget>

#include "ResourceModel.hpp"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;
class QToolButton;
namespace Ui { class ResourceTab; }

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget;
    class JsonObjectEditorWidget;

    class ResourceTab final : public QWidget {
        Q_OBJECT

    public:
        explicit ResourceTab(ResourceKind kind, QWidget *parent = nullptr);
        ~ResourceTab() override;

    private slots:
        void onAddClicked();
        void onOpenClicked();
        void onEditClicked();
        void onDeleteClicked();
        void onImportClicked();
        void onExportClicked();
        void onRefreshClicked();
        void onSearchTextChanged(const QString &text);
        void onTableDoubleClicked(int row, int column);
        void onNavigateRowClicked();
        void onEditRowClicked();
        void onRemoveRowClicked();
        void onBackClicked();
        void onSaveClicked();
        void onCancelClicked();

    private:
        QString currentResourceIdFromSelection() const;
        void rebuildTable();
        void styleTable();
        bool canNavigateResource() const;
        void selectResource(const QString &id);
        void showError(const QString &message) const;
        void showListPage();
        void showDetailsPage(const QString &id, const QJsonObject &resource, bool editMode);
        void setEditMode(bool editMode);
        void setButtonsForReadOnly();
        void setButtonsForEdit();
        void populateEditor(const QString &id, const QJsonObject &resource);
        QJsonObject resourceFromEditor() const;
        bool validateEditor(QString *message) const;
        QJsonObject parseJsonObject(const QPlainTextEdit *edit) const;
        QJsonArray coordinatesJson() const;
        void clearEditor();
        void updatePreview(const QJsonObject &resource);
        QString resourceDisplayName(const QJsonObject &resource) const;
        QString resourceDescription(const QJsonObject &resource) const;
        QString detailsTitleForCurrentState() const;

        ::Ui::ResourceTab *ui = nullptr;
        ResourceKind m_kind;
        ResourceModel *m_model;
        QStackedWidget *m_stackedWidget;
        QWidget *m_listPage;
        QWidget *m_detailsPage;
        QLineEdit *m_searchEdit;
        QTableWidget *m_tableWidget;
        QToolButton *m_addButton;
        QToolButton *m_importButton;
        QToolButton *m_exportButton;
        QToolButton *m_refreshButton;
        QToolButton *m_backButton;
        QToolButton *m_newButton;
        QToolButton *m_editButton;
        QToolButton *m_saveButton;
        QToolButton *m_cancelButton;
        QToolButton *m_deleteButton;
        QLabel *m_titleLabel;
        QLabel *m_idValueLabel;
        QLabel *m_timestampValueLabel;
        QLineEdit *m_nameEdit;
        QLineEdit *m_descriptionEdit;
        QLineEdit *m_typeEdit;
        QDoubleSpinBox *m_latitudeSpinBox;
        QDoubleSpinBox *m_longitudeSpinBox;
        QDoubleSpinBox *m_altitudeSpinBox;
        QPlainTextEdit *m_coordinatesEdit;
        QPlainTextEdit *m_geometryEdit;
        JsonObjectEditorWidget *m_propertiesEditor;
        GeoJsonPreviewWidget *m_previewWidget;
        QLineEdit *m_hrefEdit;
        QLineEdit *m_mimeTypeEdit;
        QCheckBox *m_notePositionCheckBox;
        QLineEdit *m_identifierEdit;
        QLineEdit *m_chartFormatEdit;
        QLineEdit *m_chartUrlEdit;
        QLineEdit *m_tilemapUrlEdit;
        QLineEdit *m_chartRegionEdit;
        QDoubleSpinBox *m_chartScaleSpinBox;
        QLineEdit *m_chartLayersEdit;
        QPlainTextEdit *m_chartBoundsEdit;
        QString m_currentResourceId;
        QStringList m_visibleResourceIds;
        bool m_isEditing = false;
        bool m_isCreating = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP
