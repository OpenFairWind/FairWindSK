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
        constexpr int kIndicatorWidth = 48;
        constexpr int kIndicatorHeight = 18;
        constexpr int kThrobberSize = 18;

        QString tooltipLastUpdate(const QDateTime &lastStreamUpdate) {
            if (!lastStreamUpdate.isValid()) {
                return SignalKStatusIconsWidget::tr("No live data yet");
            }

            const QDateTime localUpdate = lastStreamUpdate.toLocalTime();
            return SignalKStatusIconsWidget::tr("Last update %1").arg(localUpdate.toString(QStringLiteral("dd-MM hh:mm:ss")));
        }

        QString runtimeStateLabel(const fairwindsk::FairWindSK::RuntimeHealthState state) {
            switch (state) {
                case fairwindsk::FairWindSK::RuntimeHealthState::Disconnected:
                    return SignalKStatusIconsWidget::tr("Disconnected");
                case fairwindsk::FairWindSK::RuntimeHealthState::Connecting:
                    return SignalKStatusIconsWidget::tr("Connecting");
                case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedLive:
                    return SignalKStatusIconsWidget::tr("Live");
                case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedStale:
                    return SignalKStatusIconsWidget::tr("Stale");
                case fairwindsk::FairWindSK::RuntimeHealthState::Reconnecting:
                    return SignalKStatusIconsWidget::tr("Reconnecting");
                case fairwindsk::FairWindSK::RuntimeHealthState::RestDegraded:
                    return SignalKStatusIconsWidget::tr("REST degraded");
                case fairwindsk::FairWindSK::RuntimeHealthState::StreamDegraded:
                    return SignalKStatusIconsWidget::tr("Stream degraded");
                case fairwindsk::FairWindSK::RuntimeHealthState::AppsLoading:
                    return SignalKStatusIconsWidget::tr("Apps loading");
                case fairwindsk::FairWindSK::RuntimeHealthState::AppsStale:
                    return SignalKStatusIconsWidget::tr("Apps stale");
                case fairwindsk::FairWindSK::RuntimeHealthState::ForegroundAppDegraded:
                    return SignalKStatusIconsWidget::tr("Foreground app degraded");
            }

            return SignalKStatusIconsWidget::tr("Disconnected");
        }
    }

    SignalKStatusIconsWidget::SignalKStatusIconsWidget(QWidget *parent)
        : QWidget(parent) {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        m_serverIndicator = new QLabel(this);
        m_serverIndicator->setMinimumSize(kIndicatorWidth, kIndicatorHeight);
        m_serverIndicator->setMaximumSize(kIndicatorWidth, kIndicatorHeight);
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
            m_runtimeSummary = fairWindSK ? fairWindSK->runtimeHealthSummary() : m_stateText;
            m_runtimeBadgeText = fairWindSK ? fairWindSK->runtimeHealthBadgeText() : tr("DISC");
            m_runtimeState = fairWindSK ? fairWindSK->runtimeHealthState()
                                        : fairwindsk::FairWindSK::RuntimeHealthState::Disconnected;
            refreshIndicators(m_serverHealthy,
                              m_restHealthy,
                              m_streamHealthy,
                              false,
                              client->connectionStatusText());
        } else {
            refreshIndicators(false, false, false, false, QString());
        }

        if (fairWindSK) {
            connect(fairWindSK, &fairwindsk::FairWindSK::appsStateChanged,
                    this, &SignalKStatusIconsWidget::onAppsStateChanged);
            connect(fairWindSK, &fairwindsk::FairWindSK::runtimeHealthChanged,
                    this, &SignalKStatusIconsWidget::onRuntimeHealthChanged);
            m_appsStateText = fairWindSK->appsStateText();
            m_runtimeSummary = fairWindSK->runtimeHealthSummary();
            m_runtimeBadgeText = fairWindSK->runtimeHealthBadgeText();
            m_runtimeState = fairWindSK->runtimeHealthState();
        }
    }

    SignalKStatusIconsWidget::~SignalKStatusIconsWidget() = default;

    void SignalKStatusIconsWidget::setDetailIndicatorsVisible(const bool visible) {
        if (m_detailIndicatorsVisible == visible) {
            return;
        }

        m_detailIndicatorsVisible = visible;
        refreshDetailIndicatorVisibility();
    }

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

    void SignalKStatusIconsWidget::onAppsStateChanged(const fairwindsk::FairWindSK::AppsState state,
                                                      const QString &stateText) {
        Q_UNUSED(state)
        m_appsStateText = stateText.trimmed();
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

    void SignalKStatusIconsWidget::onRuntimeHealthChanged(const fairwindsk::FairWindSK::RuntimeHealthState state,
                                                          const QString &summary,
                                                          const QString &badgeText) {
        m_runtimeState = state;
        m_runtimeSummary = summary.trimmed();
        m_runtimeBadgeText = badgeText.trimmed();
        refreshIndicators(m_serverHealthy, m_restHealthy, m_streamHealthy, m_requestActive, QString());
    }

    void SignalKStatusIconsWidget::applyIndicatorColor(QLabel *label, const QColor &color) const {
        if (!label) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QColor borderColor = fairwindsk::ui::comfortThemeColor(configuration, preset, QStringLiteral("border"), palette().color(QPalette::Mid));

        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QStringLiteral(
            "QLabel {"
            " background: %1;"
            " color: %2;"
            " border: 1px solid %3;"
            " border-radius: 9px;"
            " font-size: 10px;"
            " font-weight: 700;"
            " padding: 0px 4px;"
            " }").arg(color.name(),
                      fairwindsk::ui::bestContrastingColor(color, {palette().color(QPalette::WindowText), palette().color(QPalette::Text)}).name(),
                      borderColor.name()));
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

    void SignalKStatusIconsWidget::refreshDetailIndicatorVisibility() const {
        if (m_restIndicator) {
            m_restIndicator->setVisible(m_detailIndicatorsVisible);
        }
        if (m_streamIndicator) {
            m_streamIndicator->setVisible(m_detailIndicatorsVisible);
        }
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

        QColor runtimeFill = colors.errorFill;
        switch (m_runtimeState) {
            case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedLive:
                runtimeFill = colors.healthyFill;
                break;
            case fairwindsk::FairWindSK::RuntimeHealthState::Connecting:
            case fairwindsk::FairWindSK::RuntimeHealthState::Reconnecting:
            case fairwindsk::FairWindSK::RuntimeHealthState::AppsLoading:
                runtimeFill = colors.warningFill;
                break;
            case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedStale:
            case fairwindsk::FairWindSK::RuntimeHealthState::RestDegraded:
            case fairwindsk::FairWindSK::RuntimeHealthState::StreamDegraded:
            case fairwindsk::FairWindSK::RuntimeHealthState::AppsStale:
            case fairwindsk::FairWindSK::RuntimeHealthState::ForegroundAppDegraded:
                runtimeFill = colors.warningFill.darker(112);
                break;
            case fairwindsk::FairWindSK::RuntimeHealthState::Disconnected:
                runtimeFill = colors.errorFill;
                break;
        }
        applyIndicatorColor(m_serverIndicator, runtimeFill);
        m_serverIndicator->setText(m_runtimeBadgeText.trimmed().isEmpty() ? tr("DISC") : m_runtimeBadgeText.trimmed());

        applyStatusBadge(m_restIndicator,
                         tr("REST"),
                         restHealthy ? colors.healthyFill : colors.errorFill,
                         restHealthy ? colors.healthyText : colors.errorText);
        applyStatusBadge(m_streamIndicator,
                         tr("STR"),
                         streamHealthy ? colors.healthyFill : colors.errorFill,
                         streamHealthy ? colors.healthyText : colors.errorText);

        setBusyVisible(requestActive);

        const QString overallState = m_runtimeSummary.trimmed().isEmpty()
                                         ? (m_stateText.trimmed().isEmpty() ? tr("Disconnected") : m_stateText.trimmed())
                                         : m_runtimeSummary.trimmed();
        const QString appsState = m_appsStateText.trimmed().isEmpty() ? tr("Apps idle") : m_appsStateText.trimmed();
        m_serverIndicator->setToolTip(tr("Shell health: %1\n%2\n%3")
                                          .arg(runtimeStateLabel(m_runtimeState), overallState, appsState));
        m_busyIndicator->setToolTip(requestActive ? tr("Signal K activity in progress") : tr("Signal K idle"));
        m_restIndicator->setToolTip(tr("Signal K REST API\n%1").arg(restHealthy ? tr("Healthy") : tr("Unavailable")));
        m_streamIndicator->setToolTip(tr("Signal K stream\n%1\n%2").arg(streamHealthy ? tr("Healthy") : tr("Unavailable"),
                                                                         tooltipLastUpdate(m_lastStreamUpdate)));
        refreshDetailIndicatorVisibility();
    }
}
