//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
#define FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP

#include <QWidget>

class QDateTimeEdit;
class QLabel;
class QDoubleSpinBox;
class QStackedWidget;
class QTableWidget;
class QToolButton;
class QTimer;
namespace Ui { class HistoryTrackTab; }

namespace fairwindsk::ui::widgets {
    class TouchComboBox;
}

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget;
    class JsonObjectEditorWidget;
    class HistoryTrackModel;
    struct HistoryTrackPoint;

    class HistoryTrackTab final : public QWidget {
        Q_OBJECT

    public:
        explicit HistoryTrackTab(QWidget *parent = nullptr);
        ~HistoryTrackTab() override;

    private slots:
        void onRefreshClicked();
        void onDurationChanged();
        void onImportClicked();
        void onExportClicked();
        void onOpenClicked();
        void onTableDoubleClicked(int row, int column);
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
        int selectedSourceRow() const;
        void rebuildTable();
        void styleTable();
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

        ::Ui::HistoryTrackTab *ui = nullptr;
        HistoryTrackModel *m_model = nullptr;
        QStackedWidget *m_stackedWidget = nullptr;
        QWidget *m_listPage = nullptr;
        QWidget *m_detailsPage = nullptr;
        QLabel *m_statusLabel = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_indexValueLabel = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_durationCombo = nullptr;
        QTableWidget *m_tableWidget = nullptr;
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
        QList<int> m_visibleRows;
        int m_currentRow = -1;
        bool m_isEditing = false;
        bool m_isCreating = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
