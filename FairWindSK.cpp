//
// Created by Raffaele Montella on 03/04/21.
//
#include <QApplication>
#include <QAbstractButton>
#include <QThread>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QEventLoop>
#include <QSettings>
#include <QScreen>
#include <QToolButton>
#include <QPushButton>
#include <QStandardPaths>
#include <QFile>


#include <QLoggingCategory>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <utility>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QUrl>
#include <QPointer>
#include <QTimer>
#include <QTime>
#include <QUrl>
#include <algorithm>

#include "Units.hpp"
#include "ui/MainWindow.hpp"
#include "ui/widgets/TouchScrollArea.hpp"


using namespace Qt::StringLiterals;

namespace fairwindsk {
    namespace {
        constexpr int kAppsRequestTimeoutMs = 5000;

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

            QFileInfo fileInfo(trimmed);
            if (fileInfo.isAbsolute()) {
                QDir().mkpath(fileInfo.absolutePath());
                return fileInfo.absoluteFilePath();
            }

            return QDir(QFileInfo(defaultConfigFilename()).absolutePath()).filePath(trimmed);
        }

        struct UiMetrics {
            int fontPointSize = 12;
            int controlHeight = 32;
            int tabHeight = 32;
            int actionIconSize = 24;
            int prominentIconSize = 56;
            int toggleWidth = 46;
            int toggleHeight = 24;
            int horizontalPadding = 12;
            int verticalPadding = 6;
        };

        struct UiComfortPalette {
            QString window = QStringLiteral("#06080d");
            QString panel = QStringLiteral("#111827");
            QString base = QStringLiteral("#f8fafc");
            QString alternateBase = QStringLiteral("#dfe7f0");
            QString buttonTop = QStringLiteral("#6b7280");
            QString buttonBottom = QStringLiteral("#4b5563");
            QString buttonHoverTop = QStringLiteral("#7c8593");
            QString buttonHoverBottom = QStringLiteral("#5b6472");
            QString buttonPressedTop = QStringLiteral("#374151");
            QString buttonPressedBottom = QStringLiteral("#1f2937");
            QString accentTop = QStringLiteral("#3b82f6");
            QString accentBottom = QStringLiteral("#1d4ed8");
            QString accentBorder = QStringLiteral("#60a5fa");
            QString text = QStringLiteral("#f9fafb");
            QString subduedText = QStringLiteral("#cbd5e1");
            QString fieldText = QStringLiteral("#111827");
            QString border = QStringLiteral("#9ca3af");
            QString borderStrong = QStringLiteral("#4b5563");
            QString scrollTrack = QStringLiteral("#d7dde6");
            QString scrollHandleTop = QStringLiteral("#fdfefe");
            QString scrollHandleMid = QStringLiteral("#eef2f7");
            QString scrollHandleBottom = QStringLiteral("#bac4cf");
        };

        UiMetrics uiMetricsForPreset(const QString &preset) {
            UiMetrics metrics;

            if (preset == "small") {
                metrics.fontPointSize = 11;
                metrics.controlHeight = 28;
                metrics.tabHeight = 28;
                metrics.actionIconSize = 20;
                metrics.prominentIconSize = 40;
                metrics.toggleWidth = 40;
                metrics.toggleHeight = 22;
                metrics.horizontalPadding = 10;
                metrics.verticalPadding = 4;
            } else if (preset == "large") {
                metrics.fontPointSize = 14;
                metrics.controlHeight = 40;
                metrics.tabHeight = 40;
                metrics.actionIconSize = 32;
                metrics.prominentIconSize = 72;
                metrics.toggleWidth = 56;
                metrics.toggleHeight = 30;
                metrics.horizontalPadding = 14;
                metrics.verticalPadding = 8;
            } else if (preset == "xlarge") {
                metrics.fontPointSize = 16;
                metrics.controlHeight = 48;
                metrics.tabHeight = 48;
                metrics.actionIconSize = 40;
                metrics.prominentIconSize = 88;
                metrics.toggleWidth = 64;
                metrics.toggleHeight = 34;
                metrics.horizontalPadding = 18;
                metrics.verticalPadding = 10;
            } else {
                metrics.fontPointSize = 12;
                metrics.controlHeight = 34;
                metrics.tabHeight = 34;
                metrics.actionIconSize = 24;
                metrics.prominentIconSize = 56;
                metrics.toggleWidth = 46;
                metrics.toggleHeight = 24;
                metrics.horizontalPadding = 12;
                metrics.verticalPadding = 6;
            }

            return metrics;
        }

        QString resolvedUiScalePreset(const Configuration &configuration) {
            if (configuration.getUiScaleMode() != "auto") {
                return configuration.getUiScalePreset();
            }

            const auto *screen = QGuiApplication::primaryScreen();
            if (!screen) {
                return "normal";
            }

            const auto geometry = screen->availableGeometry();
            const auto shortestSide = std::min(geometry.width(), geometry.height());
            const auto dpi = screen->logicalDotsPerInch();

            if (shortestSide >= 1800 || dpi >= 170.0) {
                return "xlarge";
            }
            if (shortestSide >= 1200 || dpi >= 130.0) {
                return "large";
            }
            if (shortestSide <= 720) {
                return "large";
            }
            return "normal";
        }

