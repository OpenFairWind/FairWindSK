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
#include <QStyle>
#include <QStackedWidget>


#include <QLoggingCategory>

#include <FairWindSK.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <utility>
#include <QNetworkCookie>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <QWebEngineCookieStore>
#endif
#include <QUrl>
#include <QPointer>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>

#include "Units.hpp"
#include "ui/MainWindow.hpp"
#include "ui/widgets/TouchScrollArea.hpp"


using namespace Qt::StringLiterals;

namespace fairwindsk {
    namespace {
        QColor comfortOverrideColor(const Configuration &configuration,
                                    const QString &preset,
                                    const QString &key,
                                    const QColor &fallback) {
            const QColor configured = configuration.getComfortThemeColor(preset, key, QColor());
            return configured.isValid() ? configured : fallback;
        }

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
            if (preset != "default" && preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                return QStringLiteral("default");
            }
            return preset;
        }

        QString resolveComfortViewPresetFromClock() {
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

        double normalizedSunAngleDegrees(const double rawValue) {
            if (std::isnan(rawValue)) {
                return rawValue;
            }

            const double absoluteValue = std::abs(rawValue);
            constexpr double pi = 3.14159265358979323846;
            if (absoluteValue <= (2.0 * pi + 0.001)) {
                return rawValue * (180.0 / pi);
            }
            return rawValue;
        }

        QString comfortPresetFromSunAngle(double angleDegrees) {
            angleDegrees = normalizedSunAngleDegrees(angleDegrees);
            if (std::isnan(angleDegrees)) {
                return {};
            }

            if (angleDegrees >= 6.0) {
                return QStringLiteral("day");
            }
            if (angleDegrees >= -2.0) {
                return QTime::currentTime() < QTime(12, 0) ? QStringLiteral("dawn") : QStringLiteral("sunset");
            }
            if (angleDegrees >= -8.0) {
                return QStringLiteral("dusk");
            }
            return QStringLiteral("night");
        }

        QDateTime comfortDateTimeFromValue(const QJsonValue &value) {
            if (value.isString()) {
                const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
                if (parsed.isValid()) {
                    return parsed;
                }
            }
            if (value.isObject()) {
                const auto object = value.toObject();
                const QString isoValue = object.value(QStringLiteral("value")).toString();
                const QDateTime parsed = QDateTime::fromString(isoValue, Qt::ISODate);
                if (parsed.isValid()) {
                    return parsed;
                }
            }
            return {};
        }

        QString comfortPresetFromSunSchedule(const QJsonObject &objectValue) {
            const QDateTime now = QDateTime::currentDateTimeUtc();

            const QDateTime dawn = comfortDateTimeFromValue(objectValue.value(QStringLiteral("dawn")));
            const QDateTime sunrise = comfortDateTimeFromValue(objectValue.value(QStringLiteral("sunrise")));
            const QDateTime sunriseEnd = comfortDateTimeFromValue(objectValue.value(QStringLiteral("sunriseEnd")));
            const QDateTime sunset = comfortDateTimeFromValue(objectValue.value(QStringLiteral("sunset")));
            const QDateTime dusk = comfortDateTimeFromValue(objectValue.value(QStringLiteral("dusk")));

            if (dawn.isValid() && sunrise.isValid() && now >= dawn && now < sunrise) {
                return QStringLiteral("dawn");
            }
            if (sunrise.isValid() && sunriseEnd.isValid() && now >= sunrise && now < sunriseEnd) {
                return QStringLiteral("day");
            }
            if (sunriseEnd.isValid() && sunset.isValid() && now >= sunriseEnd && now < sunset) {
                return QStringLiteral("day");
            }
            if (sunset.isValid() && dusk.isValid() && now >= sunset && now < dusk) {
                return QStringLiteral("sunset");
            }
            if (dusk.isValid() && now >= dusk) {
                return QStringLiteral("night");
            }
            if (dawn.isValid() && now < dawn) {
                return QStringLiteral("night");
            }

            const QDateTime civilDawn = comfortDateTimeFromValue(objectValue.value(QStringLiteral("civilDawn")));
            const QDateTime civilDusk = comfortDateTimeFromValue(objectValue.value(QStringLiteral("civilDusk")));
            if (civilDawn.isValid() && sunrise.isValid() && now >= civilDawn && now < sunrise) {
                return QStringLiteral("dawn");
            }
            if (sunset.isValid() && civilDusk.isValid() && now >= sunset && now < civilDusk) {
                return QStringLiteral("dusk");
            }

            return {};
        }

        QString comfortPresetFromSunUpdate(const QJsonObject &update) {
            if (update.isEmpty()) {
                return {};
            }

            const QJsonObject objectValue = fairwindsk::signalk::Client::getObjectFromUpdateByPath(update);
            const auto fromString = [](QString value) {
                value = normalizedComfortViewPreset(value);
                return value == QStringLiteral("default") ? QString() : value;
            };

            if (!objectValue.isEmpty()) {
                const QString state = fromString(objectValue.value(QStringLiteral("state")).toString());
                if (!state.isEmpty()) {
                    return state;
                }

                const QString mode = fromString(objectValue.value(QStringLiteral("mode")).toString());
                if (!mode.isEmpty()) {
                    return mode;
                }

                const QString phase = fromString(objectValue.value(QStringLiteral("phase")).toString());
                if (!phase.isEmpty()) {
                    return phase;
                }

                const QString scheduledPreset = comfortPresetFromSunSchedule(objectValue);
                if (!scheduledPreset.isEmpty()) {
                    return scheduledPreset;
                }

                const double altitude = objectValue.value(QStringLiteral("altitude")).toDouble(std::numeric_limits<double>::quiet_NaN());
                const QString altitudePreset = comfortPresetFromSunAngle(altitude);
                if (!altitudePreset.isEmpty()) {
                    return altitudePreset;
                }

                const double elevation = objectValue.value(QStringLiteral("elevation")).toDouble(std::numeric_limits<double>::quiet_NaN());
                const QString elevationPreset = comfortPresetFromSunAngle(elevation);
                if (!elevationPreset.isEmpty()) {
                    return elevationPreset;
                }
            }

            const QString directValue = fromString(fairwindsk::signalk::Client::getStringFromUpdateByPath(update));
            if (!directValue.isEmpty()) {
                return directValue;
            }

            return comfortPresetFromSunAngle(fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update));
        }

