//
// Created by Codex on 28/03/26.
//

#include "System.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "signalk/Client.hpp"
#include "ui_System.h"

#if defined(Q_OS_MACOS)
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/task.h>
#include <sys/sysctl.h>
#elif defined(Q_OS_LINUX)
#include <QFile>
#include <QTextStream>
#include <unistd.h>
#endif

namespace fairwindsk::ui::settings {
    System::System(Settings *settings, QWidget *parent) : QWidget(parent), ui(new Ui::System), m_settings(settings) {
        ui->setupUi(this);

        ui->splitter_Panes->setStretchFactor(0, 1);
        ui->splitter_Panes->setStretchFactor(1, 1);

        auto *coresLayout = new QVBoxLayout(ui->widget_CoresHost);
        coresLayout->setContentsMargins(0, 0, 0, 0);
        coresLayout->setSpacing(8);

        connect(ui->pushButton_Reset, &QPushButton::clicked, this, [this]() {
            m_settings->resetToCurrentConfiguration();
        });
        connect(ui->pushButton_RestoreDefaults, &QPushButton::clicked, this, [this]() {
            m_settings->restoreDefaultConfiguration();
        });
        connect(ui->pushButton_Restart, &QPushButton::clicked, this, [this]() {
            m_settings->restartApplication();
        });
        connect(ui->pushButton_Quit, &QPushButton::clicked, this, [this]() {
            m_settings->quitApplication();
        });

        m_previousCpuStats = sampleCpuStats();

        auto *timer = new QTimer(this);
        timer->setInterval(1000);
        connect(timer, &QTimer::timeout, this, &System::refreshDiagnostics);
        timer->start();

        refreshDiagnostics();
    }

    System::~System() {
        delete ui;
        ui = nullptr;
    }

    void System::refreshDiagnostics() {
        const quint64 processMemory = processResidentMemoryBytes();
        const quint64 totalMemory = totalMemoryBytes();
        ui->label_ProcessMemoryValue->setText(formatBytes(processMemory));
        ui->label_TotalMemoryValue->setText(totalMemory > 0 ? formatBytes(totalMemory) : tr("Unavailable"));

        if (totalMemory > 0) {
            const auto percentage = int(std::clamp<double>((double(processMemory) / double(totalMemory)) * 100.0, 0.0, 100.0));
            ui->progressBar_MemoryUsage->setValue(percentage);
            ui->progressBar_MemoryUsage->setFormat(QStringLiteral("%1%").arg(percentage));
        } else {
            ui->progressBar_MemoryUsage->setValue(0);
            ui->progressBar_MemoryUsage->setFormat(tr("Unavailable"));
        }

        const QVector<CpuSnapshot> currentCpuStats = sampleCpuStats();
        const int coreCount = std::min(m_previousCpuStats.size(), currentCpuStats.size());
        ensureCoreWidgets(coreCount);

        auto *layout = qobject_cast<QVBoxLayout *>(ui->widget_CoresHost->layout());
        if (!layout) {
            return;
        }

        for (int i = 0; i < m_coreRows.size(); ++i) {
            auto *rowWidget = m_coreRows.at(i);
            auto *rowLayout = qobject_cast<QHBoxLayout *>(rowWidget->layout());
            if (!rowLayout) {
                continue;
            }
            auto *label = qobject_cast<QLabel *>(rowLayout->itemAt(0)->widget());
            auto *progressBar = qobject_cast<QProgressBar *>(rowLayout->itemAt(1)->widget());
            if (!label || !progressBar) {
                continue;
            }

            if (i >= coreCount) {
                label->setText(tr("Core %1").arg(i + 1));
                progressBar->setValue(0);
                progressBar->setFormat(tr("Unavailable"));
                continue;
            }

            const auto previous = m_previousCpuStats.at(i);
            const auto current = currentCpuStats.at(i);
            const quint64 previousBusy = previous.user + previous.system + previous.nice;
            const quint64 currentBusy = current.user + current.system + current.nice;
            const quint64 previousTotal = previousBusy + previous.idle;
            const quint64 currentTotal = currentBusy + current.idle;
            const quint64 totalDelta = currentTotal > previousTotal ? currentTotal - previousTotal : 0;
            const quint64 busyDelta = currentBusy > previousBusy ? currentBusy - previousBusy : 0;

            int percentage = 0;
            if (totalDelta > 0) {
                percentage = int(std::clamp<double>((double(busyDelta) / double(totalDelta)) * 100.0, 0.0, 100.0));
            }

            label->setText(tr("Core %1").arg(i + 1));
            progressBar->setValue(percentage);
            progressBar->setFormat(QStringLiteral("%1%").arg(percentage));
        }

        m_previousCpuStats = currentCpuStats;
        refreshRpiDiagnostics();
    }

