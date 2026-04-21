#include <QCoreApplication>
#include <QApplication>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QSplashScreen>
#endif
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#else
#include <QtWebView/QtWebView>
#endif

#include "FairWindSK.hpp"
#include "Configuration.hpp"
#include "Units.hpp"
#include "runtime/DiagnosticsSupport.hpp"
#include "ui/MainWindow.hpp"

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

namespace {
    bool hasArgument(const int argc, char *argv[], const char *argument) {
        if (!argument) {
            return false;
        }

        const QString expected = QString::fromUtf8(argument);
        for (int index = 1; index < argc; ++index) {
            if (argv[index] && QString::fromUtf8(argv[index]) == expected) {
                return true;
            }
        }

        return false;
    }

    int runUnitsReentrancySelfTest(int argc, char *argv[]) {
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName("uniparthenope.it");
        QCoreApplication::setApplicationName("FairWindSK");

        QString failureReason;
        const bool passed = fairwindsk::Units::runSignalKLookupRegressionSelfTest(&failureReason);
        if (passed) {
            qInfo() << "Units re-entrancy self-test passed";
            return 0;
        }

        qCritical() << "Units re-entrancy self-test failed:" << failureReason;
        return 1;
    }

    QString resolveConfigurationPathFromSettings() {
        const QString settingsPath = fairwindsk::Configuration::settingsFilename();
        QSettings settings(settingsPath, QSettings::IniFormat);
        const QString configuredPath = settings.value(QStringLiteral("General/config")).toString().trimmed();
        if (!configuredPath.isEmpty()) {
            const QFileInfo configuredInfo(configuredPath);
            if (configuredInfo.isAbsolute()) {
                return configuredInfo.absoluteFilePath();
            }
            return QDir(QFileInfo(settingsPath).absolutePath()).filePath(configuredPath);
        }

        return QDir(QFileInfo(settingsPath).absolutePath()).filePath(QStringLiteral("fairwindsk.json"));
    }

    bool shouldEnableVirtualKeyboardAtStartup(QString *configurationPath = nullptr) {
        const QString resolvedConfigurationPath = resolveConfigurationPathFromSettings();
        if (configurationPath) {
            *configurationPath = resolvedConfigurationPath;
        }

        fairwindsk::Configuration configuration(resolvedConfigurationPath);
        if (configuration.getFilename().isEmpty()) {
            return false;
        }
        return configuration.getVirtualKeyboard();
    }
}

int main(int argc, char *argv[]) {
    if (hasArgument(argc, argv, "--self-test-units-reentrancy")) {
        return runUnitsReentrancySelfTest(argc, argv);
    }

    // The translator
    QTranslator translator;

    // Set the organization name
    QCoreApplication::setOrganizationName("uniparthenope.it");
    QCoreApplication::setApplicationName("FairWindSK");

    QString startupConfigurationPath;
    const bool useVirtualKeyboard = shouldEnableVirtualKeyboardAtStartup(&startupConfigurationPath);
    if (useVirtualKeyboard) {
        qputenv("QT_IM_MODULE", QByteArrayLiteral("qtvirtualkeyboard"));
    }

    fairwindsk::runtime::initializeDiagnostics();
    qInfo() << "FairWindSK bootstrap start";
    qInfo() << "argc=" << argc;
    for (int index = 0; index < argc; ++index) {
        qInfo() << "argv[" << index << "]=" << (argv[index] ? argv[index] : "<null>");
    }
    qInfo() << "Startup configuration path=" << startupConfigurationPath;
    qInfo() << "QT_IM_MODULE=" << qEnvironmentVariable("QT_IM_MODULE");

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );
    qInfo() << "AA_ShareOpenGLContexts enabled";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QtWebView::initialize();
    qInfo() << "Qt WebView initialized for mobile build";
#endif

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
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        fairwindsk::runtime::markGracefulShutdown();
    });
    fairwindsk::runtime::dispatchPendingDiagnosticsReport();

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

 #if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (auto *profile = fairWindSK->getWebEngineProfile()) {
        profile->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
        qInfo() << "WebEngine profile configured";
    } else {
        qWarning() << "WebEngine profile is null during startup";
    }
#else
    qInfo() << "Mobile web profile uses Qt WebView backend";
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Close the splash screen presenting the MainWindow UI
    qInfo() << "About to close splash screen";
#if defined(Q_OS_LINUX)
    splash.hide();
    splash.close();
    qInfo() << "Splash screen closed with Linux-safe path";
#else
    splash.finish((QWidget *) &w);
    qInfo() << "Splash screen finished";
#endif
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
            mainWindow->prewarmPersistentPagesAfterStartup();
            qInfo() << "Post-startup page prewarm scheduled";
        }
    });
    qInfo() << "Deferred startup tasks scheduled";

    // Run the application. Let Qt tear down the QObject tree on process exit so
    // the shared WebEngine profile is not destroyed before the remaining pages.
    qInfo() << "Entering QApplication event loop";
    return QApplication::exec();
}
