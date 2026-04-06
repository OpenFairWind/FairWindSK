#include <QApplication>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QSplashScreen>
#endif
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTimer>
#include <QTextStream>
#include <QTranslator>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QSettings>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"


using namespace Qt::StringLiterals;

namespace {
    QMutex g_startupLogMutex;
    QFile *g_startupLogFile = nullptr;

    QString startupLogFilePath() {
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (logDir.trimmed().isEmpty()) {
            logDir = QDir::homePath();
        }

        QDir().mkpath(logDir);
        return QDir(logDir).filePath(QStringLiteral("startup.log"));
    }

    void fairWindMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
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

        QString formatted = QStringLiteral("%1 [%2] %3")
                .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), level, message);
        if (context.file && *context.file) {
            formatted.append(QStringLiteral(" (%1:%2)").arg(QString::fromUtf8(context.file)).arg(context.line));
        }

        {
            QMutexLocker locker(&g_startupLogMutex);
            QTextStream(stderr) << formatted << Qt::endl;
            if (g_startupLogFile && g_startupLogFile->isOpen()) {
                QTextStream stream(g_startupLogFile);
                stream << formatted << Qt::endl;
                stream.flush();
            }
        }

        if (type == QtFatalMsg) {
            abort();
        }
    }

    void initializeStartupLogging() {
        if (g_startupLogFile) {
            return;
        }

        auto *logFile = new QFile(startupLogFilePath());
        if (logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            g_startupLogFile = logFile;
        } else {
            delete logFile;
        }

        qInstallMessageHandler(fairWindMessageHandler);
        qInfo() << "Startup logging initialized at" << startupLogFilePath();
    }
}

#if defined(Q_OS_LINUX)
namespace {
    bool hasOwnerOnlyPermissions(const QFileInfo &fileInfo) {
        const QFileDevice::Permissions permissions = fileInfo.permissions();
        const QFileDevice::Permissions allowed =
                QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
        return (permissions & allowed) == allowed && (permissions & ~allowed) == QFileDevice::Permissions();
    }

    void appendChromiumFlagIfMissing(QStringList &flags, const QString &flag) {
        if (!flags.contains(flag)) {
            flags.append(flag);
        }
    }

    QString qtPlatformPluginDirectory() {
        const QStringList candidatePaths = {
            QLibraryInfo::path(QLibraryInfo::PluginsPath)
        };

        for (const QString &path : candidatePaths) {
            if (path.trimmed().isEmpty()) {
                continue;
            }

            const QString platformPath = QDir(path).filePath(QStringLiteral("platforms"));
            if (QFileInfo::exists(platformPath)) {
                return platformPath;
            }
        }

        return QString();
    }

    bool hasPlatformPlugin(const QString &platformPath, const QString &pluginBaseName) {
        if (platformPath.trimmed().isEmpty() || pluginBaseName.trimmed().isEmpty()) {
            return false;
        }

        const QStringList candidates = {
            QDir(platformPath).filePath(QStringLiteral("libq%1.so").arg(pluginBaseName)),
            QDir(platformPath).filePath(QStringLiteral("q%1.dll").arg(pluginBaseName)),
            QDir(platformPath).filePath(QStringLiteral("q%1.dylib").arg(pluginBaseName))
        };

        return std::any_of(candidates.cbegin(), candidates.cend(), [](const QString &candidate) {
            return QFileInfo::exists(candidate);
        });
    }

    void configureLinuxQtPlatformFallback() {
        if (!qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            return;
        }

        const QString platformPath = qtPlatformPluginDirectory();
        const bool hasWayland = hasPlatformPlugin(platformPath, QStringLiteral("wayland"));
        const bool hasXcb = hasPlatformPlugin(platformPath, QStringLiteral("xcb"));
        const bool hasEglfs = hasPlatformPlugin(platformPath, QStringLiteral("eglfs"));
        const bool hasLinuxFb = hasPlatformPlugin(platformPath, QStringLiteral("linuxfb"));

        const QByteArray sessionType = qgetenv("XDG_SESSION_TYPE").trimmed().toLower();
        const bool waylandSession = sessionType == "wayland";

        if (waylandSession && !hasWayland) {
            if (hasXcb) {
                qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
            } else if (hasEglfs) {
                qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("eglfs"));
            } else if (hasLinuxFb) {
                qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("linuxfb"));
            }
            return;
        }

        if (!waylandSession && hasXcb) {
            qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
        }
    }

    void configureLinuxRuntimeDirectory() {
        const QString configuredRuntimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR").trimmed();
        QFileInfo runtimeInfo(configuredRuntimeDir);
        if (!configuredRuntimeDir.isEmpty() && runtimeInfo.exists() && runtimeInfo.isDir() && hasOwnerOnlyPermissions(runtimeInfo)) {
            return;
        }

        QString fallbackBase = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (fallbackBase.trimmed().isEmpty()) {
            fallbackBase = QDir::tempPath();
        }

        const QString userName = qEnvironmentVariable("USER").trimmed().isEmpty()
                ? QStringLiteral("user")
                : qEnvironmentVariable("USER").trimmed();
        const QString fallbackRuntimeDir = QDir(fallbackBase).filePath(QStringLiteral("fairwindsk-runtime-%1").arg(userName));
        QDir().mkpath(fallbackRuntimeDir);
        QFile::setPermissions(fallbackRuntimeDir,
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        qputenv("XDG_RUNTIME_DIR", fallbackRuntimeDir.toUtf8());
    }

    void configureLinuxWebEngineFallback() {
#if defined(__arm__) || defined(__aarch64__)
        if (qEnvironmentVariableIsEmpty("QT_OPENGL")) {
            qputenv("QT_OPENGL", QByteArrayLiteral("software"));
        }

        QStringList chromiumFlags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS")
                                        .split(u' ', Qt::SkipEmptyParts);
        appendChromiumFlagIfMissing(chromiumFlags, QStringLiteral("--disable-gpu"));
        appendChromiumFlagIfMissing(chromiumFlags, QStringLiteral("--disable-gpu-compositing"));
        appendChromiumFlagIfMissing(chromiumFlags, QStringLiteral("--disable-gpu-rasterization"));
        appendChromiumFlagIfMissing(chromiumFlags, QStringLiteral("--disable-features=Vulkan"));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags.join(u' ').toUtf8());
#endif
    }
}
#endif

