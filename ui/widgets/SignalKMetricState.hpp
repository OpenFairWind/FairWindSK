#ifndef FAIRWINDSK_SIGNALKMETRICSTATE_HPP
#define FAIRWINDSK_SIGNALKMETRICSTATE_HPP

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QWidget>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {

    enum class SignalKMetricState {
        Live,
        Stale,
        Missing
    };

    inline SignalKMetricState signalKMetricState(const bool hasValue, const bool pathConfigured = true) {
        if (!pathConfigured || !hasValue) {
            return SignalKMetricState::Missing;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;
        if (!client) {
            return SignalKMetricState::Stale;
        }

        return client->connectionHealthState() == fairwindsk::signalk::Client::ConnectionHealthState::Live
                   ? SignalKMetricState::Live
                   : SignalKMetricState::Stale;
    }

    inline QString signalKMetricStateLabel(const SignalKMetricState state, const bool pathConfigured = true) {
        if (!pathConfigured) {
            return QCoreApplication::translate("SignalKMetricState", "Not configured");
        }

        switch (state) {
            case SignalKMetricState::Live:
                return QCoreApplication::translate("SignalKMetricState", "Live");
            case SignalKMetricState::Stale:
                return QCoreApplication::translate("SignalKMetricState", "Stale");
            case SignalKMetricState::Missing:
                return QCoreApplication::translate("SignalKMetricState", "Missing");
        }

        return QCoreApplication::translate("SignalKMetricState", "Missing");
    }

    inline QString signalKMetricTooltip(const QString &title,
                                        const SignalKMetricState state,
                                        const bool pathConfigured = true,
                                        const QString &detail = QString()) {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *client = fairWindSK ? fairWindSK->getSignalKClient() : nullptr;

        QString tooltip = QCoreApplication::translate("SignalKMetricState", "%1: %2")
                              .arg(title, signalKMetricStateLabel(state, pathConfigured));
        if (!detail.trimmed().isEmpty()) {
            tooltip += QStringLiteral("\n") + detail.trimmed();
        }
        if (client && client->lastStreamUpdate().isValid()) {
            tooltip += QCoreApplication::translate("SignalKMetricState", "\nLast live update %1")
                           .arg(client->lastStreamUpdate().toLocalTime().toString(QStringLiteral("dd-MM hh:mm:ss")));
        }
        return tooltip;
    }

    inline QPalette signalKMetricPalette(const QLabel *label, const QWidget *container) {
        if (qApp) {
            return qApp->palette();
        }
        if (container) {
            return container->palette();
        }
        return label ? label->palette() : QPalette();
    }

    inline void applySignalKMetricLabelColor(QLabel *label, const QColor &color) {
        if (!label) {
            return;
        }

        QPalette palette = label->palette();
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            palette.setColor(group, QPalette::WindowText, color);
            palette.setColor(group, QPalette::Text, color);
        }
        label->setPalette(palette);
        label->setStyleSheet(QStringLiteral(
            "QLabel { background: transparent; border: none; color: %1; }")
                                 .arg(color.name(QColor::HexArgb)));
    }

    inline void applySignalKMetricPresentation(QLabel *valueLabel,
                                               QLabel *unitLabel,
                                               QWidget *container,
                                               const QString &title,
                                               const QString &text,
                                               const SignalKMetricState state,
                                               const bool pathConfigured = true,
                                               const bool showWhenMissing = true,
                                               const QString &missingText = QStringLiteral("--"),
                                               const QString &detail = QString()) {
        if (!valueLabel || !container) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QPalette metricPalette = signalKMetricPalette(valueLabel, container);
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, metricPalette, false);
        const auto status = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, metricPalette);

        QColor textColor = chrome.text;
        QFont font = valueLabel->font();
        const bool preserveBold = font.bold();
        font.setItalic(false);
        font.setBold(preserveBold);

        switch (state) {
            case SignalKMetricState::Live:
                break;
            case SignalKMetricState::Stale:
                textColor = status.warningFill;
                font.setBold(true);
                break;
            case SignalKMetricState::Missing:
                textColor = chrome.disabledText;
                font.setItalic(true);
                break;
        }

        const QString tooltip = signalKMetricTooltip(title, state, pathConfigured, detail);
        valueLabel->setFont(font);
        valueLabel->setText(state == SignalKMetricState::Missing ? missingText : text);
        applySignalKMetricLabelColor(valueLabel, textColor);
        valueLabel->setToolTip(tooltip);

        if (unitLabel) {
            applySignalKMetricLabelColor(unitLabel, state == SignalKMetricState::Missing ? chrome.disabledText : textColor);
            unitLabel->setToolTip(tooltip);
        }

        container->setToolTip(tooltip);
        container->setVisible(showWhenMissing || state != SignalKMetricState::Missing || !pathConfigured);
    }

    inline void applySignalKDualMetricPresentation(QLabel *primaryLabel,
                                                   QLabel *secondaryLabel,
                                                   QWidget *container,
                                                   const QString &title,
                                                   const QString &primaryText,
                                                   const QString &secondaryText,
                                                   const SignalKMetricState state,
                                                   const bool pathConfigured = true,
                                                   const bool showWhenMissing = true,
                                                   const QString &missingText = QStringLiteral("--"),
                                                   const QString &detail = QString()) {
        if (!primaryLabel || !secondaryLabel || !container) {
            return;
        }

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const QPalette metricPalette = signalKMetricPalette(primaryLabel, container);
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, metricPalette, false);
        const auto status = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, metricPalette);

        QColor textColor = chrome.text;
        QFont font = primaryLabel->font();
        const bool preserveBold = font.bold();
        font.setItalic(false);
        font.setBold(preserveBold);

        switch (state) {
            case SignalKMetricState::Live:
                break;
            case SignalKMetricState::Stale:
                textColor = status.warningFill;
                font.setBold(true);
                break;
            case SignalKMetricState::Missing:
                textColor = chrome.disabledText;
                font.setItalic(true);
                break;
        }

        const QString tooltip = signalKMetricTooltip(title, state, pathConfigured, detail);
        const QString resolvedPrimary = state == SignalKMetricState::Missing ? missingText : primaryText;
        const QString resolvedSecondary = state == SignalKMetricState::Missing ? missingText : secondaryText;

        for (auto *label : {primaryLabel, secondaryLabel}) {
            label->setFont(font);
            applySignalKMetricLabelColor(label, textColor);
            label->setToolTip(tooltip);
        }

        primaryLabel->setText(resolvedPrimary);
        secondaryLabel->setText(resolvedSecondary);
        container->setToolTip(tooltip);
        container->setVisible(showWhenMissing || state != SignalKMetricState::Missing || !pathConfigured);
    }
}

#endif // FAIRWINDSK_SIGNALKMETRICSTATE_HPP