        QString normalizedComfortViewPreset(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == "sunrise") {
                return QStringLiteral("dawn");
            }
            if (preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                return QStringLiteral("day");
            }
            return preset;
        }

        QString resolvedComfortViewPreset(const Configuration &configuration) {
            if (configuration.getComfortViewMode() != "auto") {
                return normalizedComfortViewPreset(configuration.getComfortViewPreset());
            }

            const QTime now = QTime::currentTime();
            if (now >= QTime(5, 30) && now < QTime(7, 0)) {
                return QStringLiteral("dawn");
            }
            if (now >= QTime(7, 0) && now < QTime(17, 30)) {
                return QStringLiteral("day");
            }
            if (now >= QTime(17, 30) && now < QTime(19, 0)) {
                return QStringLiteral("sunset");
            }
            if (now >= QTime(19, 0) && now < QTime(20, 30)) {
                return QStringLiteral("dusk");
            }
            return QStringLiteral("night");
        }

        QString comfortThemeResourcePath(const QString &preset) {
            return QStringLiteral(":/resources/stylesheets/%1.qss").arg(normalizedComfortViewPreset(preset));
        }

        QString loadDefaultComfortThemeStyleSheet(const QString &preset) {
            QFile file(comfortThemeResourcePath(preset));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return QString();
            }

            return QString::fromUtf8(file.readAll());
        }

        QString loadComfortThemeStyleSheet(const Configuration &configuration, const QString &preset) {
            const QString configuredStyleSheet = configuration.getComfortThemeStyleSheet(preset);
            if (!configuredStyleSheet.trimmed().isEmpty()) {
                return configuredStyleSheet;
            }

            return loadDefaultComfortThemeStyleSheet(preset);
        }

        QString loadComfortBackgroundStyleSheet(const Configuration &configuration, const QString &preset) {
            const auto buildRule = [&configuration, &preset](const QString &area, const QString &objectName) {
                const QString imagePath = configuration.getComfortBackgroundImagePath(preset, area).trimmed();
                if (imagePath.isEmpty()) {
                    return QString();
                }

                return QStringLiteral(
                    "QWidget#%1 {"
                    " border-image: url(\"%2\") 0 0 0 0 stretch stretch;"
                    " background-position: center;"
                    " }")
                    .arg(objectName, QUrl::fromLocalFile(imagePath).toString());
            };

            return buildRule(QStringLiteral("topbar"), QStringLiteral("TopBar"))
                + buildRule(QStringLiteral("launcher"), QStringLiteral("Launcher"))
                + buildRule(QStringLiteral("bottombar"), QStringLiteral("BottomBar"));
        }

        bool isAutomaticComfortViewSupported(const Configuration &configuration, signalk::Client *client) {
            const QString configuredSunPath = configuration.getSignalKPath(QStringLiteral("environment.sun")).trimmed();
            if (configuredSunPath.isEmpty() || client == nullptr || client->url().isEmpty() || !client->isRestHealthy()) {
                return false;
            }

            return !client->signalkGet(configuredSunPath).isEmpty();
        }

        UiComfortPalette uiComfortPaletteForPreset(const QString &preset) {
            UiComfortPalette palette;

            const QString normalizedPreset = normalizedComfortViewPreset(preset);

            if (normalizedPreset == "dawn") {
                palette.window = "#101a2a";
                palette.panel = "#162338";
                palette.base = "#f6e2b6";
                palette.alternateBase = "#ead2a1";
                palette.buttonTop = "#ffe3a3";
                palette.buttonBottom = "#d8b56f";
                palette.buttonHoverTop = "#ffebb7";
                palette.buttonHoverBottom = "#e2c17f";
                palette.buttonPressedTop = "#c59653";
                palette.buttonPressedBottom = "#8e6734";
                palette.accentTop = "#ffd37a";
                palette.accentBottom = "#f2a93b";
                palette.accentBorder = "#ffe7ac";
                palette.text = "#f7f3e8";
                palette.subduedText = "#d8d7d0";
                palette.fieldText = "#1f2c3a";
                palette.border = "#c7b07a";
                palette.borderStrong = "#35506d";
                palette.scrollTrack = "#d4c29d";
                palette.scrollHandleTop = "#fff0bf";
                palette.scrollHandleMid = "#edd49d";
                palette.scrollHandleBottom = "#b69056";
            } else if (normalizedPreset == "sunset") {
                palette.window = "#1b1210";
                palette.panel = "#2a1a17";
                palette.base = "#f0d2a2";
                palette.alternateBase = "#e4bb7c";
                palette.buttonTop = "#ffd39a";
                palette.buttonBottom = "#cf8d4d";
                palette.buttonHoverTop = "#ffdeaf";
                palette.buttonHoverBottom = "#daa063";
                palette.buttonPressedTop = "#c1693b";
                palette.buttonPressedBottom = "#7f3b1f";
                palette.accentTop = "#ffb45f";
                palette.accentBottom = "#ef6b2e";
                palette.accentBorder = "#ffd29d";
                palette.text = "#fff1df";
                palette.subduedText = "#e6c3aa";
                palette.fieldText = "#2b1d18";
                palette.border = "#d19965";
                palette.borderStrong = "#6f3e27";
                palette.scrollTrack = "#d1b08e";
                palette.scrollHandleTop = "#ffe4bb";
                palette.scrollHandleMid = "#e8bd8c";
                palette.scrollHandleBottom = "#ad7240";
            } else if (normalizedPreset == "dusk") {
                palette.window = "#07111d";
                palette.panel = "#0d1b2c";
                palette.base = "#bcc9d8";
                palette.alternateBase = "#96a6ba";
                palette.buttonTop = "#b7c7db";
                palette.buttonBottom = "#627a95";
                palette.buttonHoverTop = "#c7d4e4";
                palette.buttonHoverBottom = "#748ca6";
                palette.buttonPressedTop = "#4f6480";
                palette.buttonPressedBottom = "#2a3b4f";
                palette.accentTop = "#86a8d7";
                palette.accentBottom = "#4a6f9d";
                palette.accentBorder = "#b9d0ee";
                palette.text = "#e7eef7";
                palette.subduedText = "#b7c5d6";
                palette.fieldText = "#102236";
                palette.border = "#6d88a6";
                palette.borderStrong = "#38516d";
                palette.scrollTrack = "#98a7ba";
                palette.scrollHandleTop = "#d6e1ee";
                palette.scrollHandleMid = "#9eb0c6";
                palette.scrollHandleBottom = "#5c7390";
            } else if (normalizedPreset == "night") {
                palette.window = "#050304";
                palette.panel = "#120608";
                palette.base = "#1b0b0e";
                palette.alternateBase = "#2b1014";
                palette.buttonTop = "#7b2e39";
                palette.buttonBottom = "#3b0a10";
                palette.buttonHoverTop = "#96404d";
                palette.buttonHoverBottom = "#53111a";
                palette.buttonPressedTop = "#2b0b11";
                palette.buttonPressedBottom = "#120507";
                palette.accentTop = "#ff4d4d";
                palette.accentBottom = "#a61218";
                palette.accentBorder = "#ff8a8a";
                palette.text = "#ffb3b3";
                palette.subduedText = "#d78c8c";
                palette.fieldText = "#ffb3b3";
                palette.border = "#8d3741";
                palette.borderStrong = "#7f1d1d";
                palette.scrollTrack = "#341417";
                palette.scrollHandleTop = "#b84a57";
                palette.scrollHandleMid = "#822430";
                palette.scrollHandleBottom = "#501118";
            } else {
                palette.window = "#f6f2df";
                palette.panel = "#fff7dc";
                palette.base = "#fffdf5";
                palette.alternateBase = "#f4efcf";
                palette.buttonTop = "#fff9d7";
                palette.buttonBottom = "#d8ca76";
                palette.buttonHoverTop = "#fffbe4";
                palette.buttonHoverBottom = "#e4d487";
                palette.buttonPressedTop = "#c5b85c";
                palette.buttonPressedBottom = "#8c7f2b";
                palette.accentTop = "#2f5fb7";
                palette.accentBottom = "#1e4387";
                palette.accentBorder = "#4c7bd1";
                palette.text = "#111111";
                palette.subduedText = "#4a4f55";
                palette.fieldText = "#111111";
                palette.border = "#9c9160";
                palette.borderStrong = "#4f5d72";
                palette.scrollTrack = "#ece4bf";
                palette.scrollHandleTop = "#fffdf1";
                palette.scrollHandleMid = "#efe6b8";
                palette.scrollHandleBottom = "#c4b56a";
            }

            return palette;
        }

        QString buildUiMetricsStyleSheet(const UiMetrics &metrics) {
            return QString(
                       "QWidget { font-size: %1pt; }"
                       "QToolButton, QPushButton, TouchComboBox, QLineEdit, QTextEdit, QPlainTextEdit, "
                       "QSpinBox, QDoubleSpinBox, QDateTimeEdit { min-height: %2px; }"
                       "QComboBox, QAbstractSpinBox, QDateTimeEdit { padding: %3px %4px; }"
                       "QComboBox::drop-down { border: none; width: %2px; }"
                       "QToolButton, QPushButton { padding: %3px %4px; }"
                       "QToolButton:pressed, QPushButton:pressed,"
                       "QToolButton:checked, QPushButton:checked { padding-top: %5px; padding-bottom: %6px; }"
                       "QTabBar::tab {"
                       " min-height: %7px;"
                       " padding: %3px %4px;"
                       " border-top-left-radius: 6px;"
                       " border-top-right-radius: 6px;"
                       " margin-right: 2px;"
                       " }"
                       "QTabBar::tab:selected { padding-top: %5px; padding-bottom: %6px; }"
                       "QCheckBox, QLabel, QGroupBox { spacing: %4px; }"
                       "QGroupBox {"
                       " border-radius: 8px;"
                       " margin-top: %7px;"
                       " padding-top: %7px;"
                       " }"
                       "QCheckBox::indicator {"
                       " width: %8px;"
                       " height: %9px;"
                       " border-radius: %10px;"
                       " }"
                       "QHeaderView::section { font-size: %1pt; padding: %3px %4px; }"
                       "QMenu::item { padding: %3px %11px; }"
                       "%12")
                .arg(metrics.fontPointSize)
                .arg(metrics.controlHeight)
                .arg(metrics.verticalPadding)
                .arg(metrics.horizontalPadding)
                .arg(std::max(1, metrics.verticalPadding - 1))
                .arg(metrics.verticalPadding + 1)
                .arg(metrics.tabHeight)
                .arg(metrics.toggleWidth)
                .arg(metrics.toggleHeight)
                .arg(metrics.toggleHeight / 2)
                .arg(metrics.horizontalPadding * 2)
                .arg(fairwindsk::ui::widgets::TouchScrollArea::scrollBarStyleSheet());
        }

        void applyIconMetrics(QWidget *widget, const UiMetrics &metrics) {
            if (auto *toolButton = qobject_cast<QToolButton *>(widget)) {
                const bool prominent =
                    toolButton->toolButtonStyle() == Qt::ToolButtonTextUnderIcon ||
                    toolButton->iconSize().width() >= 48 ||
                    toolButton->minimumHeight() >= 64;
                const QSize iconSize(prominent ? metrics.prominentIconSize : metrics.actionIconSize,
                                     prominent ? metrics.prominentIconSize : metrics.actionIconSize);
                if (!toolButton->icon().isNull()) {
                    toolButton->setIconSize(iconSize);
                }
                if (prominent) {
                    toolButton->setMinimumHeight(std::max(toolButton->minimumHeight(), iconSize.height() + 28));
                }
                return;
            }

            if (auto *pushButton = qobject_cast<QPushButton *>(widget)) {
                if (!pushButton->icon().isNull()) {
                    pushButton->setIconSize(QSize(metrics.actionIconSize, metrics.actionIconSize));
                }
            }
        }

        nlohmann::json fetchJsonPayload(const QUrl &url, const bool debug) {
            if (!url.isValid()) {
                return {};
            }

            QNetworkAccessManager networkAccessManager;
            QNetworkRequest request(url);
            request.setTransferTimeout(kAppsRequestTimeoutMs);
            QPointer<QNetworkReply> reply = networkAccessManager.get(request);
            if (!reply) {
                return {};
            }

            QEventLoop loop;
            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeoutTimer.start(kAppsRequestTimeoutMs);
            loop.exec(QEventLoop::ExcludeUserInputEvents);

            if (!reply) {
                return {};
            }

            const bool timedOut = !reply->isFinished();
            if (timedOut) {
                reply->abort();
            }

            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (timedOut || !reply->isOpen() || reply->error() != QNetworkReply::NoError) {
                reply->deleteLater();
                return {};
            }
            const QByteArray payload = reply->readAll();
            reply->deleteLater();

            if (statusCode < 200 || statusCode >= 300 || payload.isEmpty()) {
                return {};
            }

            try {
                const auto json = nlohmann::json::parse(payload.constData(), payload.constData() + payload.size());
                if (debug) {
                    qDebug() << "Apps payload from" << url << ":" << QString::fromStdString(json.dump(2));
                }
                return json;
            } catch (const std::exception &exception) {
                if (debug) {
                    qWarning() << "Unable to parse apps payload from" << url << ":" << exception.what();
                }
                return {};
            }
        }

        void ensureFairwindMetadata(nlohmann::json &appJsonObject, const int order) {
            if (!appJsonObject.contains("fairwind") || !appJsonObject["fairwind"].is_object()) {
                appJsonObject["fairwind"] = nlohmann::json::object();
            }

            auto &fairwindJsonObject = appJsonObject["fairwind"];
            if (!fairwindJsonObject.contains("active") || !fairwindJsonObject["active"].is_boolean()) {
                fairwindJsonObject["active"] = true;
            }
            if (!fairwindJsonObject.contains("order") || !fairwindJsonObject["order"].is_number_integer()) {
                fairwindJsonObject["order"] = order;
            }
        }
    }
