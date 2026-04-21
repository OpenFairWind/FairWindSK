//
// Created by Codex on 02/04/26.
//

#ifndef FAIRWINDSK_SIGNALKSERVERBOX_HPP
#define FAIRWINDSK_SIGNALKSERVERBOX_HPP

#include <QDateTime>
#include <QResizeEvent>
#include <QWidget>

#include "signalk/Client.hpp"

namespace fairwindsk::ui::widgets {
    QT_BEGIN_NAMESPACE
    namespace Ui { class SignalKServerBox; }
    QT_END_NAMESPACE

    class SignalKServerBox : public QWidget {
        Q_OBJECT

    public:
        explicit SignalKServerBox(QWidget *parent = nullptr);
        ~SignalKServerBox() override;

    protected:
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void onServerHealthChanged(bool healthy, const QString &statusText);
        void onConnectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void onConnectionHealthStateChanged(fairwindsk::signalk::Client::ConnectionHealthState state,
                                            const QString &stateText,
                                            const QDateTime &lastStreamUpdate,
                                            const QString &statusText);
        void onServerMessageChanged(const QString &message);

    private:
        void updateStatusLabel(const QString &text);
        void updateFreshnessMessage();

        Ui::SignalKServerBox *ui = nullptr;
        QString m_statusText;
        QString m_stateText;
        QString m_serverMessage;
        QDateTime m_lastStreamUpdate;
    };
}

#endif // FAIRWINDSK_SIGNALKSERVERBOX_HPP
