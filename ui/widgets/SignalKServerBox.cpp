//
// Created by Codex on 02/04/26.
//

#include "SignalKServerBox.hpp"

#include <QLabel>
#include <QMovie>
#include <QPlainTextEdit>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui_SignalKServerBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kIndicatorSize = 12;
        constexpr int kThrobberSize = 18;
    }

    SignalKServerBox::SignalKServerBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::SignalKServerBox) {
        ui->setupUi(this);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        ui->labelIndicator->setFixedSize(kIndicatorSize, kIndicatorSize);
        ui->labelRestIndicator->setFixedSize(kIndicatorSize, kIndicatorSize);
        ui->labelStreamIndicator->setFixedSize(kIndicatorSize, kIndicatorSize);
        ui->labelBusy->setFixedSize(kThrobberSize, kThrobberSize);
        ui->labelBusy->setScaledContents(true);
        ui->labelStatus->setText(tr("Signal K"));
        ui->plainTextEditMessage->setPlainText(tr("Waiting for server"));
        ui->plainTextEditMessage->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        ui->plainTextEditMessage->document()->setDocumentMargin(0);
        ui->plainTextEditMessage->setCenterOnScroll(false);

        m_throbber = new QMovie(QStringLiteral(":/resources/images/widgets/throbber_ajax_loader_metal_512.gif"), QByteArray(), this);
        m_throbber->setScaledSize(QSize(kThrobberSize, kThrobberSize));
        ui->labelBusy->setMovie(m_throbber);
        ui->labelBusy->setVisible(false);

        applyIndicatorColor(ui->labelIndicator, QStringLiteral("#f59e0b"));
        applyIndicatorColor(ui->labelRestIndicator, QStringLiteral("#f59e0b"));
        applyIndicatorColor(ui->labelStreamIndicator, QStringLiteral("#f59e0b"));

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

    SignalKServerBox::~SignalKServerBox() {
        delete ui;
        ui = nullptr;
    }

    void SignalKServerBox::onServerHealthChanged(const bool healthy, const QString &statusText) {
        ui->labelStatus->setText(statusText.trimmed().isEmpty() ? tr("Signal K") : statusText.trimmed());
        applyIndicatorColor(ui->labelIndicator, healthy ? QStringLiteral("#22c55e") : QStringLiteral("#ef4444"));
    }

    void SignalKServerBox::onConnectivityChanged(const bool restHealthy, const bool streamHealthy, const QString &statusText) {
        Q_UNUSED(statusText)
        applyIndicatorColor(ui->labelRestIndicator, restHealthy ? QStringLiteral("#22c55e") : QStringLiteral("#ef4444"));
        applyIndicatorColor(ui->labelStreamIndicator, streamHealthy ? QStringLiteral("#22c55e") : QStringLiteral("#ef4444"));
    }

    void SignalKServerBox::onRequestActivityChanged(const bool active) {
        ui->labelBusy->setVisible(active);
        if (!m_throbber) {
            return;
        }

        if (active) {
            m_throbber->start();
        } else {
            m_throbber->stop();
        }
    }

    void SignalKServerBox::onServerMessageChanged(const QString &message) {
        const QString trimmed = message.trimmed();
        ui->plainTextEditMessage->setPlainText(trimmed.isEmpty() ? tr("Waiting for server") : trimmed);
        ui->plainTextEditMessage->setToolTip(ui->plainTextEditMessage->toPlainText());
    }

    void SignalKServerBox::applyIndicatorColor(QLabel *label, const QString &color) {
        if (!label) {
            return;
        }

        label->setStyleSheet(QStringLiteral(
            "QLabel {"
            " background: %1;"
            " border: 1px solid rgba(255,255,255,0.35);"
            " border-radius: 6px;"
            " }").arg(color));
    }
}
