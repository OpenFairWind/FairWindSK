//
// Created by Raffaele Montella on 03/06/24.
//

#ifndef FAIRWINDSK_POBBAR_HPP
#define FAIRWINDSK_POBBAR_HPP

#include <nlohmann/json.hpp>
#include <QJsonObject>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QEvent>
#include <QWidget>

#include "Units.hpp"

namespace Ui { class POBBar; }

namespace fairwindsk::ui::bottombar {

    class POBBar : public QWidget {
    Q_OBJECT

    public:
        explicit POBBar(QWidget *parent = nullptr);

        ~POBBar() override;

        void POB();
        void refreshFromConfiguration();

    public
        slots:

        void updatePOB(const QJsonObject& update);
        void updateBearing(const QJsonObject& update);
        void updateDistance(const QJsonObject& update);

        void onCancelClicked();
        void onHideClicked();
        void onCurrentIndexChanged(int index);
        void updateElapsed();


    signals:
        void cancelPOB();
        void hidden();

    private:
        void changeEvent(QEvent *event) override;
        void applyComfortStyle() const;
        QString configuredPath(const char *key) const;
        QString pobNotificationPath() const;
        QString pobNotificationApiKey() const;
        void loadExistingPobs();
        void setMetricSubscriptionsActive(bool active);
        void refreshCurrentPobUi();
        void refreshCancelButton();
        void clearDisplayedPob();
        void updateUnitLabels() const;
        void updateDisplayedPosition(const QJsonObject& position);
        void updateStartTime();
        void addOrSelectPOB(const QString& uuid);
        void removePOB(const QString& uuid);

        Ui::POBBar *ui;
        Units *m_units;
        nlohmann::json m_signalkPaths;
        QTimer *m_timer = nullptr;
        QSet<QString> m_pobUUIDs;
        QHash<QString, QJsonObject> m_pobValues;
        QHash<QString, QString> m_pobLabels;
        QDateTime m_currentStartTimeUtc;
        bool m_metricSubscriptionsActive = false;
        QJsonObject m_lastBearingUpdate;
        QJsonObject m_lastDistanceUpdate;
    };
} // fairwindsk::ui::bottombar

#endif //FAIRWINDSK_POBBAR_HPP
