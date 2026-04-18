//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
#define FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;
namespace Ui { class HistoryTrackTab; }

namespace fairwindsk::ui::widgets {
    class TouchComboBox;
}

namespace fairwindsk::ui::mydata {

    class HistoryTrackModel;
    struct HistoryTrackPoint;

    class HistoryTrackTab final : public QWidget {
        Q_OBJECT

    public:
        explicit HistoryTrackTab(QWidget *parent = nullptr);
        ~HistoryTrackTab() override;

    protected:
        void changeEvent(QEvent *event) override;

    private slots:
        void rebuildTable();
        void onRefreshClicked();
        void onDurationChanged();
        void onImportClicked();
        void onExportClicked();
        void onOpenClicked();
        void onEditClicked();
        void onAddClicked();
        void onDeleteClicked();
        void onTableDoubleClicked(int row, int column);
        void updateActionState();

    private:
        void configureTable();
        void applyTouchFriendlyStyling();
        void updateStatus(const QString &message);
        void updateStatusLabel();
        int selectedSourceRow() const;
        QList<HistoryTrackPoint> allPoints() const;
        bool editTrackPoint(int row, bool creating);
        QString currentDuration() const;
        QString currentResolution() const;

        ::Ui::HistoryTrackTab *ui = nullptr;
        HistoryTrackModel *m_model = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_statusLabel = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_durationCombo = nullptr;
        QTableWidget *m_tableWidget = nullptr;
        QPushButton *m_openButton = nullptr;
        QPushButton *m_editButton = nullptr;
        QPushButton *m_addButton = nullptr;
        QPushButton *m_deleteButton = nullptr;
        QPushButton *m_importButton = nullptr;
        QPushButton *m_exportButton = nullptr;
        QPushButton *m_refreshButton = nullptr;
        QTimer *m_refreshTimer = nullptr;
        bool m_isShuttingDown = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
