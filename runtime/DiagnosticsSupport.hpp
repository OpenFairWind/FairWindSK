//
// Created by Codex on 07/04/26.
//

#ifndef FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP
#define FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP

#include <QString>

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
    QString logLevelToString(LogLevel level);
    QString logLevelDisplayName(LogLevel level);
    LogLevel logLevelFromString(const QString &value);

    void initializeDiagnostics();
    void dispatchPendingDiagnosticsEmail();
    void markGracefulShutdown();
    void applyLiveSettings(LogLevel level, bool persistentLogging);
}

#endif // FAIRWINDSK_RUNTIME_DIAGNOSTICSSUPPORT_HPP
