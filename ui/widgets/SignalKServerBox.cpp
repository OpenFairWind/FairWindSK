//
// Created by Codex on 02/04/26.
//

#include "SignalKServerBox.hpp"

#include <QLabel>
#include <QMovie>

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
        ui->labelBusy->setFixedSize(kThrobberSize, kThrobberSize);
        ui->labelBusy->setScaledContents(true);
        ui->labelStatus->setText(tr("Signal K"));
        ui->labelMessage->setText(tr("Waiting for server"));

        m_throbber = new QMovie(QStringLiteral(":/resources/images/widgets/throbber_ajax_loader_metal_512.gif"), QByteArray(), this);
        m_throbber->setScaledSize(QSize(kThrobberSize, kThrobberSize));
        ui->labelBusy->setMovie(m_throbber);
        ui->labelBusy->setVisible(false);

        applyIndicatorColor(QStringLiteral("#f59e0b"));

        if (auto *client = FairWindSK::getInstance()->getSignalKClient()) {
            connect(client, &fairwindsk::signalk::Client::serverHealthChanged,
                    this, &SignalKServerBox::onServerHealthChanged);
            connect(client, &fairwindsk::signalk::Client::requestActivityChanged,
                    this, &SignalKServerBox::onRequestActivityChanged);
            connect(client, &fairwindsk::signalk::Client::serverMessageChanged,
                    this, &SignalKServerBox::onServerMessageChanged);
        }
    }

    SignalKServerBox::~SignalKServerBox() {
        delete ui;
        ui = nullptr;
    }

    void SignalKServerBox::onServerHealthChanged(const bool healthy, const QString &statusText) {
        ui->labelStatus->setText(statusText.trimmed().isEmpty() ? tr("Signal K") : statusText.trimmed());
        applyIndicatorColor(healthy ? QStringLiteral("#22c55e") : QStringLiteral("#ef4444"));
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
        ui->labelMessage->setText(trimmed.isEmpty() ? tr("Waiting for server") : trimmed);
        ui->labelMessage->setToolTip(ui->labelMessage->text());
    }

    void SignalKServerBox::applyIndicatorColor(const QString &color) {
        ui->labelIndicator->setStyleSheet(QStringLiteral(
            "QLabel {"
            " background: %1;"
            " border: 1px solid rgba(255,255,255,0.35);"
            " border-radius: 6px;"
            " }").arg(color));
    }
}
