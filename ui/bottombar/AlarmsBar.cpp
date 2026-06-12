//
// Created by Raffaele Montella on 03/06/24.
//

#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

#include "AlarmsBar.hpp"

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"
#include "ui_AlarmsBar.h"

namespace fairwindsk::ui::bottombar {
    namespace {
        QString alarmConnectionLabel(const fairwindsk::signalk::Client::ConnectionHealthState state) {
            switch (state) {
                case fairwindsk::signalk::Client::ConnectionHealthState::Connecting:
                    return AlarmsBar::tr("Connecting");
                case fairwindsk::signalk::Client::ConnectionHealthState::Live:
                    return AlarmsBar::tr("Live");
                case fairwindsk::signalk::Client::ConnectionHealthState::Stale:
                    return AlarmsBar::tr("Stale");
                case fairwindsk::signalk::Client::ConnectionHealthState::Reconnecting:
                    return AlarmsBar::tr("Reconnecting");
                case fairwindsk::signalk::Client::ConnectionHealthState::Degraded:
                    return AlarmsBar::tr("Degraded");
                case fairwindsk::signalk::Client::ConnectionHealthState::Disconnected:
                default:
                    return AlarmsBar::tr("Disconnected");
            }
        }
    }

    AlarmsBar::AlarmsBar(QWidget *parent) : QWidget(parent), ui(new Ui::AlarmsBar) {

        ui->setupUi(this);

        m_alarmToolButtons["abandon"] = ui->toolButton_Abandon;
        m_alarmToolButtons["adrift"] = ui->toolButton_Adrift;
        m_alarmToolButtons["fire"] = ui->toolButton_Fire;
        m_alarmToolButtons["mob"] = ui->toolButton_POB;
        m_alarmToolButtons["piracy"] = ui->toolButton_Piracy;
        m_alarmToolButtons["sinking"] = ui->toolButton_Sinking;

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        updateNotifications(client->subscribe("notifications.*", this,
                                              SLOT(fairwindsk::ui::bottombar::AlarmsBar::updateNotifications)));
        connect(client, &fairwindsk::signalk::Client::connectionHealthStateChanged, this,
                [this](const fairwindsk::signalk::Client::ConnectionHealthState state,
                       const QString &,
                       const QDateTime &lastStreamUpdate,
                       const QString &statusText) {
                    m_connectionState = state;
                    m_connectionStatusText = statusText;
                    m_lastStreamUpdate = lastStreamUpdate;
                    updateControlTooltips();
                });

        connect(ui->toolButton_POB, &QPushButton::clicked, this, &AlarmsBar::onPobClicked);
        connect(ui->toolButton_Sinking, &QPushButton::clicked, this, &AlarmsBar::onSinkingClicked);
        connect(ui->toolButton_Fire, &QPushButton::clicked, this, &AlarmsBar::onFireClicked);
        connect(ui->toolButton_Piracy, &QPushButton::clicked, this, &AlarmsBar::onPiracyClicked);
        connect(ui->toolButton_Abandon, &QPushButton::clicked, this, &AlarmsBar::onAbandonClicked);
        connect(ui->toolButton_Adrift, &QPushButton::clicked, this, &AlarmsBar::onAdriftClicked);
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AlarmsBar::onHideClicked);

