#include "DataWidget.hpp"

#include <algorithm>
#include <cmath>

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGeoCoordinate>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QSizePolicy>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Units.hpp"
#include "signalk/Client.hpp"
#include "signalk/Waypoint.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/SignalKMetricState.hpp"

namespace fairwindsk::ui::widgets {
    class DataGaugeWidget final : public QWidget {
    public:
        explicit DataGaugeWidget(QWidget *parent = nullptr)
            : QWidget(parent) {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        void setRange(const double minimum, const double maximum) {
            if (qFuzzyCompare(m_minimum, minimum) && qFuzzyCompare(m_maximum, maximum)) {
                return;
            }
            m_minimum = minimum;
            m_maximum = maximum > minimum ? maximum : minimum + 1.0;
            update();
        }

        void setValue(const double value, const bool valid) {
            if (m_valid == valid && qFuzzyCompare(m_value, value)) {
                return;
            }
            m_value = value;
            m_valid = valid;
            update();
        }

        void setColors(const QColor &track,
                       const QColor &fill,
                       const QColor &needle,
                       const QColor &border) {
            m_track = track;
            m_fill = fill;
            m_needle = needle;
            m_border = border;
            update();
        }

    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);

            const QRectF bounds = rect().adjusted(1.0, 1.0, -1.0, -1.0);
            const qreal radius = std::min(bounds.height() / 2.0, 5.0);

            painter.setPen(QPen(m_border, 1.0));
            painter.setBrush(m_track);
            painter.drawRoundedRect(bounds, radius, radius);

            if (m_valid && m_maximum > m_minimum) {
                const double normalized = std::clamp((m_value - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);
                QRectF fillBounds = bounds.adjusted(2.0, 2.0, -2.0, -2.0);
                fillBounds.setWidth(std::max<qreal>(2.0, fillBounds.width() * normalized));
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_fill);
                painter.drawRoundedRect(fillBounds, std::max<qreal>(1.0, radius - 2.0), std::max<qreal>(1.0, radius - 2.0));

                const qreal needleX = bounds.left() + bounds.width() * normalized;
                painter.setPen(QPen(m_needle, 1.6));
                painter.drawLine(QPointF(needleX, bounds.top() - 1.0), QPointF(needleX, bounds.bottom() + 1.0));
            }
        }

