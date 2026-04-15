//
// Created by Codex on 02/04/26.
//

#include "SignalKServerBox.hpp"

#include <QLabel>
#include <QFontMetrics>
#include <QMovie>
#include <QPlainTextEdit>
#include <QTextOption>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui/IconUtils.hpp"
#include "ui_SignalKServerBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kIndicatorSize = 10;
        constexpr int kThrobberSize = 18;
        constexpr int kServerBoxHeight = 58;
    }

    SignalKServerBox::SignalKServerBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::SignalKServerBox) {
        ui->setupUi(this);

        setMinimumHeight(kServerBoxHeight);
        setMaximumHeight(kServerBoxHeight);
        setMinimumWidth(280);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->labelIndicator->setFixedSize(kIndicatorSize, kIndicatorSize);
        ui->labelBusy->setFixedSize(kThrobberSize, kThrobberSize);
        ui->labelBusy->setScaledContents(true);
        ui->labelRestCaption->hide();
        ui->labelStreamCaption->hide();
        ui->plainTextEditMessage->setFixedHeight(28);
        ui->plainTextEditMessage->setPlainText(tr("Waiting for server"));
        ui->plainTextEditMessage->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        ui->plainTextEditMessage->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        ui->plainTextEditMessage->document()->setDocumentMargin(0);
        ui->plainTextEditMessage->setCenterOnScroll(false);
        ui->plainTextEditMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateStatusLabel(tr("Signal K"));

        m_throbber = new QMovie(QStringLiteral(":/resources/images/widgets/throbber_ajax_loader_metal_512.gif"), QByteArray(), this);
        m_throbber->setScaledSize(QSize(kThrobberSize, kThrobberSize));
        ui->labelBusy->setMovie(m_throbber);
        m_idleBusyPixmap = QPixmap(kThrobberSize, kThrobberSize);
        m_idleBusyPixmap.fill(Qt::transparent);
        ui->labelBusy->setPixmap(m_idleBusyPixmap);
        ui->labelBusy->setVisible(true);

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto statusColors = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, palette());

        applyIndicatorColor(ui->labelIndicator, statusColors.warningFill);
        applyStatusBadge(ui->labelRestIndicator, tr("REST"), statusColors.warningFill, statusColors.warningText);
        applyStatusBadge(ui->labelStreamIndicator, tr("STR"), statusColors.errorFill, statusColors.errorText);

        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::serverHealthChanged,
                    this, &SignalKServerBox::onServerHealthChanged);
            connect(client, &fairwindsk::signalk::Client::connectivityChanged,
                    this, &SignalKServerBox::onConnectivityChanged);
            connect(client, &fairwindsk::signalk::Client::requestActivityChanged,
                    this, &SignalKServerBox::onRequestActivityChanged);
            connect(client, &fairwindsk::signalk::Client::serverMessageChanged,
                    this, &SignalKServerBox::onServerMessageChanged);

            onConnectivityChanged(client->isRestHealthy(), client->isStreamHealthy(), client->connectionStatusText());
            onServerHealthChanged(client->isRestHealthy() && client->isStreamHealthy(), client->connectionStatusText());
        }
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

    void SignalKServerBox::onServerHealthChanged(const bool healthy, const QString &statusText) {
        updateStatusLabel(statusText.trimmed().isEmpty() ? tr("Signal K") : statusText.trimmed());
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto colors = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, palette());
        applyIndicatorColor(ui->labelIndicator, healthy ? colors.healthyFill : colors.warningFill);
    }

    void SignalKServerBox::onConnectivityChanged(const bool restHealthy, const bool streamHealthy, const QString &statusText) {
        Q_UNUSED(statusText)
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto colors = fairwindsk::ui::resolveComfortStatusColors(configuration, preset, palette());
        applyStatusBadge(ui->labelRestIndicator,
                         tr("REST"),
                         restHealthy ? colors.healthyFill : colors.errorFill,
                         restHealthy ? colors.healthyText : colors.errorText);
        applyStatusBadge(ui->labelStreamIndicator,
                         tr("STR"),
                         streamHealthy ? colors.healthyFill : colors.errorFill,
                         streamHealthy ? colors.healthyText : colors.errorText);

        if (restHealthy && streamHealthy) {
            applyIndicatorColor(ui->labelIndicator, colors.healthyFill);
        } else if (restHealthy || streamHealthy) {
            applyIndicatorColor(ui->labelIndicator, colors.warningFill);
        } else {
            applyIndicatorColor(ui->labelIndicator, colors.errorFill);
        }
    }

    void SignalKServerBox::onRequestActivityChanged(const bool active) {
        setBusyVisible(active);
    }

    void SignalKServerBox::onServerMessageChanged(const QString &message) {
        const QString trimmed = message.trimmed();
        ui->plainTextEditMessage->setPlainText(trimmed.isEmpty() ? tr("Waiting for server") : trimmed);
        ui->plainTextEditMessage->setToolTip(ui->plainTextEditMessage->toPlainText());
    }

    void SignalKServerBox::applyIndicatorColor(QLabel *label, const QColor &color) {
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

    void SignalKServerBox::applyStatusBadge(QLabel *label, const QString &text, const QColor &fillColor, const QColor &textColor) {
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

    void SignalKServerBox::updateStatusLabel(const QString &text) {
        if (!ui || !ui->labelStatus) {
            return;
        }

        const QString normalized = text.trimmed().isEmpty() ? tr("Signal K") : text.trimmed();
        const QString elided = ui->labelStatus->fontMetrics().elidedText(normalized, Qt::ElideRight, ui->labelStatus->width());
        ui->labelStatus->setText(elided);
        ui->labelStatus->setToolTip(normalized);
    }

    void SignalKServerBox::setBusyVisible(const bool active) {
        if (!ui || !ui->labelBusy) {
            return;
        }

        ui->labelBusy->setVisible(true);
        if (!m_throbber) {
            ui->labelBusy->setPixmap(m_idleBusyPixmap);
            return;
        }

        if (active) {
            ui->labelBusy->setMovie(m_throbber);
            if (m_throbber->state() != QMovie::Running) {
                m_throbber->start();
            }
        } else {
            if (m_throbber->state() == QMovie::Running) {
                m_throbber->stop();
            }
            ui->labelBusy->setMovie(nullptr);
            ui->labelBusy->setPixmap(m_idleBusyPixmap);
        }
    }
}
