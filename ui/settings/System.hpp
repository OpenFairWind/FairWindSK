//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_UI_SETTINGS_SYSTEM_HPP
#define FAIRWINDSK_UI_SETTINGS_SYSTEM_HPP

#include <QWidget>
#include <QVector>

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
        QString formatBytes(quint64 bytes) const;
        quint64 processResidentMemoryBytes() const;
        quint64 totalMemoryBytes() const;
        QVector<CpuSnapshot> sampleCpuStats() const;

    private:
        Ui::System *ui = nullptr;
        Settings *m_settings = nullptr;
        QVector<QWidget *> m_coreRows;
        QVector<CpuSnapshot> m_previousCpuStats;
    };
}

#endif
