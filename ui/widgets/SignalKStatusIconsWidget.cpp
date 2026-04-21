//
// Created by Codex on 16/04/26.
//

#include "SignalKStatusIconsWidget.hpp"

#include <QHBoxLayout>
#include <QMovie>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kIndicatorSize = 10;
        constexpr int kThrobberSize = 18;

        QString tooltipLastUpdate(const QDateTime &lastStreamUpdate) {
            if (!lastStreamUpdate.isValid()) {
                return SignalKStatusIconsWidget::tr("No live data yet");
            }

            const QDateTime localUpdate = lastStreamUpdate.toLocalTime();
            return SignalKStatusIconsWidget::tr("Last update %1").arg(localUpdate.toString(QStringLiteral("dd-MM hh:mm:ss")));
        }
    }

    SignalKStatusIconsWidget::SignalKStatusIconsWidget(QWidget *parent)
        : QWidget(parent) {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        m_serverIndicator = new QLabel(this);
        m_serverIndicator->setFixedSize(kIndicatorSize, kIndicatorSize);
        layout->addWidget(m_serverIndicator, 0, Qt::AlignVCenter);

        m_busyIndicator = new QLabel(this);
        m_busyIndicator->setFixedSize(kThrobberSize, kThrobberSize);
        m_busyIndicator->setScaledContents(true);
        layout->addWidget(m_busyIndicator, 0, Qt::AlignVCenter);

        m_restIndicator = new QLabel(this);
        m_restIndicator->setMinimumSize(44, 18);
        m_restIndicator->setMaximumSize(44, 18);
        layout->addWidget(m_restIndicator, 0, Qt::AlignVCenter);

        m_streamIndicator = new QLabel(this);
        m_streamIndicator->setMinimumSize(38, 18);
        m_streamIndicator->setMaximumSize(38, 18);
        layout->addWidget(m_streamIndicator, 0, Qt::AlignVCenter);

        m_throbber = new QMovie(QStringLiteral(":/resources/images/widgets/throbber_ajax_loader_metal_512.gif"), QByteArray(), this);
        m_throbber->setScaledSize(QSize(kThrobberSize, kThrobberSize));
        m_idleBusyPixmap = QPixmap(kThrobberSize, kThrobberSize);
        m_idleBusyPixmap.fill(Qt::transparent);

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        if (auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr) {
            connect(client, &fairwindsk::signalk::Client::serverHealthChanged,
                    this, &SignalKStatusIconsWidget::onServerHealthChanged);
            connect(client, &fairwindsk::signalk::Client::connectivityChanged,
                    this, &SignalKStatusIconsWidget::onConnectivityChanged);
            connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged,
                    this, &SignalKStatusIconsWidget::onConnectionHealthStateChanged);
            connect(client, &fairwindsk::signalk::Client::requestActivityChanged,
                    this, &SignalKStatusIconsWidget::onRequestActivityChanged);

            m_restHealthy = client->isRestHealthy();
            m_streamHealthy = client->isStreamHealthy();
            m_serverHealthy = m_restHealthy && m_streamHealthy;
            m_stateText = client->connectionHealthStateText();
            m_lastStreamUpdate = client->lastStreamUpdate();
            refreshIndicators(m_serverHealthy,
                              m_restHealthy,
                              m_streamHealthy,
                              false,
                              client->connectionStatusText());
        } else {
            refreshIndicators(false, false, false, false, QString());
        }
    }

    SignalKStatusIconsWidget::~SignalKStatusIconsWidget() = default;

    void SignalKStatusIconsWidget::refreshFromConfiguration() {
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, QString());
    }

    void SignalKStatusIconsWidget::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);

        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            refreshFromConfiguration();
        }
    }

    void SignalKStatusIconsWidget::onServerHealthChanged(const bool healthy, const QString &statusText) {
        m_serverHealthy = healthy;
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, statusText);
    }

    void SignalKStatusIconsWidget::onConnectivityChanged(const bool restHealthy,
                                                         const bool streamHealthy,
                                                         const QString &statusText) {
        m_restHealthy = restHealthy;
        m_streamHealthy = streamHealthy;
        m_serverHealthy = restHealthy && streamHealthy;
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, statusText);
    }

    void SignalKStatusIconsWidget::onRequestActivityChanged(const bool active) {
        m_requestActive = active;
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, QString());
    }

    void SignalKStatusIconsWidget::onConnectionHealthStateChanged(const fairwindsk::signalk::Client::ConnectionHealthState state,
                                                                  const QString &stateText,
                                                                  const QDateTime &lastStreamUpdate,
                                                                  const QString &statusText) {
        Q_UNUSED(state)
        m_stateText = stateText.trimmed();
        m_lastStreamUpdate = lastStreamUpdate;
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, statusText);
    }

    void SignalKStatusIconsWidget::applyIndicatorColor(QLabel *label, const QColor &color) const {
        if (!label) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QColor borderColor = fairwindsk::ui::comfortThemeColor(configuration, preset, QStringLiteral("border"), palette().color(QPalette::Mid));

        label->setStyleSheet(QStringLiteral(
            "QLabel {"
            " background: %1;"
            " border: 1px solid %2;"
            " border-radius: 5px;"
            " }").arg(color.name(), borderColor.name()));
    }

    void SignalKStatusIconsWidget::applyStatusBadge(QLabel *label,
                                                    const QString &text,
                                                    const QColor &fillColor,
                                                    const QColor &textColor) const {
        if (!label) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QColor borderColor = fairwindsk::ui::comfortThemeColor(configuration, preset, QStringLiteral("border"), palette().color(QPalette::Mid));

        label->setAlignment(Qt::AlignCenter);
        label->setText(text);
        label->setStyleSheet(QStringLiteral(
            "QLabel {"
            " background: %1;"
            " color: %2;"
            " border: 1px solid %3;"
            " border-radius: 9px;"
            " font-size: 10px;"
            " font-weight: 700;"
            " padding: 0px 4px;"
            " }").arg(fillColor.name(), textColor.name(), borderColor.name()));
    }

    void SignalKStatusIconsWidget::setBusyVisible(const bool active) {
        if (!m_busyIndicator) {
            return;
        }

        m_busyIndicator->setVisible(true);
        if (!m_throbber) {
            m_busyIndicator->setPixmap(m_idleBusyPixmap);
            return;
        }

        if (active) {
            m_busyIndicator->setMovie(m_throbber);
            if (m_throbber->state() != QMovie::Running) {
                m_throbber->start();
            }
        } else {
            if (m_throbber->state() == QMovie::Running) {
                m_throbber->stop();
            }
            m_busyIndicator->setMovie(nullptr);
            m_busyIndicator->setPixmap(m_idleBusyPixmap);
        }
    }

    void SignalKStatusIconsWidget::refreshIndicators(const bool serverHealthy,
                                                     const bool restHealthy,
                                                     const bool streamHealthy,
                                                     const bool requestActive,
                                                     const QString &statusText) {
        Q_UNUSED(statusText)

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto colors = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, palette());

        if (serverHealthy) {
            applyIndicatorColor(m_serverIndicator, colors.healthyFill);
        } else if (restHealthy || streamHealthy) {
            applyIndicatorColor(m_serverIndicator, colors.warningFill);
        } else {
            applyIndicatorColor(m_serverIndicator, colors.errorFill);
        }

        applyStatusBadge(m_restIndicator,
                         tr("REST"),
                         restHealthy ? colors.healthyFill : colors.errorFill,
                         restHealthy ? colors.healthyText : colors.errorText);
        applyStatusBadge(m_streamIndicator,
                         tr("STR"),
                         streamHealthy ? colors.healthyFill : colors.errorFill,
                         streamHealthy ? colors.healthyText : colors.errorText);

        setBusyVisible(requestActive);

        const QString overallState = m_stateText.trimmed().isEmpty() ? tr("Disconnected") : m_stateText.trimmed();
        m_serverIndicator->setToolTip(tr("Signal K %1\n%2").arg(overallState, tooltipLastUpdate(m_lastStreamUpdate)));
        m_busyIndicator->setToolTip(requestActive ? tr("Signal K activity in progress") : tr("Signal K idle"));
        m_restIndicator->setToolTip(tr("Signal K REST API"));
        m_streamIndicator->setToolTip(tr("Signal K stream\n%1").arg(tooltipLastUpdate(m_lastStreamUpdate)));
    }
}
