//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_UI_SETTINGS_SYSTEM_HPP
#define FAIRWINDSK_UI_SETTINGS_SYSTEM_HPP

#include <QWidget>
#include <QDateTime>
#include <QMap>
#include <QVector>

class QLabel;
class QFormLayout;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace fairwindsk::ui::widgets {
    class TouchCheckBox;
    class TouchComboBox;
}

namespace fairwindsk::ui::settings {
    class Settings;

    QT_BEGIN_NAMESPACE
    namespace Ui { class System; }
    QT_END_NAMESPACE

    class System : public QWidget {
        Q_OBJECT

    public:
        explicit System(Settings *settings, QWidget *parent = nullptr);
        ~System() override;
        static bool replaceConfigurationFile(const QString &sourcePath, const QString &targetPath, QString *errorMessage = nullptr);
        static bool runConfigurationImportSelfTest(QString *failureReason = nullptr);

    private slots:
        void refreshDiagnostics();
        void importConfiguration();
        void exportConfiguration();

    private:
        struct CpuSnapshot {
            quint64 user = 0;
            quint64 system = 0;
            quint64 idle = 0;
            quint64 nice = 0;
        };

        void ensureCoreWidgets(int coreCount);
        void ensureLoggingSettingsWidgets();
        void ensureRpiDiagnosticsWidgets();
        bool confirmAction(const QString &title, const QString &message, const QString &confirmText);
        void refreshRpiDiagnostics();
        double fetchSignalKRpiMetric(const QJsonObject &root, const QString &path, bool *available = nullptr) const;
        void setRpiMetricValue(const QString &path, const QString &text);
        void syncLoggingSettings();
        QString currentConfigurationPath() const;
        void ensureNetworkWidgets();
        void refreshNetworkInfo();
        QString formatBytes(quint64 bytes) const;
        quint64 processResidentMemoryBytes() const;
        quint64 totalMemoryBytes() const;
        QVector<CpuSnapshot> sampleCpuStats() const;

    private:
        Ui::System *ui = nullptr;
        Settings *m_settings = nullptr;
        QVector<QWidget *> m_coreRows;
        QVector<CpuSnapshot> m_previousCpuStats;
        QGroupBox *m_networkGroupBox = nullptr;
        QLabel *m_networkAddressesValue = nullptr;
        QDateTime m_lastNetworkRefresh;
        QGroupBox *m_rpiGroupBox = nullptr;
        QFormLayout *m_rpiFormLayout = nullptr;
        QMap<QString, QLabel *> m_rpiMetricValues;
        QGroupBox *m_loggingGroupBox = nullptr;
        QFormLayout *m_loggingFormLayout = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_logLevelComboBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_persistentLoggingCheckBox = nullptr;
        fairwindsk::ui::widgets::TouchCheckBox *m_interactionHistoryCheckBox = nullptr;
        QLineEdit *m_diagnosticsEmailEdit = nullptr;
        QLabel *m_logDirectoryValue = nullptr;
        QLabel *m_reportDirectoryValue = nullptr;
        QLabel *m_diagnosticsSubjectValue = nullptr;
        QPushButton *m_viewLogsButton = nullptr;
        QPushButton *m_viewReportsButton = nullptr;
        QVBoxLayout *m_loggingDetailsLayout = nullptr;
        QDateTime m_lastRpiRefresh;
        bool m_hasRpiMetrics = false;
    };
}

#endif
