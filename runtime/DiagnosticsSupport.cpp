//
// Created by Codex on 07/04/26.
//

#include "runtime/DiagnosticsSupport.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>

#include "Configuration.hpp"

using namespace Qt::StringLiterals;

namespace fairwindsk::runtime {
    namespace {
        struct DiagnosticsOptions {
            LogLevel logLevel = LogLevel::Off;
            bool persistentLogging = true;
            bool interactionHistoryEnabled = true;
            QString email = defaultDiagnosticsEmail();
            QString subject = defaultDiagnosticsSubject();
        };

        struct PreviousRunState {
            bool valid = false;
            bool graceful = true;
            QString startUtc;
            QString logPath;
            QString interactionPath;
        };

        struct PendingDiagnosticsReport {
            bool pending = false;
            QString currentRunId;
            PreviousRunState previousRun;
            DiagnosticsOptions options;
        };

        QMutex g_logMutex;
        QFile *g_runLogFile = nullptr;
        QFile *g_runInteractionFile = nullptr;
        DiagnosticsOptions g_diagnosticsOptions;
        PendingDiagnosticsReport g_pendingReport;
        QString g_currentRunId;
        QString g_currentRunLogPath;
        QString g_currentRunInteractionPath;
        QString g_currentRunStartUtc;

        QString defaultConfigFilename() {
            QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
            if (configDir.trimmed().isEmpty()) {
                configDir = QDir::homePath();
            }
            QDir().mkpath(configDir);
            return QDir(configDir).filePath(QStringLiteral("fairwindsk.json"));
        }

        QString normalizedConfigFilename(const QString &filename) {
            const QString trimmed = filename.trimmed();
            if (trimmed.isEmpty()) {
                return defaultConfigFilename();
            }

            const QFileInfo fileInfo(trimmed);
            if (fileInfo.isAbsolute()) {
                QDir().mkpath(fileInfo.absolutePath());
                return fileInfo.absoluteFilePath();
            }

            return QDir(QFileInfo(defaultConfigFilename()).absolutePath()).filePath(trimmed);
        }

        QString appDataDirectoryPath() {
            QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (baseDir.trimmed().isEmpty()) {
                baseDir = QDir::homePath();
            }
            QDir().mkpath(baseDir);
            return baseDir;
        }

        QString runsDirectoryPath() {
            const QString path = QDir(appDataDirectoryPath()).filePath(QStringLiteral("logs/runs"));
            QDir().mkpath(path);
            return path;
        }

        QString reportsDirectoryPath() {
            const QString path = QDir(appDataDirectoryPath()).filePath(QStringLiteral("logs/reports"));
            QDir().mkpath(path);
            return path;
        }

        QString diagnosticsStateKey(const QString &key) {
            return QStringLiteral("diagnostics/%1").arg(key);
        }

        QJsonObject loadStoredConfiguration() {
            QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
            const QString configPath = normalizedConfigFilename(settings.value("config").toString());
            QFile configFile(configPath);
            if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return {};
            }

            const QJsonDocument document = QJsonDocument::fromJson(configFile.readAll());
            return document.isObject() ? document.object() : QJsonObject{};
        }

        DiagnosticsOptions loadDiagnosticsOptions() {
            DiagnosticsOptions options;
            const QJsonObject root = loadStoredConfiguration();
            const QJsonObject diagnosticsObject = root.value(QStringLiteral("diagnostics")).toObject();

            options.logLevel = logLevelFromString(diagnosticsObject.value(QStringLiteral("logLevel")).toString());
            options.persistentLogging = diagnosticsObject.contains(QStringLiteral("persistentLogs"))
                                            ? diagnosticsObject.value(QStringLiteral("persistentLogs")).toBool(true)
                                            : true;
            options.interactionHistoryEnabled = diagnosticsObject.contains(QStringLiteral("interactionHistory"))
                                                   ? diagnosticsObject.value(QStringLiteral("interactionHistory")).toBool(true)
                                                   : true;

            const QString email = diagnosticsObject.value(QStringLiteral("email")).toString().trimmed();
            if (!email.isEmpty()) {
                options.email = email;
            }

            const QString subject = diagnosticsObject.value(QStringLiteral("subject")).toString().trimmed();
            if (!subject.isEmpty()) {
                options.subject = subject;
            }

            return options;
        }

