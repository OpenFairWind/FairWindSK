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
class QSortFilterProxyModel;
class QStackedWidget;
class QTableView;
class QToolButton;
class QDoubleSpinBox;

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget;
    class WaypointTableProxyModel;

    class Waypoints final : public QWidget {
        Q_OBJECT

    public:
        explicit Waypoints(QWidget *parent = nullptr);

    private slots:
        void onSearchTextChanged(const QString &text);
        void onAddClicked();
        void onImportClicked();
        void onExportClicked();
        void onRefreshClicked();
        void onTableDoubleClicked(const QModelIndex &index);
        void onNavigateRowClicked();
        void onShowDetailsRowClicked();
        void onBackClicked();
        void onNavigateCurrentClicked();
        void onEditClicked();
        void onDeleteClicked();
        void onSaveClicked();
        void onCancelClicked();

    private:
        QModelIndex currentSourceIndex() const;
        QModelIndex sourceIndexForProxyRow(int proxyRow) const;
        void updateActionButtons();
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

        ResourceModel *m_model = nullptr;
        WaypointTableProxyModel *m_proxyModel = nullptr;
        QStackedWidget *m_stackedWidget = nullptr;
        QWidget *m_listPage = nullptr;
        QWidget *m_detailsPage = nullptr;
        QLineEdit *m_searchEdit = nullptr;
        QTableView *m_tableView = nullptr;
        QToolButton *m_addButton = nullptr;
        QToolButton *m_importButton = nullptr;
        QToolButton *m_exportButton = nullptr;
        QToolButton *m_refreshButton = nullptr;
        QToolButton *m_backButton = nullptr;
        QToolButton *m_newButton = nullptr;
        QToolButton *m_navigateButton = nullptr;
        QToolButton *m_editButton = nullptr;
        QToolButton *m_saveButton = nullptr;
        QToolButton *m_cancelButton = nullptr;
        QToolButton *m_deleteButton = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLineEdit *m_nameEdit = nullptr;
        QLineEdit *m_descriptionEdit = nullptr;
        QLineEdit *m_typeEdit = nullptr;
        QDoubleSpinBox *m_latitudeSpinBox = nullptr;
        QDoubleSpinBox *m_longitudeSpinBox = nullptr;
        QDoubleSpinBox *m_altitudeSpinBox = nullptr;
        QPlainTextEdit *m_propertiesEdit = nullptr;
        GeoJsonPreviewWidget *m_previewWidget = nullptr;
        QLabel *m_idValueLabel = nullptr;
        QLabel *m_timestampValueLabel = nullptr;
        QString m_currentWaypointId;
        bool m_isEditing = false;
        bool m_isCreating = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_WAYPOINTS_HPP
