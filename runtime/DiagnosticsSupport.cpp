//
// Created by Codex on 07/04/26.
//

#include "runtime/DiagnosticsSupport.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
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
#include <QUrlQuery>

#include "Configuration.hpp"

using namespace Qt::StringLiterals;

namespace fairwindsk::runtime {
    namespace {
        struct DiagnosticsOptions {
            LogLevel logLevel = LogLevel::Off;
            bool persistentLogging = true;
            QString email = defaultDiagnosticsEmail();
            QString subject = defaultDiagnosticsSubject();
        };

        struct PreviousRunState {
            bool valid = false;
            bool graceful = true;
            QString startUtc;
            QString logPath;
        };

        struct PendingDiagnosticsEmail {
            bool pending = false;
            QString currentRunId;
            PreviousRunState previousRun;
            DiagnosticsOptions options;
        };

        QMutex g_logMutex;
        QFile *g_runLogFile = nullptr;
        DiagnosticsOptions g_diagnosticsOptions;
        PendingDiagnosticsEmail g_pendingEmail;
        QString g_currentRunId;
        QString g_currentRunLogPath;
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

        QString writeDiagnosticsReport(const QString &reportText, const QString &currentRunId) {
            const QString reportPath = QDir(reportsDirectoryPath()).filePath(QStringLiteral("%1-unclean-run.txt").arg(currentRunId));
            QFile file(reportPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QTextStream stream(&file);
                stream << reportText;
                stream.flush();
            }
            return reportPath;
        }

        QString condensedEmailBody(const QString &reportText, const QString &reportPath) {
            constexpr int kMaxBodyLength = 6000;
            QString body = reportText;
            if (!reportPath.trimmed().isEmpty()) {
                body.append(QStringLiteral("\n\nFull report saved at:\n%1").arg(reportPath));
            }
            if (body.size() > kMaxBodyLength) {
                body = body.left(kMaxBodyLength - 64)
                       + QStringLiteral("\n\n[truncated]\nFull report saved at:\n%1").arg(reportPath);
            }
            return body;
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

        if (g_diagnosticsOptions.persistentLogging) {
            auto *file = new QFile(g_currentRunLogPath);
            if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                g_runLogFile = file;
            } else {
                delete file;
                g_runLogFile = nullptr;
            }
        }

        qInstallMessageHandler(diagnosticsMessageHandler);
        persistCurrentRunState(false);

        writeLifecycleLine(QStringLiteral("INFO"),
                           QStringLiteral("Startup logging initialized at \"%1\"").arg(g_currentRunLogPath));
        writeLifecycleLine(QStringLiteral("INFO"),
                           QStringLiteral("Diagnostics configuration logLevel=%1 persistentLogs=%2")
                               .arg(logLevelToString(g_diagnosticsOptions.logLevel),
                                    g_diagnosticsOptions.persistentLogging ? QStringLiteral("true") : QStringLiteral("false")));

        if (previousRun.valid && !previousRun.graceful) {
            g_pendingEmail.pending = true;
            g_pendingEmail.currentRunId = g_currentRunId;
            g_pendingEmail.previousRun = previousRun;
            g_pendingEmail.options = g_diagnosticsOptions;
            writeLifecycleLine(QStringLiteral("WARN"),
                               QStringLiteral("Previous run did not end gracefully; diagnostics email is queued."));
        } else {
            g_pendingEmail = {};
        }
    }

    void dispatchPendingDiagnosticsEmail() {
        if (!g_pendingEmail.pending || g_pendingEmail.options.email.trimmed().isEmpty()) {
            return;
        }

        const QString currentRunStartUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        const QString reportText = buildDiagnosticsReport(g_pendingEmail.previousRun, currentRunStartUtc);
        const QString reportPath = writeDiagnosticsReport(reportText, g_pendingEmail.currentRunId);

        QUrl mailUrl;
        mailUrl.setScheme(QStringLiteral("mailto"));
        mailUrl.setPath(g_pendingEmail.options.email);
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("subject"), g_pendingEmail.options.subject);
        query.addQueryItem(QStringLiteral("body"), condensedEmailBody(reportText, reportPath));
        mailUrl.setQuery(query);

        const bool opened = QDesktopServices::openUrl(mailUrl);
        writeLifecycleLine(opened ? QStringLiteral("INFO") : QStringLiteral("WARN"),
                           opened
                               ? QStringLiteral("Diagnostics email composer opened for %1").arg(g_pendingEmail.options.email)
                               : QStringLiteral("Unable to open diagnostics email composer for %1").arg(g_pendingEmail.options.email));
        g_pendingEmail.pending = false;
    }

    void markGracefulShutdown() {
        writeLifecycleLine(QStringLiteral("INFO"), QStringLiteral("Application is closing gracefully."));
        persistCurrentRunState(true);
        if (g_runLogFile) {
            g_runLogFile->flush();
            g_runLogFile->close();
            delete g_runLogFile;
            g_runLogFile = nullptr;
        }
    }

    void applyLiveSettings(const LogLevel level, const bool persistentLogging) {
        QMutexLocker locker(&g_logMutex);
        const bool changed = g_diagnosticsOptions.logLevel != level
                             || g_diagnosticsOptions.persistentLogging != persistentLogging;
        g_diagnosticsOptions.logLevel = level;
        g_diagnosticsOptions.persistentLogging = persistentLogging;

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

        if (changed) {
            writeFormattedLine(currentTimestamp(QStringLiteral("INFO"),
                                                QStringLiteral("Logging preferences updated logLevel=%1 persistentLogs=%2")
                                                    .arg(logLevelToString(level),
                                                         persistentLogging ? QStringLiteral("true") : QStringLiteral("false"))));
        }
    }
}