/*
 * FairWind
 * Private constructor - called by getInstance in order to ensure
 * the singleton design pattern
 */
    FairWindSK::FairWindSK() {
        // Se the default configuration file name
        m_configFilename = defaultConfigFilename();

        // Create the WebEngine profile file name
        auto profileName = QString::fromLatin1("FairWindSK.%1").arg(qWebEngineChromiumVersion());

        // Create the WebEngine profile
        m_profile = new QWebEngineProfile(profileName, this);

        if (qApp) {
            qApp->installEventFilter(this);
        }

        m_autoComfortTimer = new QTimer(this);
        m_autoComfortTimer->setInterval(60000);
        connect(m_autoComfortTimer, &QTimer::timeout, this, &FairWindSK::refreshAutomaticComfortView);

        updateWebProfileCookie();
    }



/*
 * getInstance
 * Either returns the available instance or creates a new one
 */
    FairWindSK *FairWindSK::getInstance() {

        // Check if there is no previous instance
        if (m_instance == nullptr) {

            // Create the instance
            m_instance = new FairWindSK();
        }

        // Return the instance
        return m_instance;
    }

    /*
     * loadConfig()
     * Load the configuration from the json file
     */
    void FairWindSK::loadConfig() {

        // Initialize the QT managed settings
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);

        // Get the name of the FairWind++ configuration file
        m_debug = settings.value("debug", m_debug).toBool();

        // Store the configuration in the settings
        settings.setValue("debug", m_debug);

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "QWebEngineProfile " << m_profile->isOffTheRecord() << " data store: " << m_profile->persistentStoragePath();


            // Write a message
            qDebug() << "Loading configuration from ini file...";
        }

        // Get the name of the FairWindSK configuration file
        m_configFilename = normalizedConfigFilename(settings.value("config", m_configFilename).toString());

        // Store the name of the FairWindSK configuration in the settings
        settings.setValue("config",m_configFilename);
        settings.sync();

        // Set the configuration file name
        m_configuration.setFilename(m_configFilename);

        // Check if the file exists
        if (QFileInfo::exists(m_configFilename)) {

            // Load the configuration
            m_configuration.load();
        }
        else {
            // Set the default
            m_configuration.setDefault();

            // Save the configuration file
            m_configuration.save();
        }

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "Configuration from ini file loaded.";

            // Set the QT logging
            QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);
        }

        updateWebProfileCookie();
        refreshAutomaticComfortViewAvailability();
        applyUiPreferences();

    }

    bool FairWindSK::isAutomaticComfortViewConfigured(const Configuration *configuration) const {
        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        return !effectiveConfiguration.getSignalKPath(QStringLiteral("environment.sun")).trimmed().isEmpty();
    }

    bool FairWindSK::isAutomaticComfortViewAvailable(const Configuration *configuration) const {
        Q_UNUSED(configuration);
        return isAutomaticComfortViewConfigured(configuration) && m_automaticComfortViewAvailable;
    }

    QString FairWindSK::getActiveComfortViewPreset(const Configuration *configuration) const {
        if (!m_activeComfortViewPreset.trimmed().isEmpty()) {
            return m_activeComfortViewPreset;
        }

        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        if (effectiveConfiguration.getComfortViewMode() == "auto" && !isAutomaticComfortViewAvailable(&effectiveConfiguration)) {
            QString preset = effectiveConfiguration.getComfortViewPreset().trimmed().toLower();
            if (preset == "sunrise") {
                preset = QStringLiteral("dawn");
            }
            if (preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                preset = QStringLiteral("day");
            }
            return preset;
        }

        if (effectiveConfiguration.getComfortViewMode() != "auto") {
            QString preset = effectiveConfiguration.getComfortViewPreset().trimmed().toLower();
            if (preset == "sunrise") {
                preset = QStringLiteral("dawn");
            }
            if (preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                preset = QStringLiteral("day");
            }
            return preset;
        }

        const QTime now = QTime::currentTime();
        if (now >= QTime(5, 30) && now < QTime(7, 0)) {
            return QStringLiteral("dawn");
        }
        if (now >= QTime(7, 0) && now < QTime(17, 30)) {
            return QStringLiteral("day");
        }
        if (now >= QTime(17, 30) && now < QTime(19, 0)) {
            return QStringLiteral("sunset");
        }
        if (now >= QTime(19, 0) && now < QTime(20, 30)) {
            return QStringLiteral("dusk");
        }
        return QStringLiteral("night");
    }

    void FairWindSK::applyUiPreferences(const Configuration *configuration) {
        if (!qApp) {
            return;
        }

        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        const auto preset = resolvedUiScalePreset(effectiveConfiguration);
        const auto metrics = uiMetricsForPreset(preset);
        const QString comfortPreset =
            effectiveConfiguration.getComfortViewMode() == "auto" && !isAutomaticComfortViewAvailable(&effectiveConfiguration)
                ? normalizedComfortViewPreset(effectiveConfiguration.getComfortViewPreset())
                : resolvedComfortViewPreset(effectiveConfiguration);
        const auto comfortPalette = uiComfortPaletteForPreset(comfortPreset);

        QFont appFont = qApp->font();
        appFont.setPointSize(metrics.fontPointSize);
        qApp->setFont(appFont);
        QPalette appPalette = qApp->palette();
        appPalette.setColor(QPalette::Window, QColor(comfortPalette.window));
        appPalette.setColor(QPalette::Base, QColor(comfortPalette.base));
        appPalette.setColor(QPalette::AlternateBase, QColor(comfortPalette.alternateBase));
        appPalette.setColor(QPalette::Button, QColor(comfortPalette.buttonBottom));
        appPalette.setColor(QPalette::WindowText, QColor(comfortPalette.text));
        appPalette.setColor(QPalette::Text, QColor(comfortPalette.fieldText));
        appPalette.setColor(QPalette::ButtonText, QColor(comfortPalette.text));
        appPalette.setColor(QPalette::Highlight, QColor(comfortPalette.accentTop));
        appPalette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#eff6ff")));
        qApp->setPalette(appPalette);
        const QString combinedStyleSheet =
            buildUiMetricsStyleSheet(metrics)
            + loadComfortThemeStyleSheet(effectiveConfiguration, comfortPreset)
            + loadComfortBackgroundStyleSheet(effectiveConfiguration, comfortPreset);
        if (qApp->styleSheet() != combinedStyleSheet) {
            qApp->setStyleSheet(combinedStyleSheet);
        }

        m_activeComfortViewPreset = comfortPreset;

        if (m_autoComfortTimer) {
            if (effectiveConfiguration.getComfortViewMode() == "auto" && isAutomaticComfortViewAvailable(&effectiveConfiguration)) {
                if (!m_autoComfortTimer->isActive()) {
                    m_autoComfortTimer->start();
                }
            } else {
                m_autoComfortTimer->stop();
            }
        }

        const auto widgets = QApplication::allWidgets();
        for (auto *widget : widgets) {
            if (!widget) {
                continue;
            }

            applyIconMetrics(widget, metrics);
            widget->updateGeometry();
            widget->update();
        }
    }

    void FairWindSK::refreshAutomaticComfortView() {
        refreshAutomaticComfortViewAvailability();

        if (m_configuration.getComfortViewMode() != "auto" || !isAutomaticComfortViewAvailable()) {
            if (m_autoComfortTimer) {
                m_autoComfortTimer->stop();
            }
            return;
        }

        const QString resolvedPreset = resolvedComfortViewPreset(m_configuration);
        if (resolvedPreset != m_activeComfortViewPreset) {
            applyUiPreferences();
        }
    }

    void FairWindSK::refreshAutomaticComfortViewAvailability(const Configuration *configuration) {
        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        m_automaticComfortViewAvailable = isAutomaticComfortViewSupported(effectiveConfiguration, &m_signalkClient);
    }

    void FairWindSK::reconfigureRuntime(const quint32 runtimeChanges) {
        const bool signalKSettingsChanged = (runtimeChanges & (RuntimeSignalKConnection | RuntimeSignalKPaths)) != 0;

        if (runtimeChanges & RuntimeSignalKConnection) {
            updateWebProfileCookie();
        }

        if (runtimeChanges & (RuntimeUnits | RuntimeSignalKConnection | RuntimeSignalKPaths)) {
            Units::getInstance()->refreshSignalKPreferences();
        }

        if (signalKSettingsChanged && !m_configuration.getSignalKServerUrl().isEmpty()) {
            startSignalK();
        }

        if (signalKSettingsChanged) {
            refreshAutomaticComfortViewAvailability();
        }

        if ((runtimeChanges & RuntimeUi) || signalKSettingsChanged) {
            applyUiPreferences();
        }

        if (runtimeChanges & RuntimeApps) {
            loadApps();
        }

        if (auto *mainWindow = fairwindsk::ui::MainWindow::instance()) {
            mainWindow->applyRuntimeConfiguration();
        }
    }

    void FairWindSK::updateWebProfileCookie() {
        if (!m_profile) {
            return;
        }

        const auto serverUrl = m_configuration.getSignalKServerUrl();
        const auto token = fairwindsk::Configuration::getToken();
        if (serverUrl.isEmpty() || token.isEmpty()) {
            return;
        }

        auto authenticationCookie = QNetworkCookie("JAUTHENTICATION", token.toUtf8());
        authenticationCookie.setPath("/");
        m_profile->cookieStore()->setCookie(authenticationCookie, QUrl(serverUrl));
    }

    /*
     * startSignalK()
     * Starts the Signal K client
     */
    bool FairWindSK::startSignalK() {

        // Set the result as false
        bool result = false;

        // Get the Signal K server URL
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        // Check if the Signal K URL is not empty
        if (!signalKServerUrl.isEmpty()) {

            // Define the parameters map
            QMap<QString, QVariant> params;

            // Set some defaults
            params["active"] = true;

            // Setup the debug mode
            params["debug"] = m_debug;

            // Set the url
            params["url"] = signalKServerUrl + "/signalk";

            // Get the token
            QString token = fairwindsk::Configuration::getToken();

            // Check if the token is defined
            if (!token.isEmpty()) {

                // Set the token
                params["token"] = token;
            }

            // Number of connection tentatives
            int count = 1;

            // Start the connection
            do {

                // Check if the debug is active
                if (isDebug()) {

                    // Write a message
                    qDebug() << "Trying to connect to the " << signalKServerUrl << " Signal K server ("
                             << count
                             << "/" << m_nRetry << ")...";
                }

                // Try to connect
                result = m_signalkClient.init(params);

                // Check if the connection is successful
                if (result) {

                    // Set the token
                    fairwindsk::Configuration::setToken(m_signalkClient.getToken());
                    updateWebProfileCookie();
                    Units::getInstance()->refreshSignalKPreferences();
                    refreshAutomaticComfortViewAvailability();
                    if (m_configuration.getComfortViewMode() == "auto") {
                        applyUiPreferences();
                    }

                    // Exit the loop
                    break;
                }

                // Keep progress responsive without letting user input re-enter the startup flow.
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                // Increase the number of retry
                count++;

                // Wait for m_mSleep microseconds
                QThread::msleep(m_mSleep);

                // Loop until the number of retry
            } while (count < m_nRetry);

            // Check if the debug is active
            if (isDebug()) {

                // Check if the client is connected
                if (result) {

                    // Write a message
                    qDebug() << "Connected to " << signalKServerUrl;
                } else {

                    // Write a message
                    qDebug() << "No response from the " << signalKServerUrl << " Signal K server!";
                }
            }
        }

        // Return the result
        return result;
    }

    /*
     * loadApps()
     * Load the applications
     */
    bool FairWindSK::loadApps() {

        // Set the result value
        bool result = false;

        // Get the configuration root JSON object
        auto &configurationJsonObject = m_configuration.getRoot();

        // Check if apps is not defined in configuration
        if (!configurationJsonObject.contains("apps")) {

            // Add the apps array
            configurationJsonObject["apps"] = nlohmann::json::array();
        }

        // Get the app keys
        auto keys = m_mapHash2AppItem.keys();

        // Remove all app items
        for (const auto& key: keys) {

            // Remove the item
            delete m_mapHash2AppItem[key];
        }

        // Remove the map content
        m_mapHash2AppItem.clear();
        m_mapAppId2Hash.clear();

        // Reset the counter
        int count = 100;

        // Get the signalk server url from the configuration
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();

        // Check if it not empty
        if (!signalKServerUrl.isEmpty()) {
            auto appsPayload = fetchJsonPayload(QUrl(signalKServerUrl + "/signalk/v1/apps/list"), isDebug());
            if (!appsPayload.is_array()) {
                appsPayload = fetchJsonPayload(QUrl(signalKServerUrl + "/skServer/webapps"), isDebug());
            }

            if (appsPayload.is_array()) {
                for (auto appJsonObject : appsPayload) {
                    if (!appJsonObject.is_object()) {
                        continue;
                    }

                    if (appJsonObject.contains("keywords") && appJsonObject["keywords"].is_array()) {
                        std::vector<std::string> keywords = appJsonObject["keywords"];
                        QStringList stringListKeywords;
                        std::transform(
                            keywords.begin(), keywords.end(),
                            std::back_inserter(stringListKeywords), [](const std::string &value) {
                                return QString::fromStdString(value);
                            }
                        );
                        if (!stringListKeywords.contains("signalk-webapp")) {
                            continue;
                        }
                    }

                    ensureFairwindMetadata(appJsonObject, count);

                    auto *appItem = new AppItem(appJsonObject);
                    const QString appName = appItem->getName();
                    const int idx = m_configuration.findApp(appName);

                    if (idx == -1) {
                        m_configuration.getRoot()["apps"].push_back(appItem->asJson());
                    }

                    m_mapHash2AppItem[appName] = appItem;
                    m_mapAppId2Hash[appName] = appName;
                    count++;

                    if (isDebug()) {
                        qDebug() << "Added (app from the Signal K Apps API): "
                                 << QString::fromStdString(appItem->asJson().dump(2));
                    }
                }

                result = true;
            }
        } else {

            // Check if the debug is active
            if (m_debug) {
                // Show a message
                qDebug() << "Troubles on getting apps from " << signalKServerUrl;
            }
        }

        // Get the apps array
        auto appsJsonArray = configurationJsonObject["apps"];

        // For each app in the apps array
        for (auto app: appsJsonArray) {

            // Check if the app is an object
            if (app.is_object()) {

                // Check if the app has the key name and if the value is a string
                if (app.contains("name") && app["name"].is_string()) {

                    // Get the app name
                    auto appName = QString::fromStdString(app["name"].get<std::string>());

                    // Check if the app is one of the ones already added
                    if (m_mapHash2AppItem.contains(appName)) {

                        // Get the app item object
                        auto appItem = m_mapHash2AppItem[appName];

                        // Get the index of the application within the apps array
                        int idx = m_configuration.findApp(appName);

                        // Check if the app is present
                        if (idx != -1) {

                            // Get the item as a json
                            auto j = appItem->asJson();

                            // Update the item with app data
                            j.update(app, true);

                            // Assign the item to the configuration
                            m_configuration.getRoot()["apps"].at(idx) = j;

                            // Update the app with the configuration
                            m_mapHash2AppItem[appName]->update(j);

                            // Check if the debug is active
                            if (isDebug())
                            {
                                // Write a message
                                qDebug() << "Updated (app from the Signal k server updated by the configuration file): " << QString::fromStdString(m_configuration.getRoot()["apps"].at(idx).dump(2));
                            }
                        }
                    } else {

                        // Check if it is not a Signal K app
                        if (appName.startsWith("http://") || appName.startsWith("https://") || appName.startsWith("file://")) {

                            // Create a new app item with the configuration
                            auto appItem = new AppItem(app);

                            // Check if the order is 0
                            if (appItem->getOrder() == 0) {

                                // Update the order
                                appItem->setOrder(count);

                                // Get the index of the application within the apps array
                                int idx = m_configuration.findApp(appName);

                                // Check if the app is present
                                if (idx != -1) {

                                    // Update the configuration
                                    m_configuration.getRoot()["apps"].at(idx)["fairwind"]["order"] = count;

                                    // Check if the debug is active
                                    if (isDebug()) {

                                        // Write a message
                                        qDebug() << "Added (app from the configuration file): " << QString::fromStdString(m_configuration.getRoot()["apps"].at(idx).dump(2));
                                    }

                                    // Increase the counter
                                    count++;
                                }
                            }

                            // Add the application to the hash map
                            m_mapHash2AppItem[appName] = appItem;
                            m_mapAppId2Hash[appName] = appName;

                            // Check if the debug is active
                            if (isDebug()) {

                                // Write a message
                                qDebug() << "Added: " << QString::fromStdString(appItem->asJson().dump(2));
                            }
                        } else {

                            // Get the index of the application within the apps array
                            int idx = m_configuration.findApp(appName);

                            // Check if the app is present
                            if (idx != -1) {

                                auto &configuredApp = m_configuration.getRoot()["apps"].at(idx);
                                bool wasActive = false;
                                if (configuredApp.contains("fairwind") &&
                                    configuredApp["fairwind"].is_object() &&
                                    configuredApp["fairwind"].contains("active") &&
                                    configuredApp["fairwind"]["active"].is_boolean()) {
                                    wasActive = configuredApp["fairwind"]["active"].get<bool>();
                                }

                                // Update the configuration
                                configuredApp["fairwind"]["order"] = 10000+count;

                                // Set the application as inactive by default
                                configuredApp["fairwind"]["active"] = false;

                                // Increase the caunter
                                count++;

                                if (wasActive || m_debug) {
                                    qDebug() << "Deactivated (Signal K application present in the configuration file, but not on the Signal K server): "
                                             << QString::fromStdString(configuredApp.dump(2));
                                }

                            } else {
                                // Check if the debug is active
                                if (isDebug()) {

                                    // Write a message
                                    qDebug() << "Error!";
                                }
                            }
                        }
                    }
                }
            }
        }

        // Get the configuration json data root
        auto jsonData =m_configuration.getRoot();

        // Check if the debug is active
        if (isDebug()) {

            // Write a message
            qDebug() << "Resume";
        }

        // Check if the configuration has an apps element and if it is an array
        if (jsonData.contains("apps") && jsonData["apps"].is_array()) {

            // For each item of the apps array...
            for (const auto &app: jsonData["apps"].items()) {

                // Get the application data
                const auto& jsonApp = app.value();

                // Create an application object
                AppItem appItem(jsonApp);

                // Check if the debug is active
                if (isDebug())
                {
                    // Write a message
                    qDebug() << "App: " << appItem.getName() << " active: " << appItem.getActive() << " order: " << appItem.getOrder();
                }
            }
        }

        // Return the result
        return result;
    }

    QList<QString> FairWindSK::getAppsHashes() {
        return m_mapHash2AppItem.keys();
    }

    AppItem *FairWindSK::getAppItemByHash(const QString& hash) {
        return m_mapHash2AppItem.value(hash, nullptr);
    }

    QString FairWindSK::getAppHashById(const QString& appId) {
        if (m_mapHash2AppItem.contains(appId)) {
            return appId;
        }
        return m_mapAppId2Hash.value(appId);
    }



    bool FairWindSK::isDebug() const {
        return m_debug;
    }

    /*
 * getConfig
 * Returns the configuration infos
 */
    Configuration *FairWindSK::getConfiguration() {

        // Return the result
        return &m_configuration;
    }

    void FairWindSK::setConfiguration(Configuration *configuration) {
        m_configuration.setRoot(configuration->getRoot());
        applyUiPreferences();
    }

    signalk::Client *FairWindSK::getSignalKClient() {
        return &m_signalkClient;
    }

    QWebEngineProfile *FairWindSK::getWebEngineProfile() {
        return m_profile;
    }



    bool FairWindSK::checkAutopilotApp() {

        // Set by default result as false
        auto result = false;

        // Check if the anchor application is defined and installed
        if (!m_configuration.getAutopilotApp().isEmpty() && getAppsHashes().contains(m_configuration.getAutopilotApp())) {

            // Set the result as true
            result = true;
        }

        // Return the result
        return result;
    }

    bool FairWindSK::checkAnchorApp() {

        // Set by default result as false
        auto result = false;

        // Check if the anchor application is defined and installed
        if (!m_configuration.getAnchorApp().isEmpty() && getAppsHashes().contains(m_configuration.getAnchorApp())) {

            // Set the result as true
            result = true;
        }

        // Return the result
        return result;
    }

    FairWindSK::~FairWindSK() {
        if (qApp) {
            qApp->removeEventFilter(this);
        }

        // Get the application hashes
        auto hashes = m_mapHash2AppItem.keys();

        // For each hash...
        for (const auto& hash:hashes) {

            // Delete the application item
            delete m_mapHash2AppItem[hash];
        }

    }

    bool FairWindSK::eventFilter(QObject *watched, QEvent *event) {
        if (watched && (event->type() == QEvent::Show || event->type() == QEvent::Polish)) {
            if (auto *widget = qobject_cast<QWidget *>(watched)) {
                const auto metrics = uiMetricsForPreset(resolvedUiScalePreset(m_configuration));
                applyIconMetrics(widget, metrics);
            }
        }

        return QObject::eventFilter(watched, event);
    }

}
