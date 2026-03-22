//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
#define FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP

#include <QWidget>

class QDateTimeEdit;
class QLabel;
class QComboBox;
class QDoubleSpinBox;
class QStackedWidget;
class QTableView;
class QToolButton;
class QTimer;

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget;
    class JsonObjectEditorWidget;
    class HistoryTrackModel;
    class HistoryTrackTableProxyModel;
    struct HistoryTrackPoint;

    class HistoryTrackTab final : public QWidget {
        Q_OBJECT

    public:
        explicit HistoryTrackTab(QWidget *parent = nullptr);

    private slots:
        void onRefreshClicked();
        void onDurationChanged();
        void onImportClicked();
        void onExportClicked();
        void onOpenClicked();
        void onTableDoubleClicked(const QModelIndex &index);
        void onNavigateRowClicked();
        void onEditRowClicked();
        void onRemoveRowClicked();
        void onBackClicked();
        void onAddClicked();
        void onEditClicked();
        void onSaveClicked();
        void onCancelClicked();
        void onDeleteClicked();

    private:
        QModelIndex proxyIndexForRow(int row, int column = 0) const;
        void clearActionWidgets();
        void updateActionButtons();
        void showListPage();
        void showDetailsPage(int row, bool editMode);
        void setEditMode(bool editMode);
        void populateEditor(int row);
        void clearEditor();
        int currentRow() const;
        void updateStatusLabel();
        QList<HistoryTrackPoint> allPoints() const;
        void updatePreview();

        QString currentDuration() const;
        QString currentResolution() const;
        void updateStatus(const QString &message);

        HistoryTrackModel *m_model = nullptr;
        HistoryTrackTableProxyModel *m_proxyModel = nullptr;
        QStackedWidget *m_stackedWidget = nullptr;
        QWidget *m_listPage = nullptr;
        QWidget *m_detailsPage = nullptr;
        QLabel *m_statusLabel = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_indexValueLabel = nullptr;
        QComboBox *m_durationCombo = nullptr;
        QTableView *m_tableView = nullptr;
        QToolButton *m_refreshButton = nullptr;
        QToolButton *m_importButton = nullptr;
        QToolButton *m_exportButton = nullptr;
        QToolButton *m_backButton = nullptr;
        QToolButton *m_newButton = nullptr;
        QToolButton *m_editButton = nullptr;
        QToolButton *m_saveButton = nullptr;
        QToolButton *m_cancelButton = nullptr;
        QToolButton *m_deleteButton = nullptr;
        QDateTimeEdit *m_timestampEdit = nullptr;
        QDoubleSpinBox *m_latitudeSpinBox = nullptr;
        QDoubleSpinBox *m_longitudeSpinBox = nullptr;
        QDoubleSpinBox *m_altitudeSpinBox = nullptr;
        JsonObjectEditorWidget *m_propertiesEditor = nullptr;
        GeoJsonPreviewWidget *m_previewWidget = nullptr;
        QTimer *m_refreshTimer = nullptr;
        int m_currentRow = -1;
        bool m_isEditing = false;
        bool m_isCreating = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