    private:
        double m_minimum = 0.0;
        double m_maximum = 100.0;
        double m_value = 0.0;
        bool m_valid = false;
        QColor m_track = QColor(QStringLiteral("#202830"));
        QColor m_fill = QColor(QStringLiteral("#5ab8ff"));
        QColor m_needle = QColor(QStringLiteral("#ffffff"));
        QColor m_border = QColor(QStringLiteral("#6c7782"));
    };

    namespace {
        constexpr int kMetricHeight = 44;
        constexpr int kPositionRowsMetricHeight = 54;
        constexpr int kMetricRowHeight = 20;
        constexpr int kPositionRowsHeaderHeight = 16;
        constexpr int kPositionRowsValueHeight = 34;
        constexpr int kMetricIconSize = 16;
        constexpr int kMetricHorizontalPadding = 10;
        constexpr int kTrendWidth = 10;
        constexpr int kUnitWidth = 34;
        constexpr int kGaugeWidth = 48;
        constexpr int kGaugeHeight = 10;

        QString fallbackNumericText(const double value) {
            return QString::number(value, 'f', std::abs(value) < 10.0 ? 2 : 1);
        }

        int valueMaximumWidth(const DataWidgetKind kind) {
            if (kind == DataWidgetKind::Position ||
                kind == DataWidgetKind::PositionRows) {
                return 170;
            }
            if (kind == DataWidgetKind::DateTime || kind == DataWidgetKind::Waypoint) {
                return 102;
            }
            return 76;
        }

        int metricHeightForKind(const DataWidgetKind kind) {
            return kind == DataWidgetKind::PositionRows ? kPositionRowsMetricHeight : kMetricHeight;
        }

        int headerHeightForKind(const DataWidgetKind kind) {
            return kind == DataWidgetKind::PositionRows ? kPositionRowsHeaderHeight : kMetricRowHeight;
        }

        int valueHeightForKind(const DataWidgetKind kind) {
            return kind == DataWidgetKind::PositionRows ? kPositionRowsValueHeight : kMetricRowHeight;
        }

        int fontSizeOrFallback(const int configuredSize, const int fallback) {
            return configuredSize > 0 ? configuredSize : fallback;
        }

        QColor colorOrFallback(const QString &configuredColor, const QColor &fallback) {
            const QColor color(configuredColor.trimmed());
            return color.isValid() ? color : fallback;
        }
    }

    DataWidget::DataWidget(const DataWidgetDefinition &definition, QWidget *parent)
        : QWidget(parent),
          m_definition(definition),
          m_units(Units::getInstance()) {
        setObjectName(QStringLiteral("dataWidget_%1").arg(definition.id));
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        buildUi();
        applyComfortChrome();
        subscribeToSignalK();
    }

    DataWidget::~DataWidget() {
        unsubscribeFromSignalK();
    }

    QString DataWidget::widgetId() const {
        return m_definition.id;
    }

    QSize DataWidget::sizeHint() const {
        const int iconWidth = m_showIcon && !m_definition.icon.trimmed().isEmpty() ? kMetricIconSize + 4 : 0;
        const int titleWidth = m_showText ? std::min(72, std::max(34, static_cast<int>(m_definition.name.size()) * 8)) : 0;
        const int headerWidth = iconWidth + titleWidth;
        const int trendWidth = m_showTrend ? kTrendWidth + 3 : 0;
        const int unitsWidth = m_showUnits ? kUnitWidth + 3 : 0;
        const int gaugeWidth = gaugeVisible() ? kGaugeWidth + 4 : 0;
        const int valueWidth = valueMaximumWidth(m_definition.kind) + trendWidth + unitsWidth + gaugeWidth;
        return QSize(std::max(headerWidth, valueWidth) + kMetricHorizontalPadding, metricHeightForKind(m_definition.kind));
    }

    void DataWidget::setDefinition(const DataWidgetDefinition &definition) {
        if (definition.id == m_definition.id &&
            definition.name == m_definition.name &&
            definition.icon == m_definition.icon &&
            definition.signalKPath == m_definition.signalKPath &&
            definition.sourceUnit == m_definition.sourceUnit &&
            definition.defaultUnit == m_definition.defaultUnit &&
            definition.updatePolicy == m_definition.updatePolicy &&
            definition.dateTimeFormat == m_definition.dateTimeFormat &&
            definition.period == m_definition.period &&
            definition.minPeriod == m_definition.minPeriod &&
            definition.valueTextSize == m_definition.valueTextSize &&
            definition.labelTextSize == m_definition.labelTextSize &&
            definition.trendTextSize == m_definition.trendTextSize &&
            qFuzzyCompare(definition.minimum, m_definition.minimum) &&
            qFuzzyCompare(definition.maximum, m_definition.maximum) &&
            definition.visualizationMode == m_definition.visualizationMode &&
            definition.valueTextColor == m_definition.valueTextColor &&
            definition.labelTextColor == m_definition.labelTextColor &&
            definition.trendIncreasingColor == m_definition.trendIncreasingColor &&
            definition.trendDecreasingColor == m_definition.trendDecreasingColor &&
            definition.showIcon == m_definition.showIcon &&
            definition.showText == m_definition.showText &&
            definition.kind == m_definition.kind) {
            return;
        }

        unsubscribeFromSignalK();
        m_definition = definition;
        setObjectName(QStringLiteral("dataWidget_%1").arg(definition.id));
        if (m_titleLabel) {
            m_titleLabel->setText(m_definition.name);
        }
        if (m_valueLabel) {
            m_valueLabel->setMaximumWidth(valueMaximumWidth(m_definition.kind));
        }
        updateLayoutMetrics();
        updateIcon();
        subscribeToSignalK();
        applyComfortChrome();
        renderCurrentUpdate();
    }

    void DataWidget::setDisplayOptions(const bool showIcon,
                                       const bool showText,
                                       const bool showUnits,
                                       const bool showTrend) {
        const bool effectiveShowIcon = showIcon && m_definition.showIcon;
        const bool effectiveShowText = showText && m_definition.showText;
        const bool changed = m_showIcon != effectiveShowIcon ||
                             m_showText != effectiveShowText ||
                             m_showUnits != showUnits ||
                             m_showTrend != showTrend;

        m_showIcon = effectiveShowIcon;
        m_showText = effectiveShowText;
        m_showUnits = showUnits;
        m_showTrend = showTrend;
        updateHeaderVisibility();
        renderCurrentUpdate();
        if (changed) {
            updateGeometry();
        }
    }

    bool DataWidget::gaugeVisible() const {
        return m_definition.kind == DataWidgetKind::Gauge ||
               (m_definition.kind == DataWidgetKind::Numeric &&
                m_definition.visualizationMode == DataWidgetVisualizationMode::Gauge);
    }

    void DataWidget::updateLayoutMetrics() {
        const int metricHeight = metricHeightForKind(m_definition.kind);
        setMinimumHeight(metricHeight);
        setMaximumHeight(metricHeight);

        if (m_headerWidget) {
            m_headerWidget->setFixedHeight(headerHeightForKind(m_definition.kind));
        }
        if (m_titleLabel) {
            m_titleLabel->setMinimumHeight(headerHeightForKind(m_definition.kind));
        }
        if (m_valueWidget) {
            m_valueWidget->setFixedHeight(valueHeightForKind(m_definition.kind));
        }
        if (m_valueLabel) {
            m_valueLabel->setMinimumHeight(valueHeightForKind(m_definition.kind));
            m_valueLabel->setMaximumHeight(valueHeightForKind(m_definition.kind));
            m_valueLabel->setMaximumWidth(valueMaximumWidth(m_definition.kind));
        }
        updateGeometry();
    }

    void DataWidget::buildUi() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(5, 2, 5, 2);
        rootLayout->setSpacing(0);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

        m_headerWidget = new QWidget(this);
        auto *headerLayout = new QHBoxLayout(m_headerWidget);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(4);
        m_headerWidget->setFixedHeight(kMetricRowHeight);
        m_headerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        headerLayout->addStretch(1);

        m_iconLabel = new QLabel(this);
        m_iconLabel->setFixedSize(QSize(kMetricIconSize, kMetricIconSize));
        m_iconLabel->setScaledContents(true);
        headerLayout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);

        m_titleLabel = new QLabel(m_definition.name, this);
        m_titleLabel->setTextFormat(Qt::PlainText);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        m_titleLabel->setMinimumWidth(0);
        m_titleLabel->setMinimumHeight(kMetricRowHeight);
        m_titleLabel->setMaximumWidth(78);
        m_titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
        headerLayout->addStretch(1);
        rootLayout->addWidget(m_headerWidget, 0);

        m_valueWidget = new QWidget(this);
        auto *valueLayout = new QHBoxLayout(m_valueWidget);
        valueLayout->setContentsMargins(0, 0, 0, 0);
        valueLayout->setSpacing(2);
        m_valueWidget->setFixedHeight(kMetricRowHeight);
        m_valueWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        valueLayout->addStretch(1);

        m_trendLabel = new QLabel(this);
        m_trendLabel->setTextFormat(Qt::PlainText);
        m_trendLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_trendLabel->setFixedWidth(kTrendWidth);
        valueLayout->addWidget(m_trendLabel, 0, Qt::AlignVCenter);

        m_valueLabel = new QLabel(QStringLiteral("--"), this);
        m_valueLabel->setTextFormat(Qt::PlainText);
        m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_valueLabel->setMinimumWidth(0);
        m_valueLabel->setMinimumHeight(kMetricRowHeight);
        m_valueLabel->setMaximumWidth(valueMaximumWidth(m_definition.kind));
        m_valueLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        valueLayout->addWidget(m_valueLabel, 0, Qt::AlignVCenter);

        m_unitLabel = new QLabel(this);
        m_unitLabel->setTextFormat(Qt::PlainText);
        m_unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_unitLabel->setMinimumWidth(0);
        m_unitLabel->setMaximumWidth(kUnitWidth);
        m_unitLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        valueLayout->addWidget(m_unitLabel, 0, Qt::AlignVCenter);

        m_gauge = new DataGaugeWidget(this);
        m_gauge->setFixedSize(QSize(kGaugeWidth, kGaugeHeight));
        valueLayout->addWidget(m_gauge, 0, Qt::AlignVCenter);
        valueLayout->addStretch(1);
        rootLayout->addWidget(m_valueWidget, 0);
        updateLayoutMetrics();
        updateHeaderVisibility();
    }

    void DataWidget::subscribeToSignalK() {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        if (!client || m_definition.signalKPath.trimmed().isEmpty()) {
            renderCurrentUpdate();
            return;
        }

        m_client = client;
        connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged, this,
                [this](fairwindsk::signalk::Client::ConnectionHealthState,
                       const QString &,
                       const QDateTime &,
                       const QString &) {
                    renderCurrentUpdate();
                });

        const QJsonObject snapshot = client->subscribe(
            m_definition.signalKPath,
            this,
            SLOT(updateFromSignalK(QJsonObject)),
            m_definition.period,
            m_definition.updatePolicy.trimmed().isEmpty() ? QStringLiteral("instant") : m_definition.updatePolicy,
            m_definition.minPeriod,
            false);
        updateFromSignalK(snapshot);
    }

    void DataWidget::unsubscribeFromSignalK() {
        if (!m_client) {
            return;
        }

        if (!m_definition.signalKPath.trimmed().isEmpty()) {
            m_client->removeSubscription(m_definition.signalKPath, this);
        }
        disconnect(m_client, nullptr, this, nullptr);
        m_client.clear();
    }

    void DataWidget::updateFromSignalK(const QJsonObject &update) {
        m_lastUpdate = update;
        renderCurrentUpdate();
    }

    QString DataWidget::formattedNumericValue(const double value) const {
        if (!m_units) {
            return fallbackNumericText(value);
        }

        if (!m_definition.sourceUnit.trimmed().isEmpty() && !m_definition.defaultUnit.trimmed().isEmpty()) {
            const QString formatted = m_units->formatSignalKValue(
                m_definition.signalKPath,
                value,
                m_definition.sourceUnit,
                m_definition.defaultUnit);
            return formatted.trimmed().isEmpty() ? fallbackNumericText(value) : formatted;
        }

        if (!m_definition.defaultUnit.trimmed().isEmpty()) {
            const QString formatted = m_units->format(m_definition.defaultUnit, value);
            return formatted.trimmed().isEmpty() ? fallbackNumericText(value) : formatted;
        }

        return fallbackNumericText(value);
    }

    QString DataWidget::unitLabel() const {
        if (!m_units || m_definition.defaultUnit.trimmed().isEmpty()) {
            return {};
        }
        return m_units->getSignalKUnitLabel(m_definition.signalKPath, m_definition.defaultUnit);
    }

    void DataWidget::updateTrend(const double value, const bool hasValue) {
        if (!hasValue || std::isnan(value)) {
            m_trendDirection = 0;
            m_lastTrendValue = std::numeric_limits<double>::quiet_NaN();
            return;
        }

        if (!std::isnan(m_lastTrendValue)) {
            const double delta = value - m_lastTrendValue;
            if (std::abs(delta) > 0.0001) {
                m_trendDirection = delta > 0.0 ? 1 : -1;
            }
        }
        m_lastTrendValue = value;
    }

    QString DataWidget::trendText() const {
        if (m_trendDirection > 0) {
            return QStringLiteral("▲");
        }
        if (m_trendDirection < 0) {
            return QStringLiteral("▼");
        }
        return {};
    }

    void DataWidget::renderCurrentUpdate() {
        if (m_rendering) {
            return;
        }
        m_rendering = true;

        QString text;
        bool hasValue = false;
        bool supportsUnits = false;
        bool supportsTrend = false;
        const bool pathConfigured = !m_definition.signalKPath.trimmed().isEmpty();

        if (m_gauge) {
            m_gauge->setVisible(gaugeVisible());
        }

        switch (m_definition.kind) {
            case DataWidgetKind::Position: {
                const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(
                    m_lastUpdate,
                    m_definition.signalKPath);
                hasValue = pathConfigured && value.isValid();
                if (hasValue) {
                    const auto *configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
                    text = fairwindsk::ui::geo::formatCoordinate(value, configuration->getCoordinateFormat());
                }
                if (m_unitLabel) {
                    m_unitLabel->clear();
                }
                break;
            }
            case DataWidgetKind::PositionRows: {
                const auto value = fairwindsk::signalk::Client::getGeoCoordinateFromUpdateByPath(
                    m_lastUpdate,
                    m_definition.signalKPath);
                hasValue = pathConfigured && value.isValid();
                if (hasValue) {
                    const auto *configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
                    const QString formatId = configuration->getCoordinateFormat();
                    text = QStringLiteral("%1\n%2").arg(
                        fairwindsk::ui::geo::formatSingleCoordinate(value.latitude(), true, formatId),
                        fairwindsk::ui::geo::formatSingleCoordinate(value.longitude(), false, formatId));
                }
                if (m_unitLabel) {
                    m_unitLabel->clear();
                }
                break;
            }
            case DataWidgetKind::DateTime: {
                const auto value = fairwindsk::signalk::Client::getDateTimeFromUpdateByPath(
                    m_lastUpdate,
                    m_definition.signalKPath);
                hasValue = pathConfigured && value.isValid() && !value.isNull();
                const QString format = m_definition.dateTimeFormat.trimmed().isEmpty()
                                           ? QStringLiteral("dd-MM hh:mm")
                                           : m_definition.dateTimeFormat;
                text = hasValue ? value.toLocalTime().toString(format) : QString();
                if (m_unitLabel) {
                    m_unitLabel->clear();
                }
                break;
            }
            case DataWidgetKind::Waypoint: {
                const auto value = fairwindsk::signalk::Client::getObjectFromUpdateByPath(
                    m_lastUpdate,
                    m_definition.signalKPath);
                if (value.contains(QStringLiteral("href")) && value[QStringLiteral("href")].isString()) {
                    const QString href = value[QStringLiteral("href")].toString();
                    auto *client = fairwindsk::FairWindSK::getInstance()->getSignalKClient();
                    text = client ? client->getWaypointByHref(href).getName().trimmed() : QString();
                    hasValue = !text.trimmed().isEmpty();
                }
                if (m_unitLabel) {
                    m_unitLabel->clear();
                }
                break;
            }
            case DataWidgetKind::Gauge:
            case DataWidgetKind::Numeric:
            default: {
                const double value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(
                    m_lastUpdate,
                    m_definition.signalKPath);
                hasValue = pathConfigured && !m_lastUpdate.isEmpty() && !std::isnan(value);
                text = hasValue ? formattedNumericValue(value) : QString();
                supportsUnits = true;
                supportsTrend = true;
                updateTrend(value, hasValue);
                if (m_unitLabel) {
                    m_unitLabel->setText(unitLabel());
                }
                if (m_gauge) {
                    m_gauge->setRange(m_definition.minimum, m_definition.maximum);
                    m_gauge->setValue(value, hasValue);
                }
                break;
            }
        }

        fairwindsk::ui::widgets::applySignalKMetricPresentation(
            m_valueLabel,
            m_unitLabel,
            this,
            m_definition.name,
            text,
            fairwindsk::ui::widgets::signalKMetricState(hasValue, pathConfigured),
            pathConfigured,
            true);

        if (m_valueLabel) {
            if (m_definition.kind == DataWidgetKind::Numeric ||
                m_definition.kind == DataWidgetKind::Gauge) {
                m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else if (m_definition.kind == DataWidgetKind::PositionRows) {
                m_valueLabel->setAlignment(Qt::AlignCenter);
            } else {
                m_valueLabel->setAlignment(Qt::AlignCenter);
            }
        }
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QPalette referencePalette = qApp ? qApp->palette() : palette();
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, referencePalette, false);
        const QColor valueTextColor = hasValue
                                          ? colorOrFallback(m_definition.valueTextColor, chrome.text)
                                          : chrome.disabledText;
        fairwindsk::ui::widgets::applySignalKMetricLabelColor(m_valueLabel, valueTextColor);
        fairwindsk::ui::widgets::applySignalKMetricLabelColor(m_unitLabel, valueTextColor);
        if (m_gauge) {
            m_gauge->setColors(fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 72),
                               valueTextColor,
                               colorOrFallback(m_definition.labelTextColor, chrome.text),
                               chrome.border);
        }
        updateHeaderVisibility();
        if (m_unitLabel) {
            m_unitLabel->setVisible(m_showUnits && supportsUnits && !m_unitLabel->text().trimmed().isEmpty());
        }
        if (m_trendLabel) {
            const QString trend = trendText();
            m_trendLabel->setText(trend);
            if (!trend.isEmpty()) {
                const auto statusColors = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, referencePalette);
                fairwindsk::ui::widgets::applySignalKMetricLabelColor(
                    m_trendLabel,
                    m_trendDirection > 0
                        ? colorOrFallback(m_definition.trendIncreasingColor, statusColors.healthyFill)
                        : colorOrFallback(m_definition.trendDecreasingColor, statusColors.errorFill));
            }
            m_trendLabel->setVisible(m_showTrend && supportsTrend && hasValue && !trend.isEmpty());
        }
        m_rendering = false;
    }

    void DataWidget::updateIcon() {
        if (!m_iconLabel) {
            return;
        }
        if (m_definition.icon.trimmed().isEmpty()) {
            m_iconLabel->hide();
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QPalette referencePalette = qApp ? qApp->palette() : palette();
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, referencePalette, false);
        const QColor iconColor = colorOrFallback(m_definition.labelTextColor, chrome.transparentIcon);
        const QIcon icon = fairwindsk::ui::tintedIcon(QIcon(m_definition.icon), iconColor, QSize(kMetricIconSize, kMetricIconSize));
        m_iconLabel->setPixmap(icon.pixmap(QSize(kMetricIconSize, kMetricIconSize)));
        m_iconLabel->setVisible(m_showIcon && !icon.isNull());
    }

    void DataWidget::updateHeaderVisibility() {
        updateIcon();

        const bool iconEnabled = m_iconLabel && !m_iconLabel->isHidden();
        const bool textEnabled = m_showText && !m_definition.name.trimmed().isEmpty();

        if (m_titleLabel) {
            m_titleLabel->setText(m_definition.name);
            m_titleLabel->setAlignment(Qt::AlignCenter);
            m_titleLabel->setVisible(textEnabled);
        }
        if (m_headerWidget) {
            m_headerWidget->setVisible(iconEnabled || textEnabled);
        }
    }

    void DataWidget::applyComfortChrome() {
        // setStyleSheet() fires StyleChange/FontChange/PaletteChange events which
        // would recurse back here via changeEvent(); skip the re-entrant call.
        if (m_applyingChrome) {
            return;
        }
        m_applyingChrome = true;

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QPalette referencePalette = qApp ? qApp->palette() : palette();
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, referencePalette, false);
        const QColor labelColor = colorOrFallback(m_definition.labelTextColor, chrome.text);

        setStyleSheet(QStringLiteral(
            "QWidget#%1 { background: transparent; border: none; }"
            "QLabel { background: transparent; border: none; color: %2; }")
                          .arg(objectName(),
                               labelColor.name()));

        if (m_titleLabel) {
            QFont titleFont = m_titleLabel->font();
            titleFont.setBold(true);
            titleFont.setPointSize(fontSizeOrFallback(m_definition.labelTextSize, 10));
            m_titleLabel->setFont(titleFont);
            fairwindsk::ui::widgets::applySignalKMetricLabelColor(m_titleLabel, labelColor);
        }
        if (m_valueLabel) {
            QFont valueFont = m_valueLabel->font();
            valueFont.setBold(true);
            valueFont.setPointSize(fontSizeOrFallback(m_definition.valueTextSize, 11));
            m_valueLabel->setFont(valueFont);
        }
        if (m_unitLabel) {
            QFont unitFont = m_unitLabel->font();
            unitFont.setPointSize(std::max(8, fontSizeOrFallback(m_definition.valueTextSize, 11) - 2));
            m_unitLabel->setFont(unitFont);
        }
        if (m_trendLabel) {
            QFont trendFont = m_trendLabel->font();
            trendFont.setBold(true);
            trendFont.setPointSize(fontSizeOrFallback(m_definition.trendTextSize, 10));
            m_trendLabel->setFont(trendFont);
            const QFontMetrics metrics(trendFont);
            m_trendLabel->setFixedWidth(std::max(kTrendWidth, metrics.horizontalAdvance(QStringLiteral("▲")) + 4));
        }
        if (m_gauge) {
            m_gauge->setColors(fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 72),
                               colorOrFallback(m_definition.valueTextColor, chrome.accentBottom),
                               labelColor,
                               chrome.border);
        }
        updateIcon();
        renderCurrentUpdate();
        m_applyingChrome = false;
    }

    void DataWidget::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event && (event->type() == QEvent::PaletteChange ||
                      event->type() == QEvent::ApplicationPaletteChange ||
                      event->type() == QEvent::StyleChange ||
                      event->type() == QEvent::FontChange)) {
            applyComfortChrome();
        }
    }
}
