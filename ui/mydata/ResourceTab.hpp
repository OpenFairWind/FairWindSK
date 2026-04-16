//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCETAB_HPP

#include <QModelIndex>
#include <QJsonObject>
#include <QIcon>
#include <QWidget>

#include "ResourceModel.hpp"

class QCheckBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QToolButton;
namespace Ui { class ResourceTab; }

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget;
    class JsonObjectEditorWidget;

    class ResourceTab : public QWidget {
        Q_OBJECT

    public:
        ~ResourceTab() override;

    protected:
        explicit ResourceTab(ResourceKind kind, QWidget *parent = nullptr);
        void changeEvent(QEvent *event) override;

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
        void onCoordinateEditClicked();
        void onPrimaryRowClicked();
        void onNavigateRowClicked();
        void onEditRowClicked();
        void onRemoveRowClicked();
        void onBackClicked();
        void onSaveClicked();
        void onCancelClicked();

    private:
        void retintToolButtons() const;
        QString currentResourceIdFromSelection() const;
        void rebuildTable();
        void styleTable();
        void configureTableColumns();
        bool canNavigateResource() const;
        void selectResource(const QString &id);
        void showError(const QString &message) const;
        void showListPage();
        void showDetailsPage(const QString &id, const QJsonObject &resource, bool editMode);
        void setEditMode(bool editMode);
        void setButtonsForReadOnly();
        void setButtonsForEdit();
        void populateEditor(const QString &id, const QJsonObject &resource);
        QJsonObject resourceFromEditor(const QString &idOverride = QString()) const;
        bool validateEditor(QString *message) const;
        QJsonObject parseJsonObject(const QPlainTextEdit *edit) const;
        QJsonArray coordinatesJson() const;
        void clearEditor();
        void updatePreview(const QJsonObject &resource);
        void updateCoordinateDisplay();
        void syncDetailTabs();
        void connectEditorPreviewInputs();
        void refreshEditorLiveState();
        QString resourceDisplayName(const QJsonObject &resource) const;
        QString resourceDescription(const QJsonObject &resource) const;
        QString detailsTitleForCurrentState() const;
    protected:
        void refreshWorkflowTexts();
        virtual QString searchPlaceholderText() const;
        virtual QString namePlaceholderText() const;
        virtual QString descriptionPlaceholderText() const;
        virtual QString typePlaceholderText() const;
        virtual QString coordinatesPlaceholderText() const;
        virtual QString geometryPlaceholderText() const;
        virtual QString importButtonText() const;
        virtual QString exportButtonText() const;
        virtual QString importFileFilter() const;
        virtual QString exportFileFilter() const;
        virtual QString exportDefaultFileName() const;
        virtual QString primaryRowActionToolTip() const;
        virtual QIcon primaryRowActionIcon() const;
        virtual void triggerPrimaryAction(const QString &id, const QJsonObject &resource);
        virtual bool importResourcesFromPath(const QString &fileName,
                                             QList<QPair<QString, QJsonObject>> *resources,
                                             QString *message) const;
        virtual bool exportResourcesToPath(const QString &fileName,
                                           const QList<QPair<QString, QJsonObject>> &resources,
                                           QString *message) const;
        QFormLayout *editorFormLayout() const;
        QLineEdit *nameEdit() const;
        QLineEdit *descriptionEdit() const;
        QLineEdit *hrefEdit() const;
        QLineEdit *mimeTypeEdit() const;
        QLineEdit *identifierEdit() const;
        QLineEdit *chartFormatEdit() const;
        QLineEdit *chartUrlEdit() const;
        QLineEdit *tilemapUrlEdit() const;
        QString currentResourceId() const;
        void showWorkflowError(const QString &message) const;

        ::Ui::ResourceTab *ui = nullptr;
        ResourceKind m_kind;
        ResourceModel *m_model;
        QStackedWidget *m_stackedWidget;
        QTabWidget *m_detailTabs = nullptr;
        QWidget *m_listPage;
        QWidget *m_detailsPage;
        QFormLayout *m_formLayout = nullptr;
        QWidget *m_propertiesTreeTab = nullptr;
        QWidget *m_propertiesJsonTab = nullptr;
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
        QWidget *m_coordinateRowWidget;
        QLineEdit *m_coordinateDisplayEdit;
        QToolButton *m_coordinateEditButton;
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
