#include <QApplication>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QSplashScreen>
#endif
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QSettings>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"


using namespace Qt::StringLiterals;

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

    // Enable OpenGL shared contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts );

#if defined(Q_OS_LINUX)
    configureLinuxRuntimeDirectory();
    configureLinuxQtPlatformFallback();
    configureLinuxWebEngineFallback();
#endif

    // The QT application
    QApplication app(argc, argv);

    // Install the translator
    QApplication::installTranslator(&translator);

    // Set the window icon
    QApplication::setWindowIcon(QIcon(QPixmap::fromImage(QImage(":/resources/images/mainwindow/fairwind_icon.png"))));

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

    // Load the configuration inside the FairWind singleton itself
    fairWindSK->loadConfig();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#endif

    // Set web profile options
    // Create a new MainWindow object
    fairwindsk::ui::MainWindow w;

    if (auto *profile = fairWindSK->getWebEngineProfile()) {
        profile->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
        profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Close the splash screen presenting the MainWindow UI
    splash.finish((QWidget *) &w);
#endif

    QTimer::singleShot(0, &w, [fairWindSK]() {
        fairWindSK->startSignalK();
        fairWindSK->loadApps();
        if (auto *mainWindow = fairwindsk::ui::MainWindow::instance()) {
            mainWindow->applyRuntimeConfiguration();
        }
    });

    // Run the application. Let Qt tear down the QObject tree on process exit so
    // the shared WebEngine profile is not destroyed before the remaining pages.
    return QApplication::exec();
}
