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

    private slots:
        void refreshDiagnostics();

    private:
        struct CpuSnapshot {
            quint64 user = 0;
            quint64 system = 0;
            quint64 idle = 0;
            quint64 nice = 0;
        };

        void ensureCoreWidgets(int coreCount);
        void ensureRpiDiagnosticsWidgets();
        void refreshRpiDiagnostics();
        double fetchSignalKRpiMetric(const QString &path, bool *available = nullptr) const;
        void setRpiMetricValue(const QString &path, const QString &text);
        QString formatBytes(quint64 bytes) const;
        quint64 processResidentMemoryBytes() const;
        quint64 totalMemoryBytes() const;
        QVector<CpuSnapshot> sampleCpuStats() const;

    private:
        Ui::System *ui = nullptr;
        Settings *m_settings = nullptr;
        QVector<QWidget *> m_coreRows;
        QVector<CpuSnapshot> m_previousCpuStats;
        QGroupBox *m_rpiGroupBox = nullptr;
        QFormLayout *m_rpiFormLayout = nullptr;
        QMap<QString, QLabel *> m_rpiMetricValues;
        QDateTime m_lastRpiRefresh;
        bool m_hasRpiMetrics = false;
    };
}

#endif
