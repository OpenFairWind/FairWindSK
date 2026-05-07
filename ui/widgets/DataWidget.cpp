#include "DataWidget.hpp"

#include <algorithm>
#include <cmath>

#include <QBoxLayout>
#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QGeoCoordinate>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>

#include "FairWindSK.hpp"
#include "Units.hpp"
#include "signalk/Client.hpp"
#include "signalk/Waypoint.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/SignalKMetricState.hpp"

namespace fairwindsk::ui::widgets {
    namespace {
        QString fallbackNumericText(const double value) {
            return QString::number(value, 'f', std::abs(value) < 10.0 ? 2 : 1);
        }

        int gaugePercent(const double value, const double minimum, const double maximum) {
            if (maximum <= minimum) {
                return 0;
            }
            const double normalized = (value - minimum) / (maximum - minimum);
            return std::clamp(static_cast<int>(std::round(normalized * 100.0)), 0, 100);
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
        if (m_definition.kind == DataWidgetKind::Position) {
            return QSize(224, 58);
        }
        if (m_definition.kind == DataWidgetKind::DateTime ||
            m_definition.kind == DataWidgetKind::Waypoint) {
            return QSize(136, 58);
        }
        return m_definition.kind == DataWidgetKind::Gauge ? QSize(126, 74) : QSize(104, 58);
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
            qFuzzyCompare(definition.minimum, m_definition.minimum) &&
            qFuzzyCompare(definition.maximum, m_definition.maximum) &&
            definition.kind == m_definition.kind) {
            return;
        }

        unsubscribeFromSignalK();
        m_definition = definition;
        setObjectName(QStringLiteral("dataWidget_%1").arg(definition.id));
        if (m_titleLabel) {
            m_titleLabel->setText(m_definition.name);
        }
        updateIcon();
        subscribeToSignalK();
        renderCurrentUpdate();
    }

    void DataWidget::buildUi() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(6, 2, 6, 2);
        rootLayout->setSpacing(0);

        auto *titleLayout = new QHBoxLayout();
        titleLayout->setContentsMargins(0, 0, 0, 0);
        titleLayout->setSpacing(4);

        m_iconLabel = new QLabel(this);
        m_iconLabel->setFixedSize(QSize(18, 18));
        m_iconLabel->setScaledContents(true);
        titleLayout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);

        m_titleLabel = new QLabel(m_definition.name, this);
        m_titleLabel->setTextFormat(Qt::PlainText);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        titleLayout->addWidget(m_titleLabel, 1);
        rootLayout->addLayout(titleLayout);

        auto *valueLayout = new QHBoxLayout();
        valueLayout->setContentsMargins(0, 0, 0, 0);
        valueLayout->setSpacing(2);

        m_valueLabel = new QLabel(QStringLiteral("--"), this);
        m_valueLabel->setTextFormat(Qt::PlainText);
        m_valueLabel->setAlignment(Qt::AlignCenter);
        valueLayout->addWidget(m_valueLabel, 1);

        m_unitLabel = new QLabel(this);
        m_unitLabel->setTextFormat(Qt::PlainText);
        m_unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueLayout->addWidget(m_unitLabel, 0, Qt::AlignVCenter);
        rootLayout->addLayout(valueLayout);

        m_gauge = new QProgressBar(this);
        m_gauge->setRange(0, 100);
        m_gauge->setTextVisible(false);
        m_gauge->setFixedHeight(10);
        rootLayout->addWidget(m_gauge);
        updateIcon();
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
                },
                Qt::UniqueConnection);

        const QJsonObject snapshot = client->subscribe(
            m_definition.signalKPath,
            this,
            SLOT(fairwindsk::ui::widgets::DataWidget::updateFromSignalK),
            m_definition.period,
            m_definition.updatePolicy.trimmed().isEmpty() ? QStringLiteral("ideal") : m_definition.updatePolicy,
            m_definition.minPeriod);
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

    void DataWidget::renderCurrentUpdate() {
        QString text;
        bool hasValue = false;
        const bool pathConfigured = !m_definition.signalKPath.trimmed().isEmpty();

        if (m_gauge) {
            m_gauge->setVisible(m_definition.kind == DataWidgetKind::Gauge);
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
                if (m_unitLabel) {
                    m_unitLabel->setText(unitLabel());
                }
                if (m_gauge) {
                    m_gauge->setValue(hasValue ? gaugePercent(value, m_definition.minimum, m_definition.maximum) : 0);
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
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QIcon icon = fairwindsk::ui::tintedIcon(QIcon(m_definition.icon), chrome.transparentIcon, QSize(18, 18));
        m_iconLabel->setPixmap(icon.pixmap(QSize(18, 18)));
        m_iconLabel->setVisible(!icon.isNull());
    }

    void DataWidget::applyComfortChrome() {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        setStyleSheet(QStringLiteral(
            "QWidget#%1 { background: transparent; border: none; }"
            "QLabel { background: transparent; border: none; color: %2; }"
            "QProgressBar { border: 1px solid %3; border-radius: 3px; background: %4; }"
            "QProgressBar::chunk { background: %5; border-radius: 2px; }")
                          .arg(objectName(),
                               chrome.text.name(),
                               chrome.border.name(),
                               fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 28).name(QColor::HexArgb),
                               chrome.accentBottom.name()));

        if (m_titleLabel) {
            QFont titleFont = m_titleLabel->font();
            titleFont.setBold(true);
            titleFont.setPointSizeF(std::max(8.0, titleFont.pointSizeF()));
            m_titleLabel->setFont(titleFont);
        }
        if (m_valueLabel) {
            QFont valueFont = m_valueLabel->font();
            valueFont.setBold(true);
            valueFont.setPointSizeF(std::max(13.0, valueFont.pointSizeF()));
            m_valueLabel->setFont(valueFont);
        }
        updateIcon();
        renderCurrentUpdate();
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
