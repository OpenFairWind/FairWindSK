//
// Created by Codex on 02/04/26.
//

#include "SignalKServerBox.hpp"

#include <QFontMetrics>
#include <QPlainTextEdit>
#include <QTextOption>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui_SignalKServerBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kServerBoxHeight = 58;

        QString formatLastUpdateText(const QDateTime &lastStreamUpdate) {
            if (!lastStreamUpdate.isValid()) {
                return SignalKServerBox::tr("No live data yet");
            }

            const QDateTime localUpdate = lastStreamUpdate.toLocalTime();
            const qint64 ageSeconds = std::max<qint64>(0, localUpdate.secsTo(QDateTime::currentDateTime()));
            if (ageSeconds < 60) {
                return SignalKServerBox::tr("Last update %1 s ago").arg(ageSeconds);
            }
            if (ageSeconds < 3600) {
                return SignalKServerBox::tr("Last update %1 min ago").arg(ageSeconds / 60);
            }

            return SignalKServerBox::tr("Last update %1").arg(localUpdate.toString(QStringLiteral("dd-MM hh:mm")));
        }
    }

    SignalKServerBox::SignalKServerBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::SignalKServerBox) {
        ui->setupUi(this);

        setMinimumHeight(kServerBoxHeight);
        setMaximumHeight(kServerBoxHeight);
        setMinimumWidth(280);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->plainTextEditMessage->setFixedHeight(28);
        ui->plainTextEditMessage->setPlainText(tr("Waiting for server"));
        ui->plainTextEditMessage->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        ui->plainTextEditMessage->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        ui->plainTextEditMessage->document()->setDocumentMargin(0);
        ui->plainTextEditMessage->setCenterOnScroll(false);
        ui->plainTextEditMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateStatusLabel(tr("Signal K"));

        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::serverHealthChanged,
                    this, &SignalKServerBox::onServerHealthChanged);
            connect(client, &fairwindsk::signalk::Client::connectivityChanged,
                    this, &SignalKServerBox::onConnectivityChanged);
            connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged,
                    this, &SignalKServerBox::onConnectionHealthStateChanged);
            connect(client, &fairwindsk::signalk::Client::serverMessageChanged,
                    this, &SignalKServerBox::onServerMessageChanged);

            m_stateText = client->connectionHealthStateText();
            m_lastStreamUpdate = client->lastStreamUpdate();
            m_statusText = client->connectionStatusText();
            m_serverMessage = tr("Waiting for server");
            onConnectivityChanged(client->isRestHealthy(), client->isStreamHealthy(), client->connectionStatusText());
            onServerHealthChanged(client->isRestHealthy() && client->isStreamHealthy(), client->connectionStatusText());
            onConnectionHealthStateChanged(client->connectionHealthState(),
                                           client->connectionHealthStateText(),
                                           client->lastStreamUpdate(),
                                           client->connectionStatusText());
        }
        if (auto *fairWindSK = FairWindSK::getInstance()) {
            connect(fairWindSK, &fairwindsk::FairWindSK::appsStateChanged,
                    this, &SignalKServerBox::onAppsStateChanged);
            connect(fairWindSK, &fairwindsk::FairWindSK::runtimeHealthChanged,
                    this, &SignalKServerBox::onRuntimeHealthChanged);
            m_appsStateText = fairWindSK->appsStateText();
            m_runtimeSummary = fairWindSK->runtimeHealthSummary();
            m_runtimeState = fairWindSK->runtimeHealthState();
        }
        updateFreshnessMessage();
    }

    void SignalKServerBox::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        if (ui && ui->labelStatus) {
            updateStatusLabel(ui->labelStatus->toolTip());
        }
    }

    SignalKServerBox::~SignalKServerBox() {
        delete ui;
        ui = nullptr;
    }

    void SignalKServerBox::setDisplayOptions(const bool showText) {
        if (!ui) {
            return;
        }

        if (ui->labelStatus) {
            ui->labelStatus->setVisible(showText);
        }
        if (ui->plainTextEditMessage) {
            ui->plainTextEditMessage->setVisible(showText);
        }
        updateGeometry();
    }

    void SignalKServerBox::onServerHealthChanged(const bool healthy, const QString &statusText) {
        Q_UNUSED(healthy)
        m_statusText = statusText.trimmed();
        updateStatusLabel(m_statusText.isEmpty() ? tr("Signal K") : m_statusText);
    }

    void SignalKServerBox::onConnectivityChanged(const bool restHealthy, const bool streamHealthy, const QString &statusText) {
        Q_UNUSED(restHealthy)
        Q_UNUSED(streamHealthy)
        m_statusText = statusText.trimmed();
        updateStatusLabel(m_statusText.isEmpty() ? tr("Signal K") : m_statusText);
        updateFreshnessMessage();
    }

    void SignalKServerBox::onConnectionHealthStateChanged(const fairwindsk::signalk::Client::ConnectionHealthState state,
                                                          const QString &stateText,
                                                          const QDateTime &lastStreamUpdate,
                                                          const QString &statusText) {
        Q_UNUSED(state)
        m_stateText = stateText.trimmed();
        m_lastStreamUpdate = lastStreamUpdate;
        if (!statusText.trimmed().isEmpty()) {
            m_statusText = statusText.trimmed();
            updateStatusLabel(m_statusText);
        }
        updateFreshnessMessage();
    }

    void SignalKServerBox::onServerMessageChanged(const QString &message) {
        m_serverMessage = message.trimmed();
        updateFreshnessMessage();
    }

    void SignalKServerBox::onAppsStateChanged(const fairwindsk::FairWindSK::AppsState state, const QString &stateText) {
        Q_UNUSED(state)
        m_appsStateText = stateText.trimmed();
        updateFreshnessMessage();
    }

    void SignalKServerBox::onRuntimeHealthChanged(const fairwindsk::FairWindSK::RuntimeHealthState state,
                                                  const QString &summary,
                                                  const QString &badgeText) {
        Q_UNUSED(badgeText)
        m_runtimeState = state;
        m_runtimeSummary = summary.trimmed();
        updateFreshnessMessage();
    }

    void SignalKServerBox::updateStatusLabel(const QString &text) {
        if (!ui || !ui->labelStatus) {
            return;
        }

        const QString normalized = text.trimmed().isEmpty() ? tr("Signal K") : text.trimmed();
        const QString elided = ui->labelStatus->fontMetrics().elidedText(normalized, Qt::ElideRight, ui->labelStatus->width());
        ui->labelStatus->setText(elided);
        ui->labelStatus->setToolTip(normalized);
    }

    void SignalKServerBox::updateFreshnessMessage() {
        if (!ui || !ui->plainTextEditMessage) {
            return;
        }

        const QStringList lines = {
            m_serverMessage.trimmed().isEmpty() ? tr("Waiting for server") : m_serverMessage.trimmed(),
            tr("%1 • %2").arg(m_runtimeSummary.trimmed().isEmpty()
                                  ? (m_stateText.trimmed().isEmpty() ? tr("Disconnected") : m_stateText.trimmed())
                                  : m_runtimeSummary.trimmed(),
                              formatLastUpdateText(m_lastStreamUpdate)),
            tr("Shell %1 • %2")
                .arg([this]() {
                    switch (m_runtimeState) {
                        case fairwindsk::FairWindSK::RuntimeHealthState::Disconnected: return tr("disconnected");
                        case fairwindsk::FairWindSK::RuntimeHealthState::Connecting: return tr("connecting");
                        case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedLive: return tr("live");
                        case fairwindsk::FairWindSK::RuntimeHealthState::ConnectedStale: return tr("stale");
                        case fairwindsk::FairWindSK::RuntimeHealthState::Reconnecting: return tr("reconnecting");
                        case fairwindsk::FairWindSK::RuntimeHealthState::RestDegraded: return tr("REST degraded");
                        case fairwindsk::FairWindSK::RuntimeHealthState::StreamDegraded: return tr("stream degraded");
                        case fairwindsk::FairWindSK::RuntimeHealthState::AppsLoading: return tr("apps loading");
                        case fairwindsk::FairWindSK::RuntimeHealthState::AppsStale: return tr("apps stale");
                        case fairwindsk::FairWindSK::RuntimeHealthState::ForegroundAppDegraded: return tr("app degraded");
                    }
                    return tr("disconnected");
                }(),
                     m_appsStateText.trimmed().isEmpty() ? tr("Apps idle") : m_appsStateText.trimmed())
        };

        const QString text = lines.join(QLatin1Char('\n'));
        ui->plainTextEditMessage->setPlainText(text);
        ui->plainTextEditMessage->setToolTip(text);
    }
}
