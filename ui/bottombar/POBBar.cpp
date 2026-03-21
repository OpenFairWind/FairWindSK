//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_POBBar.h" resolved

#include "POBBar.hpp"

#include <algorithm>
#include <cmath>
#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QSignalBlocker>
#include <QTimer>
#include <QDateTime>
#include <QTimeZone>

#include "FairWindSK.hpp"
#include "ui_POBBar.h"

namespace fairwindsk::ui::bottombar {
    POBBar::POBBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::POBBar) {

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Check if the debug is on
        if (fairWindSK->isDebug()) {

            // Write a message
            qDebug() << "POBBar::POBBar";
        }

        // Get units converter instance
        m_units = Units::getInstance();

        // Initialize the user interface
        ui->setupUi(this);

        // Create a new timer which will contain the current time
        m_timer = new QTimer(this);

        // When the timer stops, update the time
        connect(m_timer, &QTimer::timeout, this, &POBBar::updateElapsed);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->pushButton_POB_Cancel, &QPushButton::clicked, this, &POBBar::onCancelClicked);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &POBBar::onHideClicked);

        // When the current POB has changed
        connect(ui->comboBox_currentPOB, qOverload<int>(&QComboBox::currentIndexChanged), this, &POBBar::onCurrentIndexChanged);

        // Not visible by default
        QWidget::setVisible(false);

        // Get the configuration json object
        const auto configurationJsonObject = configuration->getRoot();

        ui->label_unitDistance->setText(FairWindSK::getInstance()->getConfiguration()->getRangeUnits());

        // Check if the configuration object contains the key 'signalk' with an object value
        if (configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];

            const auto path = pobNotificationPath();
            if (!path.isEmpty()) {
                if (fairWindSK->isDebug()) {
                    qDebug() << "POBBar::POBBar path:" << path;
                }

                loadExistingPobs();

                updatePOB(FairWindSK::getInstance()->getSignalKClient()->subscribe(
                    path,
                    this,
                    SLOT(fairwindsk::ui::bottombar::POBBar::updatePOB)
                ));
            }
        }
    }

    /*
     * POB()
     * Cancel or set POB emergency
     */
    void POBBar::POB() {
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto signalKClient = fairWindSK->getSignalKClient();
        const auto path = pobNotificationPath();
        const auto apiKey = pobNotificationApiKey();
        if (path.isEmpty() || apiKey.isEmpty()) {
            return;
        }

        const auto notificationObject = signalKClient->signalkGet("vessels.self." + path + ".value");
        if (notificationObject.isEmpty() || (
            notificationObject.contains("state") &&
            notificationObject["state"].isString() &&
            notificationObject["state"].toString() == "normal")) {
            if (m_pobUUIDs.isEmpty()) {
                clearDisplayedPob();
            } else {
                refreshCurrentPobUi();
            }
            setVisible(true);

            const auto notificationUrl = QUrl(signalKClient->server().toString() + "/signalk/v2/api/notifications/" + apiKey);
            const auto message = apiKey == "mob" ? QStringLiteral("POB") : apiKey.toUpper();
            auto payload = QString(R"({"message":"%1"})").arg(message);
            signalKClient->signalkPut(notificationUrl, payload);
            return;
        }

        refreshCurrentPobUi();
        setVisible(true);
    }

    /*
     * onCancelClicked
     * Cancel the POB emergency
     */
    void POBBar::onCancelClicked() {
        const auto signalKClient = FairWindSK::getInstance()->getSignalKClient();
        const auto currentPOB = ui->comboBox_currentPOB->currentData();
        if (!currentPOB.isValid() || currentPOB.toString().isEmpty()) {
            return;
        }

        const auto apiKey = pobNotificationApiKey();
        if (apiKey.isEmpty()) {
            return;
        }

        const auto url = signalKClient->server().toString() + "/signalk/v2/api/notifications/" + apiKey + "/" + currentPOB.toString();
        signalKClient->signalkDelete(QUrl(url));

        m_pobUUIDs.remove(currentPOB.toString());
        m_pobValues.remove(currentPOB.toString());
        removePOB(currentPOB.toString());

        if (m_pobUUIDs.isEmpty()) {
            setMetricSubscriptionsActive(false);
            clearDisplayedPob();
            setVisible(false);
        } else {
            refreshCurrentPobUi();
        }

        emit cancelPOB();
    }

    /*
     * onHideClicked
     * Hides the widget window
     */
    void POBBar::onHideClicked() {

        // Hide the widget
        setVisible(false);

        // Emit the hide signal
        emit hidden();
    }

    /*
     * onCurrentIndexChanged
     * The current POB as changed
     */
    void POBBar::onCurrentIndexChanged(int index) {
        Q_UNUSED(index);
        refreshCurrentPobUi();
    }


    /*
 * updateElapsed
 * Method called to update the current datetime
 */
    void POBBar::updateElapsed() {
        if (!m_currentStartTimeUtc.isValid()) {
            ui->label_Elapsed->setText("00:00:00");
            return;
        }

        const auto elapsedSecs = std::max<qint64>(0, m_currentStartTimeUtc.secsTo(QDateTime::currentDateTimeUtc()));
        ui->label_Elapsed->setText(QDateTime::fromSecsSinceEpoch(elapsedSecs, QTimeZone::UTC).toString("hh:mm:ss"));
    }

    /*
 * updatePOB
 * Method called in accordance to signalk to update the navigation position
 */
    void POBBar::updatePOB(const QJsonObject &update) {
        if (update.isEmpty()) {
            return;
        }

        const auto path = pobNotificationPath();
        if (path.isEmpty()) {
            return;
        }

        auto value = fairwindsk::signalk::Client::getObjectFromUpdateByPath(update, path);
        if (value.isEmpty()) {
            return;
        }

        const auto pobUUID = value["subPath"].toString();
        if (pobUUID.isEmpty() || !value.contains("state") || !value["state"].isString()) {
            return;
        }

        const auto state = value["state"].toString();
        if (state == "normal") {
            m_pobUUIDs.remove(pobUUID);
            m_pobValues.remove(pobUUID);
            removePOB(pobUUID);

            if (m_pobUUIDs.isEmpty()) {
                setMetricSubscriptionsActive(false);
                clearDisplayedPob();
                setVisible(false);
            } else {
                refreshCurrentPobUi();
            }
            return;
        }

        if (state != "emergency" && state != "alarm") {
            return;
        }

        m_pobUUIDs.insert(pobUUID);
        m_pobValues.insert(pobUUID, value);
        addOrSelectPOB(pobUUID);
        setMetricSubscriptionsActive(true);
        refreshCurrentPobUi();
        setVisible(true);
    }

    /*
 * updateBearing
 * Method called in accordance to signalk to update the bearing
 */
    void POBBar::updateBearing(const QJsonObject &update) {

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            // Convert rad to deg
            value = m_units->convert("rad","deg", value);

            // Build the formatted value
            const auto text = m_units->format("deg", value);

            // Set the course over ground label from the UI to the formatted value
            ui->label_Bearing->setText(text);
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void POBBar::updateDistance(const QJsonObject &update) {


        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }


        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            // Convert m/s to knots
            value = m_units->convert("m",FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Build the formatted value
            const auto text = m_units->format(FairWindSK::getInstance()->getConfiguration()->getRangeUnits(), value);

            // Set the speed over ground label from the UI to the formatted value
            ui->label_Distance->setText(text);
        }
    }

    /*
     * ~POBBar
     * Destructor
     */
    POBBar::~POBBar() {
        if (m_timer) {
            m_timer->stop();
        }
        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }

    QString POBBar::configuredPath(const char *key) const {
        if (!m_signalkPaths.contains(key) || !m_signalkPaths[key].is_string()) {
            return {};
        }
        return QString::fromStdString(m_signalkPaths[key].get<std::string>());
    }

    QString POBBar::pobNotificationPath() const {
        return configuredPath("notifications.pob");
    }

    QString POBBar::pobNotificationApiKey() const {
        auto apiKey = pobNotificationPath();
        apiKey.replace("notifications.", "");
        return apiKey;
    }

    void POBBar::loadExistingPobs() {
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const auto path = pobNotificationPath();
        if (path.isEmpty()) {
            return;
        }

        QString nextPointId;
        const auto navigationCourseUrl = QUrl(client->server().toString() + "/signalk/v2/api/vessels/self/navigation/course");
        const auto navigationCourse = client->signalkGet(navigationCourseUrl);
        if (navigationCourse.contains("nextPoint") && navigationCourse["nextPoint"].isObject()) {
            const auto nextPoint = navigationCourse["nextPoint"].toObject();
            if (nextPoint.contains("href") && nextPoint["href"].isString()) {
                nextPointId = nextPoint["href"].toString().replace("/resources/waypoints/", "");
            }
        }

        const auto pobs = client->signalkGet("vessels.self." + path);
        QString preferredUuid;
        {
            const QSignalBlocker blocker(ui->comboBox_currentPOB);
            for (const auto &key : pobs.keys()) {
                if (!pobs[key].isObject()) {
                    continue;
                }
                const auto pob = pobs[key].toObject();
                if (!pob.contains("value") || !pob["value"].isObject()) {
                    continue;
                }

                const auto value = pob["value"].toObject();
                if (!value.contains("state") || !value["state"].isString()) {
                    continue;
                }

                const auto state = value["state"].toString();
                if (state != "emergency" && state != "alarm") {
                    continue;
                }

                m_pobUUIDs.insert(key);
                m_pobValues.insert(key, value);
                addOrSelectPOB(key);
                if (key == nextPointId) {
                    preferredUuid = key;
                }
            }
        }

        if (!preferredUuid.isEmpty()) {
            const QSignalBlocker blocker(ui->comboBox_currentPOB);
            const int index = ui->comboBox_currentPOB->findData(preferredUuid);
            if (index >= 0) {
                ui->comboBox_currentPOB->setCurrentIndex(index);
            }
        }

        setMetricSubscriptionsActive(!m_pobUUIDs.isEmpty());
        refreshCurrentPobUi();
        setVisible(!m_pobUUIDs.isEmpty());
    }

    void POBBar::setMetricSubscriptionsActive(const bool active) {
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const auto bearingPath = configuredPath("pob.bearing");
        const auto distancePath = configuredPath("pob.distance");

        if (active == m_metricSubscriptionsActive) {
            if (active) {
                updateStartTime();
                updateElapsed();
            }
            return;
        }

        if (active) {
            if (!bearingPath.isEmpty()) {
                updateBearing(client->subscribe(
                    bearingPath,
                    this,
                    SLOT(fairwindsk::ui::bottombar::POBBar::updateBearing)
                ));
            }
            if (!distancePath.isEmpty()) {
                updateDistance(client->subscribe(
                    distancePath,
                    this,
                    SLOT(fairwindsk::ui::bottombar::POBBar::updateDistance)
                ));
            }
            updateStartTime();
            updateElapsed();
            m_timer->start(1000);
        } else {
            if (!bearingPath.isEmpty()) {
                client->removeSubscription(bearingPath, this);
            }
            if (!distancePath.isEmpty()) {
                client->removeSubscription(distancePath, this);
            }
            m_timer->stop();
            ui->label_Bearing->setText("___");
            ui->label_Distance->setText("___");
            ui->label_Elapsed->setText("00:00:00");
            m_currentStartTimeUtc = {};
        }

        m_metricSubscriptionsActive = active;
    }

    void POBBar::refreshCurrentPobUi() {
        refreshCancelButton();

        if (ui->comboBox_currentPOB->count() == 0) {
            clearDisplayedPob();
            return;
        }

        int index = ui->comboBox_currentPOB->currentIndex();
        if (index < 0) {
            const QSignalBlocker blocker(ui->comboBox_currentPOB);
            ui->comboBox_currentPOB->setCurrentIndex(0);
            index = 0;
        }

        const auto uuid = ui->comboBox_currentPOB->itemData(index).toString();
        if (uuid.isEmpty() || !m_pobValues.contains(uuid)) {
            clearDisplayedPob();
            return;
        }

        const auto value = m_pobValues.value(uuid);
        if (value.contains("position") && value["position"].isObject()) {
            updateDisplayedPosition(value["position"].toObject());
        }

        updateStartTime();
        updateElapsed();
    }

    void POBBar::refreshCancelButton() {
        ui->pushButton_POB_Cancel->setEnabled(ui->comboBox_currentPOB->currentIndex() >= 0);
    }

    void POBBar::clearDisplayedPob() {
        ui->label_Latitude->setText("00°00.000' N");
        ui->label_Longitude->setText("000°00.000' E");
        ui->label_Bearing->setText("___");
        ui->label_Distance->setText("___");
        ui->label_Elapsed->setText("00:00:00");
        m_currentStartTimeUtc = {};
        refreshCancelButton();
    }

    void POBBar::updateDisplayedPosition(const QJsonObject &position) {
        QGeoCoordinate geoCoordinate;
        if (position.contains("latitude")) {
            geoCoordinate.setLatitude(position["latitude"].toDouble());
        }
        if (position.contains("longitude")) {
            geoCoordinate.setLongitude(position["longitude"].toDouble());
        }
        if (position.contains("altitude")) {
            geoCoordinate.setAltitude(position["altitude"].toDouble());
        }

        const auto texts = geoCoordinate.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere).split(",");
        if (texts.size() >= 2) {
            ui->label_Latitude->setText(texts[0].trimmed());
            ui->label_Longitude->setText(texts[1].trimmed());
        }
    }

    void POBBar::updateStartTime() {
        m_currentStartTimeUtc = {};

        const auto startTimePath = configuredPath("pob.startTime");
        if (startTimePath.isEmpty() || ui->comboBox_currentPOB->count() == 0) {
            return;
        }

        const auto signalKClient = FairWindSK::getInstance()->getSignalKClient();
        const auto url = signalKClient->url().toString() + "/v1/api/vessels/self/" + QString(startTimePath).replace(".", "/");
        const auto jsonObjectStartTime = signalKClient->signalkGet(QUrl(url));
        const auto startTimeIso8601 = jsonObjectStartTime["value"].toString();
        if (startTimeIso8601.isEmpty()) {
            return;
        }

        auto startDateTimeUtc = QDateTime::fromString(startTimeIso8601, Qt::ISODateWithMs);
        if (!startDateTimeUtc.isValid()) {
            startDateTimeUtc = QDateTime::fromString(startTimeIso8601, Qt::ISODate);
        }
        if (startDateTimeUtc.isValid()) {
            startDateTimeUtc.setTimeZone(QTimeZone::UTC);
            m_currentStartTimeUtc = startDateTimeUtc;
        }
    }

    void POBBar::addOrSelectPOB(const QString& uuid) {
        if (uuid.isEmpty()) {
            return;
        }

        const int existingIndex = ui->comboBox_currentPOB->findData(uuid);
        if (existingIndex >= 0) {
            ui->comboBox_currentPOB->setCurrentIndex(existingIndex);
            return;
        }

        if (!m_pobLabels.contains(uuid)) {
            auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref("/resources/waypoints/" + uuid);
            m_pobLabels.insert(uuid, waypoint.isEmpty() ? uuid : waypoint.getName());
        }

        const QString label = m_pobLabels.value(uuid, uuid);
        ui->comboBox_currentPOB->addItem(label, uuid);
        ui->comboBox_currentPOB->setCurrentIndex(ui->comboBox_currentPOB->count() - 1);
        refreshCancelButton();
    }

    void POBBar::removePOB(const QString& uuid) {
        const int index = ui->comboBox_currentPOB->findData(uuid);
        if (index >= 0) {
            ui->comboBox_currentPOB->removeItem(index);
        }

        refreshCancelButton();
    }
} // fairwindsk::ui::bottombar