    void System::ensureCoreWidgets(const int coreCount) {
        auto *layout = qobject_cast<QVBoxLayout *>(ui->widget_CoresHost->layout());
        if (!layout) {
            return;
        }

        while (m_coreRows.size() < coreCount) {
            auto *rowWidget = new QWidget(ui->widget_CoresHost);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);

            auto *label = new QLabel(rowWidget);
            auto *progressBar = new QProgressBar(rowWidget);
            progressBar->setRange(0, 100);
            progressBar->setValue(0);
            progressBar->setTextVisible(true);

            rowLayout->addWidget(label);
            rowLayout->addWidget(progressBar, 1);
            layout->addWidget(rowWidget);
            m_coreRows.append(rowWidget);
        }

        if (layout->count() == m_coreRows.size()) {
            layout->addStretch(1);
        }
    }

    void System::ensureRpiDiagnosticsWidgets() {
        if (m_rpiGroupBox) {
            return;
        }

        m_rpiGroupBox = new QGroupBox(tr("Raspberry Pi"), this);
        m_rpiFormLayout = new QFormLayout(m_rpiGroupBox);
        m_rpiFormLayout->setContentsMargins(0, 0, 0, 0);
        m_rpiFormLayout->setSpacing(8);

        const QList<QPair<QString, QString>> metrics = {
            {QStringLiteral("environment.rpi.cpu.temperature"), tr("CPU temperature")},
            {QStringLiteral("environment.rpi.gpu.temperature"), tr("GPU temperature")},
            {QStringLiteral("environment.rpi.cpu.utilisation"), tr("CPU utilisation")},
            {QStringLiteral("environment.rpi.memory.utilisation"), tr("Memory utilisation")},
            {QStringLiteral("environment.rpi.sd.utilisation"), tr("SD utilisation")}
        };

        for (const auto &metric : metrics) {
            auto *valueLabel = new QLabel(tr("Unavailable"), m_rpiGroupBox);
            m_rpiFormLayout->addRow(metric.second, valueLabel);
            m_rpiMetricValues.insert(metric.first, valueLabel);
        }

        m_rpiGroupBox->setVisible(false);
        ui->verticalLayout_Diagnostics->addWidget(m_rpiGroupBox);
    }

    double System::fetchSignalKRpiMetric(const QString &path, bool *available) const {
        if (available) {
            *available = false;
        }

        auto *client = FairWindSK::getInstance()->getSignalKClient();
        if (!client || client->url().isEmpty() || !client->isRestHealthy()) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        const QJsonObject update = client->signalkGet(path);
        if (update.isEmpty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        const double value = signalk::Client::getDoubleFromUpdateByPath(update, path);
        if (std::isnan(value)) {
            return value;
        }

        if (available) {
            *available = true;
        }
        return value;
    }

    void System::setRpiMetricValue(const QString &path, const QString &text) {
        if (auto *label = m_rpiMetricValues.value(path, nullptr)) {
            label->setText(text);
        }
    }

    void System::refreshRpiDiagnostics() {
        ensureRpiDiagnosticsWidgets();
        if (!m_rpiGroupBox) {
            return;
        }

        const QDateTime now = QDateTime::currentDateTimeUtc();
        if (m_lastRpiRefresh.isValid() && m_lastRpiRefresh.msecsTo(now) < 5000) {
            return;
        }
        m_lastRpiRefresh = now;

        bool hasAnyMetric = false;

        auto applyTemperature = [this, &hasAnyMetric](const QString &path) {
            bool available = false;
            const double value = fetchSignalKRpiMetric(path, &available);
            if (available) {
                hasAnyMetric = true;
                setRpiMetricValue(path, tr("%1 °C").arg(QString::number(value, 'f', 1)));
            } else {
                setRpiMetricValue(path, tr("Unavailable"));
            }
        };

        auto applyPercent = [this, &hasAnyMetric](const QString &path) {
            bool available = false;
            const double value = fetchSignalKRpiMetric(path, &available);
            if (available) {
                hasAnyMetric = true;
                const double percentage = value <= 1.0 ? value * 100.0 : value;
                setRpiMetricValue(path, tr("%1%").arg(QString::number(percentage, 'f', 0)));
            } else {
                setRpiMetricValue(path, tr("Unavailable"));
            }
        };

        applyTemperature(QStringLiteral("environment.rpi.cpu.temperature"));
        applyTemperature(QStringLiteral("environment.rpi.gpu.temperature"));
        applyPercent(QStringLiteral("environment.rpi.cpu.utilisation"));
        applyPercent(QStringLiteral("environment.rpi.memory.utilisation"));
        applyPercent(QStringLiteral("environment.rpi.sd.utilisation"));

        m_hasRpiMetrics = hasAnyMetric;
        m_rpiGroupBox->setVisible(m_hasRpiMetrics);
    }

    QString System::formatBytes(const quint64 bytes) const {
        static const QVector<QString> units = {tr("B"), tr("KB"), tr("MB"), tr("GB"), tr("TB")};
        double value = double(bytes);
        int unitIndex = 0;
        while (value >= 1024.0 && unitIndex < units.size() - 1) {
            value /= 1024.0;
            ++unitIndex;
        }
        return QStringLiteral("%1 %2").arg(QString::number(value, 'f', unitIndex == 0 ? 0 : 1), units.at(unitIndex));
    }

    quint64 System::processResidentMemoryBytes() const {
#if defined(Q_OS_MACOS)
        mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &infoCount) == KERN_SUCCESS) {
            return quint64(info.resident_size);
        }
