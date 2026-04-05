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

        connect(ui->toolButton_POB, &QPushButton::clicked, this, &AlarmsBar::onPobClicked);
        connect(ui->toolButton_Sinking, &QPushButton::clicked, this, &AlarmsBar::onSinkingClicked);
        connect(ui->toolButton_Fire, &QPushButton::clicked, this, &AlarmsBar::onFireClicked);
        connect(ui->toolButton_Piracy, &QPushButton::clicked, this, &AlarmsBar::onPiracyClicked);
        connect(ui->toolButton_Abandon, &QPushButton::clicked, this, &AlarmsBar::onAbandonClicked);
        connect(ui->toolButton_Adrift, &QPushButton::clicked, this, &AlarmsBar::onAdriftClicked);
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AlarmsBar::onHideClicked);

        applyComfortStyle();
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
        emit alarmed(alarmUiKey(apiKey), active);
    }

    void AlarmsBar::applyComfortStyle() const {
        const QColor buttonColor = palette().color(QPalette::Button);
        const QColor borderColor = buttonColor.darker(140);
        const QColor hoverColor = buttonColor.lighter(110);
        const QColor pressedColor = buttonColor.darker(118);
        const QColor iconColor = fairwindsk::ui::bestContrastingColor(
            buttonColor,
            {palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText),
             palette().color(QPalette::Text),
             QColor(QStringLiteral("#f8f8f8")),
             QColor(QStringLiteral("#111111"))});
        const QString style = QStringLiteral(
            "QToolButton {"
            " border: 1px solid %1;"
            " border-radius: 8px;"
            " padding: 6px;"
            " background: %2;"
            " color: %3;"
            " }"
            "QToolButton:hover { background: %4; }"
            "QToolButton:pressed, QToolButton:checked { background: %5; color: %3; }")
            .arg(borderColor.name(), buttonColor.name(), iconColor.name(), hoverColor.name(), pressedColor.name());

        for (auto *button : findChildren<QToolButton *>()) {
            button->setAutoRaise(false);
            button->setStyleSheet(style);
            if (!button->iconSize().isValid()) {
                button->setIconSize(QSize(32, 32));
            }
            fairwindsk::ui::applyTintedButtonIcon(button, iconColor, QSize(32, 32));
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
