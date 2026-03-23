//
// Created by Raffaele Montella on 15/02/25.
//

#ifndef FAIRWINDSK_UI_MYDATA_WAYPOINTS_HPP
#define FAIRWINDSK_UI_MYDATA_WAYPOINTS_HPP

#include <QJsonObject>
#include <QWidget>

#include "ResourceModel.hpp"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QFormLayout;
class QHBoxLayout;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QToolButton;
class QDoubleSpinBox;
class QTimer;
class QWebEngineView;
namespace Ui { class Waypoints; }

namespace fairwindsk::ui::mydata {

    class JsonObjectEditorWidget;

    class Waypoints final : public QWidget {
        Q_OBJECT

    public:
        explicit Waypoints(QWidget *parent = nullptr);
        ~Waypoints() override;

    private slots:
        void onSearchTextChanged(const QString &text);
        void onAddClicked();
        void onImportClicked();
        void onExportClicked();
        void onRefreshClicked();
        void onSearchTimeout();
        void onTableDoubleClicked(int row, int column);
        void onNavigateRowClicked();
        void onDetailsRowClicked();
        void onEditRowClicked();
        void onRemoveRowClicked();
        void onSelectAllClicked();
        void onRemoveSelectedClicked();
        void onBackClicked();
        void onNavigateCurrentClicked();
        void onEditClicked();
        void onDeleteClicked();
        void onSaveClicked();
        void onCancelClicked();

    private:
        QString currentWaypointIdFromSelection() const;
        void rebuildTable();
        void applySearchFilter();
        void styleTable();
        void setBusy(bool busy, const QString &label = QString(), int maximum = 0);
        void updateBulkButtons();
        QStringList visibleWaypointIds() const;
        void syncDetailTabs();
        void updateSpecialFieldVisibility();
        void rebuildSeaFloorTypeIcons(const QJsonArray &types);
        void showListPage();
        void showDetailsPage(const QString &id, const QJsonObject &resource, bool editMode);
        void applyWaypointToEditor(const QString &id, const QJsonObject &resource);
        QJsonObject waypointFromEditor() const;
        bool validateEditor(QString *message) const;
        void setEditMode(bool editMode);
        void setActionButtonsForReadOnly();
        void setActionButtonsForEdit();
        void selectWaypoint(const QString &id);
        void navigateToWaypoint(const QString &id, const QJsonObject &resource);
        void deleteWaypoint(const QString &id, const QString &name);
        void showError(const QString &message) const;
        void clearEditor();
        void updatePreview(const QJsonObject &resource);
        QString waypointHref(const QString &id) const;

        ::Ui::Waypoints *ui = nullptr;
        ResourceModel *m_model = nullptr;
        QStackedWidget *m_stackedWidget = nullptr;
        QWidget *m_listPage = nullptr;
        QWidget *m_detailsPage = nullptr;
        QFormLayout *m_detailsFormLayout = nullptr;
        QStackedWidget *m_searchStack = nullptr;
        QLineEdit *m_searchEdit = nullptr;
        QProgressBar *m_progressBar = nullptr;
        QTableWidget *m_tableWidget = nullptr;
        QToolButton *m_addButton = nullptr;
        QToolButton *m_importButton = nullptr;
        QToolButton *m_exportButton = nullptr;
        QToolButton *m_refreshButton = nullptr;
        QToolButton *m_selectAllButton = nullptr;
        QToolButton *m_bulkRemoveButton = nullptr;
        QToolButton *m_backButton = nullptr;
        QToolButton *m_newButton = nullptr;
        QToolButton *m_navigateButton = nullptr;
        QToolButton *m_editButton = nullptr;
        QToolButton *m_saveButton = nullptr;
        QToolButton *m_cancelButton = nullptr;
        QToolButton *m_deleteButton = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLineEdit *m_nameEdit = nullptr;
        QPlainTextEdit *m_descriptionEdit = nullptr;
        QLineEdit *m_typeEdit = nullptr;
        QDoubleSpinBox *m_latitudeSpinBox = nullptr;
        QDoubleSpinBox *m_longitudeSpinBox = nullptr;
        QDoubleSpinBox *m_altitudeSpinBox = nullptr;
        QPlainTextEdit *m_contactsEdit = nullptr;
        QLabel *m_contactsLabel = nullptr;
        QTabWidget *m_detailTabs = nullptr;
        QWidget *m_propertiesTreeTab = nullptr;
        QWidget *m_propertiesJsonTab = nullptr;
        QPlainTextEdit *m_geoJsonDetailsEdit = nullptr;
        QWebEngineView *m_mapPreviewView = nullptr;
        QLabel *m_seaFloorRowLabel = nullptr;
        QLabel *m_slipsRowLabel = nullptr;
        QWidget *m_seaFloorWidget = nullptr;
        QWidget *m_slipsWidget = nullptr;
        QWidget *m_seaFloorTypeWidget = nullptr;
        QHBoxLayout *m_seaFloorTypeLayout = nullptr;
        QLabel *m_seaFloorMinValueLabel = nullptr;
        QLabel *m_seaFloorMaxValueLabel = nullptr;
        QLabel *m_slipsValueLabel = nullptr;
        JsonObjectEditorWidget *m_propertiesEditor = nullptr;
        QTimer *m_searchTimer = nullptr;
        QString m_currentWaypointId;
        QStringList m_visibleWaypointIds;
        QStringList m_searchHaystacks;
        bool m_isEditing = false;
        bool m_isCreating = false;
        bool m_isBusy = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_WAYPOINTS_HPP