#elif defined(Q_OS_LINUX)
        QFile file(QStringLiteral("/proc/self/statm"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            quint64 sizePages = 0;
            quint64 residentPages = 0;
            stream >> sizePages >> residentPages;
            return residentPages * quint64(sysconf(_SC_PAGESIZE));
        }
#endif
        return 0;
    }

    quint64 System::totalMemoryBytes() const {
#if defined(Q_OS_MACOS)
        int mib[2] = {CTL_HW, HW_MEMSIZE};
        uint64_t size = 0;
        size_t length = sizeof(size);
        if (sysctl(mib, 2, &size, &length, nullptr, 0) == 0) {
            return quint64(size);
        }
#elif defined(Q_OS_LINUX)
        QFile file(QStringLiteral("/proc/meminfo"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                const QByteArray line = file.readLine();
                if (line.startsWith("MemTotal:")) {
                    const QList<QByteArray> parts = line.simplified().split(' ');
                    if (parts.size() >= 2) {
                        return parts.at(1).toULongLong() * 1024ULL;
                    }
                }
            }
        }
#endif
        return 0;
    }

    QVector<System::CpuSnapshot> System::sampleCpuStats() const {
        QVector<CpuSnapshot> snapshots;
#if defined(Q_OS_MACOS)
        processor_info_array_t cpuInfo;
        mach_msg_type_number_t cpuInfoCount;
        natural_t cpuCount;
        if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &cpuInfo, &cpuInfoCount) == KERN_SUCCESS) {
            snapshots.reserve(int(cpuCount));
            for (natural_t i = 0; i < cpuCount; ++i) {
                CpuSnapshot snapshot;
                snapshot.user = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER];
                snapshot.system = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM];
                snapshot.idle = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE];
                snapshot.nice = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_NICE];
                snapshots.append(snapshot);
            }
            vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(cpuInfo), cpuInfoCount * sizeof(integer_t));
        }
#elif defined(Q_OS_LINUX)
        QFile file(QStringLiteral("/proc/stat"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                const QByteArray line = file.readLine().trimmed();
                if (!line.startsWith("cpu") || line.startsWith("cpu ")) {
                    continue;
                }
                const QList<QByteArray> parts = line.simplified().split(' ');
                if (parts.size() < 5) {
                    continue;
                }
                CpuSnapshot snapshot;
                snapshot.user = parts.value(1).toULongLong();
                snapshot.nice = parts.value(2).toULongLong();
                snapshot.system = parts.value(3).toULongLong();
                snapshot.idle = parts.value(4).toULongLong();
                snapshots.append(snapshot);
            }
        }
#endif
        return snapshots;
    }
}