        applyComfortStyle();
        updateControlTooltips();
        QWidget::setVisible(false);
    }

    void AlarmsBar::refreshFromConfiguration() {
        applyComfortStyle();
    }

    void AlarmsBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            applyComfortStyle();
        }
    }

    QString AlarmsBar::alarmApiKey(const QString &alarm) const {
        if (alarm == "pob") {
            return "mob";
        }
        return alarm;
    }

    QString AlarmsBar::alarmUiKey(const QString &apiKey) const {
        if (apiKey == "mob") {
            return "pob";
        }
        return apiKey;
    }

    void AlarmsBar::setAlarmState(const QString &apiKey, const bool active) {
        if (!m_alarmToolButtons.contains(apiKey)) {
            return;
        }

        m_alarmToolButtons[apiKey]->setChecked(active);
        applyComfortStyle();
        updateControlTooltips();
        emit alarmed(alarmUiKey(apiKey), active);
    }

    void AlarmsBar::applyComfortStyle() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        for (auto *button : findChildren<QToolButton *>()) {
            const bool isTransparentIconButton =
                button == ui->toolButton_Alarms || button == ui->toolButton_Hide;
            const bool criticalAlarmActive = button->isCheckable() && button->isChecked() && !isTransparentIconButton;
            fairwindsk::ui::applyBottomBarToolButtonChrome(
                button,
                colors,
                isTransparentIconButton
                    ? fairwindsk::ui::BottomBarButtonChrome::Transparent
                    : (criticalAlarmActive
                           ? fairwindsk::ui::BottomBarButtonChrome::Accent
                           : fairwindsk::ui::BottomBarButtonChrome::Flat),
                QSize(40, 40),
                88);
        }
    }

    void AlarmsBar::updateControlTooltips() {
        const QString connectionLabel = alarmConnectionLabel(m_connectionState);
        QString connectionLine = tr("Signal K %1").arg(connectionLabel);
        if (!m_connectionStatusText.trimmed().isEmpty()) {
            connectionLine += tr("\n%1").arg(m_connectionStatusText.trimmed());
        }
        if (m_lastStreamUpdate.isValid()) {
            connectionLine += tr("\nLast live update %1")
                                  .arg(m_lastStreamUpdate.toLocalTime().toString(QStringLiteral("dd-MM hh:mm:ss")));
        }

        if (ui->toolButton_Alarms) {
            ui->toolButton_Alarms->setToolTip(tr("Alarm controls\n%1").arg(connectionLine));
        }
        if (ui->toolButton_Hide) {
            ui->toolButton_Hide->setToolTip(tr("Close alarm controls"));
        }

        for (auto it = m_alarmToolButtons.cbegin(); it != m_alarmToolButtons.cend(); ++it) {
            if (!it.value()) {
                continue;
            }

            QString tooltip = tr("%1 alarm").arg(alarmUiKey(it.key()).toUpper());
            tooltip += it.value()->isChecked() ? tr(": active") : tr(": inactive");
            tooltip += tr("\n%1").arg(connectionLine);
            it.value()->setToolTip(tooltip);
        }
    }

    void AlarmsBar::onHideClicked() {
        setVisible(false);
        emit hidden();
    }

    void AlarmsBar::onAbandonClicked() { onAlarm("abandon"); }
    void AlarmsBar::onAdriftClicked() { onAlarm("adrift"); }
    void AlarmsBar::onFireClicked() { onAlarm("fire"); }
    void AlarmsBar::onPobClicked() { onAlarm("pob"); }
    void AlarmsBar::onPiracyClicked() { onAlarm("piracy"); }
    void AlarmsBar::onSinkingClicked() { onAlarm("sinking"); }

    void AlarmsBar::onAlarm(const QString& alarm) {
        const QString apiKey = alarmApiKey(alarm);
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const auto notificationUrl = QUrl(client->server().toString() + "/signalk/v2/api/notifications/" + apiKey);

        if (m_alarmToolButtons.value(apiKey)->isChecked()) {
            client->signalkDelete(notificationUrl);
            setAlarmState(apiKey, false);
            return;
        }

        QString payload = QString(R"({"message":"%1"})").arg(alarm.toUpper());
        client->signalkPut(notificationUrl, payload);
        setAlarmState(apiKey, true);
    }

    void AlarmsBar::updateNotifications(const QJsonObject &update) {
        if (update.isEmpty() || !update.contains("updates") || !update["updates"].isArray()) {
            return;
        }

        for (const auto &updateItem : update["updates"].toArray()) {
            if (!updateItem.isObject()) {
                continue;
            }
            const auto updateItemObject = updateItem.toObject();
            if (!updateItemObject.contains("values") || !updateItemObject["values"].isArray()) {
                continue;
            }

            for (const auto &value : updateItemObject["values"].toArray()) {
                if (!value.isObject()) {
                    continue;
                }

                const auto valueObject = value.toObject();
                const QString path = valueObject["path"].toString();
                if (!path.startsWith("notifications.")) {
                    continue;
                }

                const QStringList pathParts = path.split('.');
                if (pathParts.size() < 2) {
                    continue;
                }

                const QString apiKey = pathParts.at(1);
                if (!m_alarmToolButtons.contains(apiKey)) {
                    continue;
                }

                bool emergency = false;
                if (valueObject.contains("value") && valueObject["value"].isObject()) {
                    const auto notificationValue = valueObject["value"].toObject();
                    if (notificationValue.contains("state") && notificationValue["state"].isString()) {
                        const QString state = notificationValue["state"].toString();
                        emergency = (state == "emergency" || state == "alarm");
                    }
                }

                setAlarmState(apiKey, emergency);
            }
        }
    }

    AlarmsBar::~AlarmsBar() {
        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }
} // fairwindsk::ui::bottombar