        QString resolvedComfortViewPreset(const Configuration &configuration, const QJsonObject &sunUpdate) {
            if (configuration.getComfortViewMode() != "auto") {
                return normalizedComfortViewPreset(configuration.getComfortViewPreset());
            }

            const QString environmentPreset = comfortPresetFromSunUpdate(sunUpdate);
            if (!environmentPreset.isEmpty()) {
                return environmentPreset;
            }

            return resolveComfortViewPresetFromClock();
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

            const auto buildSelectorBorderImageRule = [&configuration, &preset](const QString &area, const QString &selector) {
                const QString imagePath = configuration.getComfortBackgroundImagePath(preset, area).trimmed();
                if (imagePath.isEmpty()) {
                    return QString();
                }

                return QStringLiteral(
                    "%1 {"
                    " border-image: url(\"%2\") 0 0 0 0 stretch stretch;"
                    " background: transparent;"
                    " border: 0px;"
                    " }")
                    .arg(selector, QUrl::fromLocalFile(imagePath).toString());
            };

            const auto buildCheckboxRule = [&configuration, &preset](const QString &area, const QString &selector) {
                const QString imagePath = configuration.getComfortBackgroundImagePath(preset, area).trimmed();
                if (imagePath.isEmpty()) {
                    return QString();
                }

                return QStringLiteral(
                    "%1 {"
                    " image: url(\"%2\");"
                    " border: 0px;"
                    " background: transparent;"
                    " width: 28px;"
                    " height: 28px;"
                    " }")
                    .arg(selector, QUrl::fromLocalFile(imagePath).toString());
            };

            return buildRule(QStringLiteral("topbar"), QStringLiteral("TopBar"))
                + buildRule(QStringLiteral("launcher"), QStringLiteral("Launcher"))
                + buildRule(QStringLiteral("bottombar"), QStringLiteral("BottomBar"))
                + buildSelectorBorderImageRule(
                    QStringLiteral("buttons-default"),
                    QStringLiteral("QToolButton, QPushButton"))
                + buildSelectorBorderImageRule(
                    QStringLiteral("buttons-selected"),
                    QStringLiteral("QToolButton:pressed, QPushButton:pressed, QToolButton:checked, QPushButton:checked"))
                + buildSelectorBorderImageRule(
                    QStringLiteral("tabs-default"),
                    QStringLiteral("QTabBar::tab"))
                + buildSelectorBorderImageRule(
                    QStringLiteral("tabs-selected"),
                    QStringLiteral("QTabBar::tab:selected"))
                + buildCheckboxRule(
                    QStringLiteral("checkbox-default"),
                    QStringLiteral("QCheckBox::indicator:unchecked"))
                + buildCheckboxRule(
                    QStringLiteral("checkbox-selected"),
                    QStringLiteral("QCheckBox::indicator:checked"));
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
            } else if (normalizedPreset == "day") {
                palette.window = "#10253b";
                palette.panel = "#173149";
                palette.base = "#dbe8f5";
                palette.alternateBase = "#adc3da";
                palette.buttonTop = "#f1de9a";
                palette.buttonBottom = "#c59f4e";
                palette.buttonHoverTop = "#f7e8b2";
                palette.buttonHoverBottom = "#d4b567";
                palette.buttonPressedTop = "#5b84b8";
                palette.buttonPressedBottom = "#2f5fb7";
                palette.accentTop = "#5b84b8";
                palette.accentBottom = "#2f5fb7";
                palette.accentBorder = "#8bb3df";
                palette.text = "#f2f7ff";
                palette.subduedText = "#c2d3e5";
                palette.fieldText = "#10263c";
                palette.border = "#6f8dad";
                palette.borderStrong = "#3f6083";
                palette.scrollTrack = "#9bb2c8";
                palette.scrollHandleTop = "#e1ebf5";
                palette.scrollHandleMid = "#a6bdd4";
                palette.scrollHandleBottom = "#5f7f9f";
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
                palette.window = "#0d1521";
                palette.panel = "#12253a";
                palette.base = "#102338";
                palette.alternateBase = "#1a3047";
                palette.buttonTop = "#ecd28c";
                palette.buttonBottom = "#c6a357";
                palette.buttonHoverTop = "#f6df9e";
                palette.buttonHoverBottom = "#d4b468";
                palette.buttonPressedTop = "#2f5fb7";
                palette.buttonPressedBottom = "#1e4387";
                palette.accentTop = "#2f5fb7";
                palette.accentBottom = "#1e4387";
                palette.accentBorder = "#4c7bd1";
                palette.text = "#edf5ff";
                palette.subduedText = "#c0d0e2";
                palette.fieldText = "#edf5ff";
                palette.border = "#6f89a7";
                palette.borderStrong = "#4f5d72";
                palette.scrollTrack = "#20374f";
                palette.scrollHandleTop = "#f4fbff";
                palette.scrollHandleMid = "#d7e6f5";
                palette.scrollHandleBottom = "#bfd2e6";
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

        nlohmann::json parseJsonPayload(const QByteArray &payload, const QUrl &url, const bool debug) {
            if (payload.isEmpty()) {
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

        bool isOnHiddenStackPage(QWidget *widget) {
            if (!widget) {
                return false;
            }

            QWidget *current = widget;
            while (current) {
                auto *parentWidget = current->parentWidget();
                if (!parentWidget) {
                    return false;
                }

                if (auto *stackedWidget = qobject_cast<QStackedWidget *>(parentWidget)) {
                    return stackedWidget->currentWidget() != current;
                }

                current = parentWidget;
            }

            return false;
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

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        // Create the WebEngine profile file name
        auto profileName = QString::fromLatin1("FairWindSK.%1").arg(qWebEngineChromiumVersion());

        // Create the WebEngine profile
        m_profile = new QWebEngineProfile(profileName, this);
#endif

        if (qApp) {
            qApp->installEventFilter(this);
        }

        m_autoComfortTimer = new QTimer(this);
        m_autoComfortTimer->setInterval(60000);
        connect(m_autoComfortTimer, &QTimer::timeout, this, &FairWindSK::refreshAutomaticComfortView);
        m_runtimeNetworkAccessManager = new QNetworkAccessManager(this);
        connect(&m_signalkClient, &signalk::Client::serverStateResynchronized, this, [this](const bool recoveredFromDisconnect) {
            if (!recoveredFromDisconnect) {
                return;
            }

            qInfo() << "FairWindSK detected Signal K recovery; resynchronizing runtime state";
            Units::getInstance()->refreshSignalKPreferences();
            reloadAppsAsync();
            refreshAutomaticComfortViewAvailability();
            if (m_configuration.getComfortViewMode() == "auto") {
                applyUiPreferences();
            }
            if (auto *mainWindow = fairwindsk::ui::MainWindow::instance()) {
                mainWindow->applyRuntimeConfiguration();
            }
        });
        connect(&m_signalkClient, &signalk::Client::connectionHealthStateChanged, this, [this](const signalk::Client::ConnectionHealthState,
                                                                                                 const QString &,
                                                                                                 const QDateTime &,
                                                                                                 const QString &) {
            refreshRuntimeHealth();
        });
        connect(&m_signalkClient, &signalk::Client::connectivityChanged, this, [this](const bool restHealthy,
                                                                                       const bool streamHealthy,
                                                                                       const QString &statusText) {
            Q_UNUSED(statusText)
            if (!restHealthy && !streamHealthy) {
                refreshRuntimeHealth();
                return;
            }

            const QString token = m_signalkClient.getToken();
            if (token.isEmpty()) {
                refreshRuntimeHealth();
                return;
            }

            if (fairwindsk::Configuration::getToken() != token) {
                fairwindsk::Configuration::setToken(token);
            }
            updateWebProfileCookie();
            refreshRuntimeHealth();
        });

        updateWebProfileCookie();
        refreshRuntimeHealth();
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
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
            if (m_profile) {
                qDebug() << "QWebEngineProfile " << m_profile->isOffTheRecord() << " data store: " << m_profile->persistentStoragePath();
            }
#endif


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
            if (preset != "default" && preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                preset = QStringLiteral("default");
            }
            return preset;
        }

        if (effectiveConfiguration.getComfortViewMode() != "auto") {
            QString preset = effectiveConfiguration.getComfortViewPreset().trimmed().toLower();
            if (preset == "sunrise") {
                preset = QStringLiteral("dawn");
            }
            if (preset != "default" && preset != "dawn" && preset != "day" && preset != "sunset" && preset != "dusk" && preset != "night") {
                preset = QStringLiteral("default");
            }
            return preset;
        }

        return resolvedComfortViewPreset(effectiveConfiguration, m_automaticComfortEnvironmentUpdate);
    }

    UiScrollPalette FairWindSK::getActiveComfortScrollPalette(const Configuration *configuration) const {
        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        const QString preset = getActiveComfortViewPreset(&effectiveConfiguration);
        const UiComfortPalette palette = uiComfortPaletteForPreset(preset);
        return {
            comfortOverrideColor(effectiveConfiguration, preset, QStringLiteral("scrollBarBackground"), QColor(palette.scrollTrack)),
            comfortOverrideColor(effectiveConfiguration, preset, QStringLiteral("scrollBarKnob"), QColor(palette.scrollHandleTop)).lighter(108),
            comfortOverrideColor(effectiveConfiguration, preset, QStringLiteral("scrollBarKnob"), QColor(palette.scrollHandleMid)),
            comfortOverrideColor(effectiveConfiguration, preset, QStringLiteral("scrollBarKnob"), QColor(palette.scrollHandleBottom)).darker(108)
        };
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
                : resolvedComfortViewPreset(effectiveConfiguration, m_automaticComfortEnvironmentUpdate);
        const auto comfortPalette = uiComfortPaletteForPreset(comfortPreset);

        const QColor applicationBackgroundColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("applicationBackground"), QColor(comfortPalette.panel));
        const QColor panelColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("panel"), QColor(comfortPalette.panel));
        const QColor baseColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("base"), QColor(comfortPalette.base));
        const QColor textColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("text"), QColor(comfortPalette.text));
        const QColor buttonBackgroundColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("buttonBackground"), QColor(comfortPalette.buttonBottom));
        const QColor buttonTextColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("buttonText"), QColor(comfortPalette.text));
        const QColor accentTopColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("accentTop"), QColor(comfortPalette.accentTop));
        const QColor accentTextColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("accentText"), QColor(QStringLiteral("#eff6ff")));
        const QColor borderColor = comfortOverrideColor(effectiveConfiguration, comfortPreset, QStringLiteral("border"), QColor(comfortPalette.border));
        const QString metricsSignature = QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8")
            .arg(preset)
            .arg(metrics.fontPointSize)
            .arg(metrics.controlHeight)
            .arg(metrics.tabHeight)
            .arg(metrics.actionIconSize)
            .arg(metrics.prominentIconSize)
            .arg(metrics.toggleWidth)
            .arg(metrics.toggleHeight);

        QFont appFont = qApp->font();
        appFont.setPointSize(metrics.fontPointSize);
        qApp->setFont(appFont);
        QPalette appPalette = qApp->palette();
        appPalette.setColor(QPalette::Window, applicationBackgroundColor);
        appPalette.setColor(QPalette::Base, baseColor);
        appPalette.setColor(QPalette::AlternateBase, panelColor);
        appPalette.setColor(QPalette::Button, buttonBackgroundColor);
        appPalette.setColor(QPalette::Mid, borderColor);
        appPalette.setColor(QPalette::WindowText, textColor);
        appPalette.setColor(QPalette::Text, textColor);
        appPalette.setColor(QPalette::ButtonText, buttonTextColor);
        appPalette.setColor(QPalette::Highlight, accentTopColor);
        appPalette.setColor(QPalette::HighlightedText, accentTextColor);
        qApp->setPalette(appPalette);
        const QString combinedStyleSheet =
            buildUiMetricsStyleSheet(metrics)
            + loadComfortThemeStyleSheet(effectiveConfiguration, comfortPreset)
            + loadComfortBackgroundStyleSheet(effectiveConfiguration, comfortPreset);
        const bool metricsChanged = m_lastUiMetricsSignature != metricsSignature;
        const bool themeChanged = m_lastUiThemeSignature != combinedStyleSheet;
        if (themeChanged) {
            qApp->setStyleSheet(combinedStyleSheet);
        }

        m_activeComfortViewPreset = comfortPreset;
        m_lastUiMetricsSignature = metricsSignature;
        m_lastUiThemeSignature = combinedStyleSheet;

        if (m_autoComfortTimer) {
            if (effectiveConfiguration.getComfortViewMode() == "auto" && isAutomaticComfortViewAvailable(&effectiveConfiguration)) {
                if (!m_autoComfortTimer->isActive()) {
                    m_autoComfortTimer->start();
                }
            } else {
                m_autoComfortTimer->stop();
            }
        }

        if (!metricsChanged && !themeChanged) {
            return;
        }

        const auto widgets = QApplication::allWidgets();
        for (auto *widget : widgets) {
            if (!widget || isOnHiddenStackPage(widget)) {
                continue;
            }

            const bool widgetVisible = widget->isVisible();
            if (metricsChanged && widgetVisible) {
                applyIconMetrics(widget, metrics);
            }
            if (themeChanged && widgetVisible && qApp->style()) {
                auto *style = qApp->style();
                style->unpolish(widget);
                style->polish(widget);
            }
            if (metricsChanged && widgetVisible) {
                widget->updateGeometry();
            }
            if (widgetVisible) {
                widget->update();
            }
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

        const QString resolvedPreset = resolvedComfortViewPreset(m_configuration, m_automaticComfortEnvironmentUpdate);
        if (resolvedPreset != m_activeComfortViewPreset) {
            applyUiPreferences();
        }
    }

    void FairWindSK::refreshAutomaticComfortViewAvailability(const Configuration *configuration) {
        const Configuration &effectiveConfiguration = configuration ? *configuration : m_configuration;
        const QString configuredSunPath = effectiveConfiguration.getSignalKPath(QStringLiteral("environment.sun")).trimmed();
        if (configuredSunPath != m_automaticComfortViewPath) {
            if (!m_automaticComfortViewPath.isEmpty()) {
                m_signalkClient.removeSubscription(m_automaticComfortViewPath, this);
            }
            m_automaticComfortViewPath = configuredSunPath;
            m_automaticComfortEnvironmentUpdate = {};
            if (!m_automaticComfortViewPath.isEmpty()) {
                m_signalkClient.subscribeStream(QStringLiteral("vessels.self"),
                                                m_automaticComfortViewPath,
                                                this,
                                                SLOT(onAutomaticComfortEnvironmentUpdate(QJsonObject)));
            }
        }

        m_automaticComfortViewAvailable =
            isAutomaticComfortViewConfigured(&effectiveConfiguration)
            && !comfortPresetFromSunUpdate(m_automaticComfortEnvironmentUpdate).isEmpty();
    }

    void FairWindSK::reconfigureRuntime(const quint32 runtimeChanges) {
        const bool signalKSettingsChanged = (runtimeChanges & (RuntimeSignalKConnection | RuntimeSignalKPaths)) != 0;

        if (runtimeChanges & RuntimeSignalKConnection) {
            updateWebProfileCookie();
        }

        if (runtimeChanges & (RuntimeUnits | RuntimeSignalKConnection | RuntimeSignalKPaths)) {
            Units::getInstance()->refreshSignalKPreferences();
        }

        if (signalKSettingsChanged) {
            if (m_configuration.getSignalKConnectionEnabled() && !m_configuration.getSignalKServerUrl().isEmpty()) {
                startSignalK();
            } else {
                stopSignalK();
            }
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        return;
#else
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
#endif
    }

    /*
     * startSignalK()
     * Starts the Signal K client
     */
    bool FairWindSK::startSignalK() {
        qInfo() << "FairWindSK::startSignalK entered";

        // Set the result as false
        bool result = false;

        // Get the Signal K server URL
        auto signalKServerUrl = m_configuration.getSignalKServerUrl();
        qInfo() << "FairWindSK::startSignalK server URL =" << signalKServerUrl;

        // Check if the Signal K URL is not empty
        if (!m_configuration.getSignalKConnectionEnabled()) {
            qInfo() << "FairWindSK::startSignalK skipped because connection is paused";
            stopSignalK();
        } else if (!signalKServerUrl.isEmpty()) {

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

            qInfo() << "FairWindSK::startSignalK starting non-blocking client initialization";
            result = m_signalkClient.init(params);
            qInfo() << "FairWindSK::startSignalK Client::init returned" << result;

            // Check if the debug is active
            if (isDebug()) {
                if (result) {
                    qDebug() << "Signal K startup initiated for" << signalKServerUrl;
                } else {
                    qDebug() << "Signal K startup disabled or unavailable for" << signalKServerUrl;
                }
            }
        } else {
            qWarning() << "FairWindSK::startSignalK skipped because server URL is empty";
        }

        // Return the result
        qInfo() << "FairWindSK::startSignalK exiting with result" << result;
        return result;
    }

    bool FairWindSK::stopSignalK() {
        qInfo() << "FairWindSK::stopSignalK entered";

        QMap<QString, QVariant> params;
        params["active"] = false;
        params["debug"] = m_debug;
        const QString signalKServerUrl = m_configuration.getSignalKServerUrl();
        if (!signalKServerUrl.isEmpty()) {
            params["url"] = signalKServerUrl + "/signalk";
        }

        m_signalkClient.init(params);
        refreshRuntimeHealth();

        qInfo() << "FairWindSK::stopSignalK exiting";
        return true;
    }

    /*
     * loadApps()
     * Load the applications
     */
    bool FairWindSK::loadApps() {
        const bool hasRegistry = rebuildAppRegistry();

        if (!m_configuration.getSignalKServerUrl().trimmed().isEmpty()) {
            reloadAppsAsync();
        } else {
            setAppsState(hasRegistry ? AppsState::Loaded : AppsState::Idle,
                         hasRegistry ? tr("Apps ready") : tr("Apps idle"));
        }

        return hasRegistry;
    }

    void FairWindSK::reloadAppsAsync() {
        const QString signalKServerUrl = m_configuration.getSignalKServerUrl().trimmed();
        if (!m_runtimeNetworkAccessManager || signalKServerUrl.isEmpty()) {
            setAppsState(m_mapHash2AppItem.isEmpty() ? AppsState::Idle : AppsState::Stale,
                         m_mapHash2AppItem.isEmpty() ? tr("Apps idle") : tr("Apps cached"));
            return;
        }

        ++m_appsReloadGeneration;
        const quint64 generation = m_appsReloadGeneration;
        if (m_appsReply) {
            m_appsReply->abort();
            m_appsReply->deleteLater();
            m_appsReply = nullptr;
        }

        setAppsState(AppsState::Loading,
                     m_mapHash2AppItem.isEmpty() ? tr("Loading apps") : tr("Refreshing apps"));
        emit appsReloadStarted();
        startAppsRequest(QUrl(signalKServerUrl + "/signalk/v1/apps/list"), generation, false);
    }

    bool FairWindSK::rebuildAppRegistry(const nlohmann::json *appsPayload) {
        auto &configurationJsonObject = m_configuration.getRoot();
        if (!configurationJsonObject.contains("apps")) {
            configurationJsonObject["apps"] = nlohmann::json::array();
        }

        const auto keys = m_mapHash2AppItem.keys();
        for (const auto &key : keys) {
            delete m_mapHash2AppItem[key];
        }
        m_mapHash2AppItem.clear();
        m_mapAppId2Hash.clear();

        int count = 100;
        bool hadServerPayload = appsPayload && appsPayload->is_array();

        if (hadServerPayload) {
            for (auto appJsonObject : *appsPayload) {
                if (!appJsonObject.is_object()) {
                    continue;
                }

                if (appJsonObject.contains("keywords") && appJsonObject["keywords"].is_array()) {
                    std::vector<std::string> keywords = appJsonObject["keywords"];
                    QStringList stringListKeywords;
                    std::transform(keywords.begin(), keywords.end(), std::back_inserter(stringListKeywords), [](const std::string &value) {
                        return QString::fromStdString(value);
                    });
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
        }

        const auto appsJsonArray = configurationJsonObject["apps"];
        for (auto app : appsJsonArray) {
            if (!app.is_object() || !app.contains("name") || !app["name"].is_string()) {
                continue;
            }

            const auto appName = QString::fromStdString(app["name"].get<std::string>());
            if (m_mapHash2AppItem.contains(appName)) {
                auto *appItem = m_mapHash2AppItem[appName];
                const int idx = m_configuration.findApp(appName);
                if (idx != -1) {
                    auto mergedJson = appItem->asJson();
                    mergedJson.update(app, true);
                    m_configuration.getRoot()["apps"].at(idx) = mergedJson;
                    appItem->update(mergedJson);
                }
                continue;
            }

            if (appName.startsWith("http://") || appName.startsWith("https://") || appName.startsWith("file://")) {
                auto *appItem = new AppItem(app);
                if (appItem->getOrder() == 0) {
                    appItem->setOrder(count);
                    const int idx = m_configuration.findApp(appName);
                    if (idx != -1) {
                        m_configuration.getRoot()["apps"].at(idx)["fairwind"]["order"] = count;
                    }
                    count++;
                }
                m_mapHash2AppItem[appName] = appItem;
                m_mapAppId2Hash[appName] = appName;
                continue;
            }

            const int idx = m_configuration.findApp(appName);
            if (idx != -1) {
                auto &configuredApp = m_configuration.getRoot()["apps"].at(idx);
                configuredApp["fairwind"]["order"] = 10000 + count;
                configuredApp["fairwind"]["active"] = false;
                count++;
            }
        }

        return !m_mapHash2AppItem.isEmpty() || hadServerPayload;
    }

    void FairWindSK::setAppsState(const AppsState state, const QString &stateText) {
        const QString normalizedStateText = stateText.trimmed().isEmpty() ? tr("Apps idle") : stateText.trimmed();
        if (m_appsState == state && m_appsStateText == normalizedStateText) {
            return;
        }

        m_appsState = state;
        m_appsStateText = normalizedStateText;
        emit appsStateChanged(m_appsState, m_appsStateText);
        refreshRuntimeHealth();
    }

    void FairWindSK::startAppsRequest(const QUrl &url, const quint64 generation, const bool fallbackRequest) {
        if (!m_runtimeNetworkAccessManager || !url.isValid()) {
            finalizeAppsReload(false, tr("Apps refresh failed"));
            return;
        }

        QNetworkRequest request(url);
        request.setTransferTimeout(kAppsRequestTimeoutMs);
        auto *reply = m_runtimeNetworkAccessManager->get(request);
        if (generation == m_appsReloadGeneration) {
            m_appsReply = reply;
        }

        connect(reply, &QNetworkReply::finished, this, [this, reply, url, generation, fallbackRequest]() {
            const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> guard(reply);
            if (generation != m_appsReloadGeneration) {
                return;
            }

            if (m_appsReply == reply) {
                m_appsReply = nullptr;
            }

            const int statusCode = guard->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool success = guard->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300;
            const nlohmann::json appsPayload = success ? parseJsonPayload(guard->readAll(), url, isDebug()) : nlohmann::json{};
            if (appsPayload.is_array()) {
                finalizeAppsReload(true, tr("Apps ready"), &appsPayload);
                return;
            }

            if (!fallbackRequest) {
                startAppsRequest(QUrl(m_configuration.getSignalKServerUrl().trimmed() + "/skServer/webapps"), generation, true);
                return;
            }

            finalizeAppsReload(false,
                               m_mapHash2AppItem.isEmpty()
                                   ? tr("Apps refresh failed")
                                   : tr("Apps cached"));
        });
    }

    void FairWindSK::finalizeAppsReload(const bool success,
                                        const QString &statusText,
                                        const nlohmann::json *appsPayload) {
        const bool hasRegistry = success ? rebuildAppRegistry(appsPayload) : !m_mapHash2AppItem.isEmpty();
        if (success) {
            setAppsState(AppsState::Loaded, statusText);
        } else if (hasRegistry) {
            setAppsState(AppsState::Stale, statusText);
        } else {
            setAppsState(AppsState::Failed, statusText);
        }

        emit appsReloadFinished(success);

        if (auto *mainWindow = fairwindsk::ui::MainWindow::instance()) {
            mainWindow->applyRuntimeConfiguration();
        }
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

    FairWindSK::AppsState FairWindSK::appsState() const {
        return m_appsState;
    }

    QString FairWindSK::appsStateText() const {
        return m_appsStateText;
    }

    FairWindSK::RuntimeHealthState FairWindSK::runtimeHealthState() const {
        return m_runtimeHealthState;
    }

    QString FairWindSK::runtimeHealthSummary() const {
        return m_runtimeHealthSummary;
    }

    QString FairWindSK::runtimeHealthBadgeText() const {
        return m_runtimeHealthBadgeText;
    }

    void FairWindSK::setForegroundAppHealth(const QString &summary, const bool degraded) {
        const QString normalizedSummary = summary.trimmed();
        if (m_foregroundAppHealthSummary == normalizedSummary && m_foregroundAppDegraded == degraded) {
            return;
        }

        m_foregroundAppHealthSummary = normalizedSummary;
        m_foregroundAppDegraded = degraded;
        refreshRuntimeHealth();
    }

    void FairWindSK::clearForegroundAppHealth() {
        setForegroundAppHealth(QString(), false);
    }

    void FairWindSK::refreshRuntimeHealth() {
        RuntimeHealthState nextState = RuntimeHealthState::Disconnected;
        QString nextSummary = tr("Signal K disconnected");
        QString nextBadge = QStringLiteral("DISC");

        const auto connectionState = m_signalkClient.connectionHealthState();
        const bool restHealthy = m_signalkClient.isRestHealthy();
        const bool streamHealthy = m_signalkClient.isStreamHealthy();
        const QString connectionSummary = m_signalkClient.connectionStatusText().trimmed();
        const QString appsSummary = m_appsStateText.trimmed();

        if (m_foregroundAppDegraded) {
            nextState = RuntimeHealthState::ForegroundAppDegraded;
            nextSummary = m_foregroundAppHealthSummary.isEmpty() ? tr("Foreground app degraded") : m_foregroundAppHealthSummary;
            nextBadge = QStringLiteral("APP");
        } else if (connectionState == signalk::Client::ConnectionHealthState::Connecting) {
            nextState = RuntimeHealthState::Connecting;
            nextSummary = connectionSummary.isEmpty() ? tr("Connecting to Signal K") : connectionSummary;
            nextBadge = QStringLiteral("CONN");
        } else if (connectionState == signalk::Client::ConnectionHealthState::Reconnecting) {
            nextState = RuntimeHealthState::Reconnecting;
            nextSummary = connectionSummary.isEmpty() ? tr("Reconnecting to Signal K") : connectionSummary;
            nextBadge = QStringLiteral("RECN");
        } else if (!restHealthy && streamHealthy) {
            nextState = RuntimeHealthState::RestDegraded;
            nextSummary = connectionSummary.isEmpty() ? tr("REST degraded") : connectionSummary;
            nextBadge = QStringLiteral("REST");
        } else if (restHealthy && !streamHealthy) {
            nextState = RuntimeHealthState::StreamDegraded;
            nextSummary = connectionSummary.isEmpty() ? tr("Stream degraded") : connectionSummary;
            nextBadge = QStringLiteral("STRM");
        } else if (connectionState == signalk::Client::ConnectionHealthState::Stale) {
            nextState = RuntimeHealthState::ConnectedStale;
            nextSummary = connectionSummary.isEmpty() ? tr("Live data stale") : connectionSummary;
            nextBadge = QStringLiteral("STAL");
        } else if (connectionState == signalk::Client::ConnectionHealthState::Live) {
            if (m_appsState == AppsState::Loading) {
                nextState = RuntimeHealthState::AppsLoading;
                nextSummary = appsSummary.isEmpty() ? tr("Signal K live • Refreshing apps") : tr("Signal K live • %1").arg(appsSummary);
                nextBadge = QStringLiteral("APPS");
            } else if (m_appsState == AppsState::Failed || m_appsState == AppsState::Stale) {
                nextState = RuntimeHealthState::AppsStale;
                nextSummary = appsSummary.isEmpty() ? tr("Signal K live • Apps stale") : tr("Signal K live • %1").arg(appsSummary);
                nextBadge = QStringLiteral("APPS");
            } else {
                nextState = RuntimeHealthState::ConnectedLive;
                nextSummary = connectionSummary.isEmpty() ? tr("Signal K live") : connectionSummary;
                nextBadge = QStringLiteral("LIVE");
            }
        } else if (connectionState == signalk::Client::ConnectionHealthState::Degraded) {
            nextState = RuntimeHealthState::ConnectedStale;
            nextSummary = connectionSummary.isEmpty() ? tr("Signal K degraded") : connectionSummary;
            nextBadge = QStringLiteral("DEGD");
        }

        if (m_runtimeHealthState == nextState
            && m_runtimeHealthSummary == nextSummary
            && m_runtimeHealthBadgeText == nextBadge) {
            return;
        }

        m_runtimeHealthState = nextState;
        m_runtimeHealthSummary = nextSummary;
        m_runtimeHealthBadgeText = nextBadge;
        emit runtimeHealthChanged(m_runtimeHealthState, m_runtimeHealthSummary, m_runtimeHealthBadgeText);
    }

    void FairWindSK::handleAutomaticComfortEnvironmentUpdate(const QJsonObject &update) {
        m_automaticComfortEnvironmentUpdate = update;
        const bool wasAvailable = m_automaticComfortViewAvailable;
        const QString previousPreset = getActiveComfortViewPreset();
        refreshAutomaticComfortViewAvailability();
        if (m_configuration.getComfortViewMode() == "auto"
            && (m_automaticComfortViewAvailable != wasAvailable
                || getActiveComfortViewPreset() != previousPreset)) {
            applyUiPreferences();
        }
    }

    void FairWindSK::onAutomaticComfortEnvironmentUpdate(const QJsonObject &update) {
        handleAutomaticComfortEnvironmentUpdate(update);
    }

    WebProfileHandle *FairWindSK::getWebEngineProfile() {
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