        QString currentTimestamp(const QString &level, const QString &message, const QMessageLogContext &context = QMessageLogContext()) {
            QString formatted = QStringLiteral("%1 [%2] %3")
                                    .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), level, message);
            if (context.file && *context.file) {
                formatted.append(QStringLiteral(" (%1:%2)")
                                     .arg(QString::fromUtf8(context.file))
                                     .arg(context.line));
            }
            return formatted;
        }

        int severityForType(const QtMsgType type) {
            switch (type) {
                case QtFatalMsg:
                case QtCriticalMsg:
                    return 0;
                case QtWarningMsg:
                    return 1;
                case QtInfoMsg:
                    return 2;
                case QtDebugMsg:
                    return 3;
            }
            return 3;
        }

        bool shouldLogMessage(const QtMsgType type) {
            switch (g_diagnosticsOptions.logLevel) {
                case LogLevel::Off:
                    return false;
                case LogLevel::Critical:
                    return severityForType(type) <= 0;
                case LogLevel::Warning:
                    return severityForType(type) <= 1;
                case LogLevel::Info:
                    return severityForType(type) <= 2;
                case LogLevel::Debug:
                case LogLevel::Full:
                    return true;
            }
            return false;
        }

        void writeFormattedLine(const QString &formatted) {
            QTextStream(stderr) << formatted << Qt::endl;
            if (g_diagnosticsOptions.persistentLogging && g_runLogFile && g_runLogFile->isOpen()) {
                QTextStream stream(g_runLogFile);
                stream << formatted << Qt::endl;
                stream.flush();
            }
        }

        void writeInteractionLine(const QJsonObject &payload) {
            if (!g_diagnosticsOptions.interactionHistoryEnabled || !g_runInteractionFile || !g_runInteractionFile->isOpen()) {
                return;
            }

            QJsonObject enrichedPayload = payload;
            enrichedPayload.insert(QStringLiteral("runId"), g_currentRunId);
            if (!enrichedPayload.contains(QStringLiteral("ts"))) {
                enrichedPayload.insert(QStringLiteral("ts"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            }

            QTextStream stream(g_runInteractionFile);
            stream << QString::fromUtf8(QJsonDocument(enrichedPayload).toJson(QJsonDocument::Compact)) << Qt::endl;
            stream.flush();
        }

        void writeLifecycleLine(const QString &level, const QString &message) {
            QMutexLocker locker(&g_logMutex);
            writeFormattedLine(currentTimestamp(level, message));
        }

        void diagnosticsMessageHandler(const QtMsgType type, const QMessageLogContext &context, const QString &message) {
            if (!shouldLogMessage(type)) {
                if (type == QtFatalMsg) {
                    writeLifecycleLine(QStringLiteral("FATAL"), message);
                    abort();
                }
                return;
            }

            const QString level = [&]() {
                switch (type) {
                    case QtDebugMsg: return QStringLiteral("DEBUG");
                    case QtInfoMsg: return QStringLiteral("INFO");
                    case QtWarningMsg: return QStringLiteral("WARN");
                    case QtCriticalMsg: return QStringLiteral("CRIT");
                    case QtFatalMsg: return QStringLiteral("FATAL");
                }
                return QStringLiteral("LOG");
            }();

            {
                QMutexLocker locker(&g_logMutex);
                writeFormattedLine(currentTimestamp(level, message, context));
            }

            if (type == QtFatalMsg) {
                abort();
            }
        }

        PreviousRunState loadPreviousRunState() {
            QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
            PreviousRunState state;
            state.startUtc = settings.value(diagnosticsStateKey(QStringLiteral("lastRunStartUtc"))).toString();
            state.logPath = settings.value(diagnosticsStateKey(QStringLiteral("lastRunLogPath"))).toString();
            state.interactionPath = settings.value(diagnosticsStateKey(QStringLiteral("lastRunInteractionPath"))).toString();
            state.graceful = settings.value(diagnosticsStateKey(QStringLiteral("lastRunGraceful")), true).toBool();
            state.valid = !state.startUtc.trimmed().isEmpty();
            return state;
        }

        QString sanitizeRunId(const QDateTime &dateTimeUtc) {
            return dateTimeUtc.toUTC().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz"));
        }

        void persistCurrentRunState(const bool graceful) {
            QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
            settings.setValue(diagnosticsStateKey(QStringLiteral("lastRunStartUtc")), g_currentRunStartUtc);
            settings.setValue(diagnosticsStateKey(QStringLiteral("lastRunId")), g_currentRunId);
            settings.setValue(diagnosticsStateKey(QStringLiteral("lastRunLogPath")), g_currentRunLogPath);
            settings.setValue(diagnosticsStateKey(QStringLiteral("lastRunInteractionPath")), g_currentRunInteractionPath);
            settings.setValue(diagnosticsStateKey(QStringLiteral("lastRunGraceful")), graceful);
            settings.sync();
        }

        QString readTextFile(const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return {};
            }
            return QString::fromUtf8(file.readAll());
        }

        QString hardwareSummary() {
            QStringList lines;
            lines << QStringLiteral("Platform: %1").arg(QGuiApplication::platformName())
                  << QStringLiteral("OS: %1").arg(QSysInfo::prettyProductName())
                  << QStringLiteral("Kernel: %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion())
                  << QStringLiteral("CPU architecture: %1").arg(QSysInfo::currentCpuArchitecture())
                  << QStringLiteral("Build ABI: %1").arg(QSysInfo::buildAbi())
                  << QStringLiteral("Host name: %1").arg(QSysInfo::machineHostName())
                  << QStringLiteral("CPU cores: %1").arg(QThread::idealThreadCount())
                  << QStringLiteral("Qt version: %1").arg(QString::fromLatin1(qVersion()))
                  << QStringLiteral("FairWindSK version: %1").arg(QStringLiteral(FAIRWINDSK_VERSION));

            if (const auto *screen = QGuiApplication::primaryScreen()) {
                const QSize size = screen->size();
                lines << QStringLiteral("Primary screen: %1x%2 @ %3")
                             .arg(size.width())
                             .arg(size.height())
                             .arg(screen->devicePixelRatio(), 0, 'f', 2);
            }

            return lines.join(u'\n');
        }

        QString buildDiagnosticsReport(const PreviousRunState &previousRun, const QString &currentRunStartUtc) {
            QStringList sections;
            sections << QStringLiteral("FairWindSK diagnostics")
                     << QStringLiteral("======================")
                     << QStringLiteral("Current start (UTC): %1").arg(currentRunStartUtc)
                     << QStringLiteral("Previous start (UTC): %1").arg(previousRun.startUtc)
                     << QStringLiteral("Previous run graceful: %1").arg(previousRun.graceful ? QStringLiteral("yes") : QStringLiteral("no"))
                     << QStringLiteral("Previous run log path: %1").arg(previousRun.logPath)
                     << QStringLiteral("Previous interaction path: %1").arg(previousRun.interactionPath)
                     << QString()
                     << QStringLiteral("Environment")
                     << QStringLiteral("-----------")
                     << hardwareSummary()
                     << QString()
                     << QStringLiteral("Previous run log")
                     << QStringLiteral("----------------");

            const QString previousLog = readTextFile(previousRun.logPath);
            if (previousLog.trimmed().isEmpty()) {
                sections << QStringLiteral("Unavailable");
            } else {
                sections << previousLog.trimmed();
            }

            return sections.join(u'\n');
        }

        QString reportDirectoryPath(const QString &reportId) {
            const QString path = QDir(reportsDirectoryPath()).filePath(reportId);
            QDir().mkpath(path);
            return path;
        }

        bool copyFileReplacing(const QString &sourcePath, const QString &targetPath) {
            if (sourcePath.trimmed().isEmpty() || !QFileInfo::exists(sourcePath)) {
                return false;
            }

            QFile::remove(targetPath);
            return QFile::copy(sourcePath, targetPath);
        }

        QStringList summarizeInteractionHistory(const QString &path, const int maxEntries = 12) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return {};
            }

            QStringList entries;
            while (!file.atEnd()) {
                const QByteArray rawLine = file.readLine();
                const QJsonDocument document = QJsonDocument::fromJson(rawLine);
                if (!document.isObject()) {
                    continue;
                }

                const QJsonObject object = document.object();
                const QString timestamp = object.value(QStringLiteral("ts")).toString();
                const QString category = object.value(QStringLiteral("category")).toString();
                const QString action = object.value(QStringLiteral("action")).toString();
                const QString target = object.value(QStringLiteral("target")).toString();
                QString summary = QStringLiteral("%1  %2/%3").arg(timestamp, category, action);
                if (!target.trimmed().isEmpty()) {
                    summary.append(QStringLiteral("  %1").arg(target));
                }
                entries.append(summary);
            }

            if (entries.size() > maxEntries) {
                entries = entries.mid(entries.size() - maxEntries);
            }
            return entries;
        }

        QString buildInteractionSummaryText(const QString &path) {
            const QStringList entries = summarizeInteractionHistory(path);
            if (entries.isEmpty()) {
                return QStringLiteral("No interaction history is available for the previous run.");
            }

            QStringList lines;
            lines << QStringLiteral("Recent operator interactions before the previous shutdown:")
                  << QStringLiteral("-------------------------------------------------------");
            lines.append(entries);
            return lines.join(u'\n');
        }

        QString writeTextFile(const QString &path, const QString &content) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QTextStream stream(&file);
                stream << content;
                stream.flush();
            }
            return path;
        }

        QString writeDiagnosticsReportBundle(const PreviousRunState &previousRun,
                                             const DiagnosticsOptions &options,
                                             const QString &currentRunId,
                                             const QString &currentRunStartUtc,
                                             QString *reportTextOut = nullptr) {
            const QString reportId = QStringLiteral("%1-unclean-run").arg(currentRunId);
            const QString bundlePath = reportDirectoryPath(reportId);
            const QString reportText = buildDiagnosticsReport(previousRun, currentRunStartUtc);
            const QString interactionSummary = buildInteractionSummaryText(previousRun.interactionPath);
            const QString combinedReportText = reportText + QStringLiteral("\n\n") + interactionSummary
                                               + QStringLiteral("\n\nBundle directory:\n%1\n").arg(bundlePath);

            writeTextFile(QDir(bundlePath).filePath(QStringLiteral("report.txt")), combinedReportText);
            writeTextFile(QDir(reportsDirectoryPath()).filePath(QStringLiteral("%1.txt").arg(reportId)), combinedReportText);

            copyFileReplacing(previousRun.logPath,
                              QDir(bundlePath).filePath(QStringLiteral("previous-run.log")));
            copyFileReplacing(previousRun.interactionPath,
                              QDir(bundlePath).filePath(QStringLiteral("previous-run.events.ndjson")));

            QJsonObject manifest;
            manifest.insert(QStringLiteral("reportId"), reportId);
            manifest.insert(QStringLiteral("status"), QStringLiteral("pending-review"));
            manifest.insert(QStringLiteral("createdAtUtc"), currentRunStartUtc);
            manifest.insert(QStringLiteral("currentRunId"), currentRunId);
            manifest.insert(QStringLiteral("previousRunStartUtc"), previousRun.startUtc);
            manifest.insert(QStringLiteral("previousRunGraceful"), previousRun.graceful);
            manifest.insert(QStringLiteral("previousRunLogPath"), previousRun.logPath);
            manifest.insert(QStringLiteral("previousRunInteractionPath"), previousRun.interactionPath);
            manifest.insert(QStringLiteral("logLevel"), logLevelToString(options.logLevel));
            manifest.insert(QStringLiteral("persistentLogs"), options.persistentLogging);
            manifest.insert(QStringLiteral("interactionHistory"), options.interactionHistoryEnabled);
            manifest.insert(QStringLiteral("fallbackEmail"), options.email);
            manifest.insert(QStringLiteral("subject"), options.subject);
            manifest.insert(QStringLiteral("summary"), interactionSummary);
            manifest.insert(QStringLiteral("bundlePath"), bundlePath);

            QJsonArray recentInteractions;
            for (const QString &entry : summarizeInteractionHistory(previousRun.interactionPath)) {
                recentInteractions.append(entry);
            }
            manifest.insert(QStringLiteral("recentInteractions"), recentInteractions);
            manifest.insert(QStringLiteral("environment"), hardwareSummary());

            writeTextFile(QDir(bundlePath).filePath(QStringLiteral("report.json")),
                          QString::fromUtf8(QJsonDocument(manifest).toJson(QJsonDocument::Indented)));

            if (reportTextOut) {
                *reportTextOut = combinedReportText;
            }

            return bundlePath;
        }

    }

    QString defaultDiagnosticsEmail() {
        return QStringLiteral("hpsclab@uniparthenope.it");
    }

    QString defaultDiagnosticsSubject() {
        return QStringLiteral("FairWindSK diagnostics");
    }

    QString persistentLogsDirectoryPath() {
        return runsDirectoryPath();
    }

    QString persistentReportsDirectoryPath() {
        return reportsDirectoryPath();
    }

    QString logLevelToString(const LogLevel level) {
        switch (level) {
            case LogLevel::Off: return QStringLiteral("off");
            case LogLevel::Critical: return QStringLiteral("critical");
            case LogLevel::Warning: return QStringLiteral("warning");
            case LogLevel::Info: return QStringLiteral("info");
            case LogLevel::Debug: return QStringLiteral("debug");
            case LogLevel::Full: return QStringLiteral("full");
        }
        return QStringLiteral("off");
    }

    QString logLevelDisplayName(const LogLevel level) {
        switch (level) {
            case LogLevel::Off: return QObject::tr("No logging");
            case LogLevel::Critical: return QObject::tr("Critical");
            case LogLevel::Warning: return QObject::tr("Warning");
            case LogLevel::Info: return QObject::tr("Info");
            case LogLevel::Debug: return QObject::tr("Debug");
            case LogLevel::Full: return QObject::tr("Full");
        }
        return QObject::tr("No logging");
    }

    LogLevel logLevelFromString(const QString &value) {
        const QString normalized = value.trimmed().toLower();
        if (normalized == QStringLiteral("critical")) {
            return LogLevel::Critical;
        }
        if (normalized == QStringLiteral("warning") || normalized == QStringLiteral("warn")) {
            return LogLevel::Warning;
        }
        if (normalized == QStringLiteral("info")) {
            return LogLevel::Info;
        }
        if (normalized == QStringLiteral("debug")) {
            return LogLevel::Debug;
        }
        if (normalized == QStringLiteral("full")) {
            return LogLevel::Full;
        }
        return LogLevel::Off;
    }

    void initializeDiagnostics() {
        g_diagnosticsOptions = loadDiagnosticsOptions();

        const PreviousRunState previousRun = loadPreviousRunState();
        const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
        g_currentRunId = sanitizeRunId(nowUtc);
        g_currentRunStartUtc = nowUtc.toString(Qt::ISODateWithMs);
        g_currentRunLogPath = QDir(runsDirectoryPath()).filePath(QStringLiteral("%1.log").arg(g_currentRunId));
        g_currentRunInteractionPath = QDir(runsDirectoryPath()).filePath(QStringLiteral("%1.events.ndjson").arg(g_currentRunId));

        if (g_diagnosticsOptions.persistentLogging) {
            auto *file = new QFile(g_currentRunLogPath);
            if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                g_runLogFile = file;
            } else {
                delete file;
                g_runLogFile = nullptr;
            }
        }

        if (g_diagnosticsOptions.interactionHistoryEnabled) {
            auto *file = new QFile(g_currentRunInteractionPath);
            if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                g_runInteractionFile = file;
            } else {
                delete file;
                g_runInteractionFile = nullptr;
            }
        }

        qInstallMessageHandler(diagnosticsMessageHandler);
        persistCurrentRunState(false);

        writeLifecycleLine(QStringLiteral("INFO"),
                           QStringLiteral("Startup logging initialized at \"%1\"").arg(g_currentRunLogPath));
        writeLifecycleLine(QStringLiteral("INFO"),
                           QStringLiteral("Diagnostics configuration logLevel=%1 persistentLogs=%2 interactionHistory=%3")
                               .arg(logLevelToString(g_diagnosticsOptions.logLevel),
                                    g_diagnosticsOptions.persistentLogging ? QStringLiteral("true") : QStringLiteral("false"),
                                    g_diagnosticsOptions.interactionHistoryEnabled ? QStringLiteral("true") : QStringLiteral("false")));
        recordUserInteraction(QStringLiteral("lifecycle"),
                              QStringLiteral("startup"),
                              QStringLiteral("FairWindSK"),
                              QJsonObject{
                                  {QStringLiteral("logPath"), g_currentRunLogPath},
                                  {QStringLiteral("interactionPath"), g_currentRunInteractionPath}
                              });

        if (previousRun.valid && !previousRun.graceful) {
            g_pendingReport.pending = true;
            g_pendingReport.currentRunId = g_currentRunId;
            g_pendingReport.previousRun = previousRun;
            g_pendingReport.options = g_diagnosticsOptions;
            writeLifecycleLine(QStringLiteral("WARN"),
                               QStringLiteral("Previous run did not end gracefully; diagnostics report bundle is queued."));
        } else {
            g_pendingReport = {};
        }
    }

    void dispatchPendingDiagnosticsReport() {
        if (!g_pendingReport.pending) {
            return;
        }

        const QString currentRunStartUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        const QString reportPath = writeDiagnosticsReportBundle(g_pendingReport.previousRun,
                                                                g_pendingReport.options,
                                                                g_pendingReport.currentRunId,
                                                                currentRunStartUtc);

        writeLifecycleLine(QStringLiteral("WARN"),
                           QStringLiteral("Stored crash report bundle at \"%1\"").arg(reportPath));
        recordUserInteraction(QStringLiteral("diagnostics"),
                              QStringLiteral("report_bundle_created"),
                              reportPath,
                              QJsonObject{
                                  {QStringLiteral("previousRunStartUtc"), g_pendingReport.previousRun.startUtc},
                                  {QStringLiteral("fallbackEmail"), g_pendingReport.options.email}
                              });

        if (!g_pendingReport.options.email.trimmed().isEmpty()) {
            writeLifecycleLine(QStringLiteral("INFO"),
                               QStringLiteral("Fallback diagnostics email target is %1; single-window mode stored the report locally at \"%2\".")
                                   .arg(g_pendingReport.options.email, reportPath));
        }

        g_pendingReport.pending = false;
    }

    void markGracefulShutdown() {
        recordUserInteraction(QStringLiteral("lifecycle"), QStringLiteral("graceful_shutdown"), QStringLiteral("FairWindSK"));
        writeLifecycleLine(QStringLiteral("INFO"), QStringLiteral("Application is closing gracefully."));
        persistCurrentRunState(true);
        if (g_runLogFile) {
            g_runLogFile->flush();
            g_runLogFile->close();
            delete g_runLogFile;
            g_runLogFile = nullptr;
        }
        if (g_runInteractionFile) {
            g_runInteractionFile->flush();
            g_runInteractionFile->close();
            delete g_runInteractionFile;
            g_runInteractionFile = nullptr;
        }
    }

    void applyLiveSettings(const LogLevel level, const bool persistentLogging, const bool interactionHistoryEnabled) {
        QMutexLocker locker(&g_logMutex);
        const bool changed = g_diagnosticsOptions.logLevel != level
                             || g_diagnosticsOptions.persistentLogging != persistentLogging
                             || g_diagnosticsOptions.interactionHistoryEnabled != interactionHistoryEnabled;
        g_diagnosticsOptions.logLevel = level;
        g_diagnosticsOptions.persistentLogging = persistentLogging;
        g_diagnosticsOptions.interactionHistoryEnabled = interactionHistoryEnabled;

        if (g_diagnosticsOptions.persistentLogging && !g_runLogFile) {
            auto *file = new QFile(g_currentRunLogPath);
            if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                g_runLogFile = file;
            } else {
                delete file;
            }
        } else if (!g_diagnosticsOptions.persistentLogging && g_runLogFile) {
            g_runLogFile->flush();
            g_runLogFile->close();
            delete g_runLogFile;
            g_runLogFile = nullptr;
        }

        if (g_diagnosticsOptions.interactionHistoryEnabled && !g_runInteractionFile) {
            auto *file = new QFile(g_currentRunInteractionPath);
            if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                g_runInteractionFile = file;
            } else {
                delete file;
            }
        } else if (!g_diagnosticsOptions.interactionHistoryEnabled && g_runInteractionFile) {
            g_runInteractionFile->flush();
            g_runInteractionFile->close();
            delete g_runInteractionFile;
            g_runInteractionFile = nullptr;
        }

        if (changed) {
            writeFormattedLine(currentTimestamp(QStringLiteral("INFO"),
                                                QStringLiteral("Logging preferences updated logLevel=%1 persistentLogs=%2 interactionHistory=%3")
                                                    .arg(logLevelToString(level),
                                                         persistentLogging ? QStringLiteral("true") : QStringLiteral("false"),
                                                         interactionHistoryEnabled ? QStringLiteral("true") : QStringLiteral("false"))));
        }
    }

    void recordUserInteraction(const QString &category,
                               const QString &action,
                               const QString &target,
                               const QJsonObject &context) {
        if (category.trimmed().isEmpty() || action.trimmed().isEmpty()) {
            return;
        }

        QMutexLocker locker(&g_logMutex);
        QJsonObject payload = context;
        payload.insert(QStringLiteral("category"), category);
        payload.insert(QStringLiteral("action"), action);
        if (!target.trimmed().isEmpty()) {
            payload.insert(QStringLiteral("target"), target);
        }
        writeInteractionLine(payload);
    }
}