int main(int argc, char *argv[]) {

    // The translator
    QTranslator translator;

    // Set the organization name
    QCoreApplication::setOrganizationName("uniparthenope.it");
    QCoreApplication::setApplicationName("FairWindSK");
    initializeStartupLogging();
    qInfo() << "FairWindSK bootstrap start";
    qInfo() << "argc=" << argc;
    for (int index = 0; index < argc; ++index) {
        qInfo() << "argv[" << index << "]=" << (argv[index] ? argv[index] : "<null>");
    }

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );
    qInfo() << "AA_ShareOpenGLContexts enabled";

#if defined(Q_OS_LINUX)
    configureLinuxRuntimeDirectory();
    configureLinuxQtPlatformFallback();
    configureLinuxWebEngineFallback();
    qInfo() << "Linux startup environment"
            << "XDG_SESSION_TYPE=" << qEnvironmentVariable("XDG_SESSION_TYPE")
            << "XDG_RUNTIME_DIR=" << qEnvironmentVariable("XDG_RUNTIME_DIR")
            << "QT_QPA_PLATFORM=" << qEnvironmentVariable("QT_QPA_PLATFORM")
            << "QT_OPENGL=" << qEnvironmentVariable("QT_OPENGL")
            << "QTWEBENGINE_CHROMIUM_FLAGS=" << qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS");
#endif

    // The QT application
    QApplication app(argc, argv);
    qInfo() << "QApplication created";

    // Install the translator
    QApplication::installTranslator(&translator);
    qInfo() << "Translator installed";

    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png"))));
    qInfo() << "Application icon set";

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Get the splash screen logo
    const QPixmap pixmap(":/resources/images/other/splash_logo.png");

    // Create a splash screen containing the logo
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint);

    // Show the logo
    splash.show();

    // Show message
    splash.showMessage(QObject::tr("Welcome to FairWindSK a GUI for the Signal K server!"), 500, Qt::white);
#endif

    // Get the FairWind singleton
    const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
    qInfo() << "FairWindSK singleton created";

    // Load the configuration inside the FairWind singleton itself
    fairWindSK->loadConfig();
    qInfo() << "Configuration loaded";

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#endif

    // Set web profile options
    // Create a new MainWindow object
    fairwindsk::ui::MainWindow w;
    qInfo() << "MainWindow created";

    if (auto *profile = fairWindSK->getWebEngineProfile()) {
        profile->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
        qInfo() << "WebEngine profile configured";
    } else {
        qWarning() << "WebEngine profile is null during startup";
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Close the splash screen presenting the MainWindow UI
    qInfo() << "About to finish splash screen";
    splash.finish((QWidget *) &w);
    qInfo() << "Splash screen finished";
#endif

    qInfo() << "Scheduling deferred startup tasks";
    QTimer::singleShot(0, &w, [fairWindSK]() {
        qInfo() << "Deferred startup tasks begin";
        fairWindSK->startSignalK();
        qInfo() << "Signal K startup completed";
        fairWindSK->loadApps();
        qInfo() << "App loading completed";
        if (auto *mainWindow = fairwindsk::ui::MainWindow::instance()) {
            mainWindow->applyRuntimeConfiguration();
            qInfo() << "Runtime configuration applied";
        }
    });
    qInfo() << "Deferred startup tasks scheduled";

    // Run the application. Let Qt tear down the QObject tree on process exit so
    // the shared WebEngine profile is not destroyed before the remaining pages.
    qInfo() << "Entering QApplication event loop";
    return QApplication::exec();
}
