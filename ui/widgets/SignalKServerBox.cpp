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
        Q_UNUSED(healthy)
        updateStatusLabel(statusText.trimmed().isEmpty() ? tr("Signal K") : statusText.trimmed());
    }

    void SignalKServerBox::onConnectivityChanged(const bool restHealthy, const bool streamHealthy, const QString &statusText) {
        Q_UNUSED(restHealthy)
        Q_UNUSED(streamHealthy)
        updateStatusLabel(statusText.trimmed().isEmpty() ? tr("Signal K") : statusText.trimmed());
    }

    void SignalKServerBox::onServerMessageChanged(const QString &message) {
        const QString trimmed = message.trimmed();
        ui->plainTextEditMessage->setPlainText(trimmed.isEmpty() ? tr("Waiting for server") : trimmed);
        ui->plainTextEditMessage->setToolTip(ui->plainTextEditMessage->toPlainText());
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
}
