//
// Created by Codex on 07/04/26.
//

#ifndef FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP
#define FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP

#include <QString>
#include <QJsonObject>

namespace fairwindsk::runtime {
    enum class LogLevel {
        Off,
        Critical,
        Warning,
        Info,
        Debug,
        Full
    };

    QString defaultDiagnosticsEmail();
    QString defaultDiagnosticsSubject();
    QString persistentLogsDirectoryPath();
    QString persistentReportsDirectoryPath();
    QString logLevelToString(LogLevel level);
    QString logLevelDisplayName(LogLevel level);
    LogLevel logLevelFromString(const QString &value);

    void initializeDiagnostics();
    void dispatchPendingDiagnosticsReport();
    void markGracefulShutdown();
    void applyLiveSettings(LogLevel level, bool persistentLogging, bool interactionHistoryEnabled);
    void recordUserInteraction(const QString &category,
                               const QString &action,
                               const QString &target = QString(),
                               const QJsonObject &context = QJsonObject());
}

#endif // FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP
