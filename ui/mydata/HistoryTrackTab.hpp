//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
#define FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP

#include <QWidget>

class QLabel;
class QComboBox;
class QTableView;
class QToolButton;
class QTimer;

namespace fairwindsk::ui::mydata {

    class HistoryTrackModel;

    class HistoryTrackTab final : public QWidget {
        Q_OBJECT

    public:
        explicit HistoryTrackTab(QWidget *parent = nullptr);

    private slots:
        void onRefreshClicked();
        void onDurationChanged();
        void onImportClicked();
        void onExportClicked();

    private:
        QString currentDuration() const;
        QString currentResolution() const;
        void updateStatus(const QString &message);

        HistoryTrackModel *m_model = nullptr;
        QLabel *m_statusLabel = nullptr;
        QComboBox *m_durationCombo = nullptr;
        QTableView *m_tableView = nullptr;
        QToolButton *m_refreshButton = nullptr;
        QToolButton *m_importButton = nullptr;
        QToolButton *m_exportButton = nullptr;
        QTimer *m_refreshTimer = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_HISTORYTRACKTAB_HPP
