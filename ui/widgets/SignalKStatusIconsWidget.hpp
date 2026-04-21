//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_SIGNALKSTATUSICONSWIDGET_HPP
#define FAIRWINDSK_SIGNALKSTATUSICONSWIDGET_HPP

#include <QDateTime>
#include <QLabel>
#include <QPixmap>
#include <QWidget>

#include "FairWindSK.hpp"
#include "signalk/Client.hpp"

class QMovie;

namespace fairwindsk::ui::widgets {
    class SignalKStatusIconsWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit SignalKStatusIconsWidget(QWidget *parent = nullptr);
        ~SignalKStatusIconsWidget() override;

        void refreshFromConfiguration();

    protected:
        void changeEvent(QEvent *event) override;

    private slots:
        void onServerHealthChanged(bool healthy, const QString &statusText);
        void onConnectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void onConnectionHealthStateChanged(fairwindsk::signalk::Client::ConnectionHealthState state,
                                            const QString &stateText,
                                            const QDateTime &lastStreamUpdate,
                                            const QString &statusText);
        void onAppsStateChanged(fairwindsk::FairWindSK::AppsState state, const QString &stateText);
        void onRequestActivityChanged(bool active);
        void onRuntimeHealthChanged(fairwindsk::FairWindSK::RuntimeHealthState state,
                                    const QString &summary,
                                    const QString &badgeText);

    private:
        void applyIndicatorColor(QLabel *label, const QColor &color) const;
        void applyStatusBadge(QLabel *label, const QString &text, const QColor &fillColor, const QColor &textColor) const;
        void setBusyVisible(bool active);
        void refreshIndicators(bool serverHealthy,
                               bool restHealthy,
                               bool streamHealthy,
                               bool requestActive,
                               const QString &statusText);

        QLabel *m_serverIndicator = nullptr;
        QLabel *m_busyIndicator = nullptr;
        QLabel *m_restIndicator = nullptr;
        QLabel *m_streamIndicator = nullptr;
        QMovie *m_throbber = nullptr;
        QPixmap m_idleBusyPixmap;
        bool m_serverHealthy = false;
        bool m_restHealthy = false;
        bool m_streamHealthy = false;
        bool m_requestActive = false;
        QString m_stateText;
        QString m_runtimeSummary;
        QString m_runtimeBadgeText;
        QString m_appsStateText;
        QDateTime m_lastStreamUpdate;
    };
}

#endif // FAIRWINDSK_SIGNALKSTATUSICONSWIDGET_HPP
