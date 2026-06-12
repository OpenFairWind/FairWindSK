//
// Created by Raffaele Montella on 03/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_POBBar.h" resolved

#include "POBBar.hpp"

#include <algorithm>
#include <cmath>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalBlocker>
#include <QTimer>
#include <QDateTime>
#include <QToolButton>
#include <QTimeZone>
#include <QUuid>

#include "FairWindSK.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui_POBBar.h"

namespace {
    constexpr double kSarHalfSideMeters = 250.0;
    constexpr double kSarSpiralSpacingMeters = 50.0;
    constexpr int kSarSpiralLegs = 10;
    constexpr double kSarParallelSpacingMeters = 60.0;

    QString standardPobUuid() {
        return QStringLiteral("__signalk_standard_mob__");
    }

    QString managedPobType() {
        return QStringLiteral("alarm-mob");
    }

    QString managedPobDescriptionPrefix() {
        return QStringLiteral("FairWindSK POB");
    }

    QString resourceIdFromResponse(const QJsonObject &response) {
        if (response.contains("id") && response["id"].isString()) {
            return response["id"].toString();
        }
        if (response.contains("identifier") && response["identifier"].isString()) {
            return response["identifier"].toString();
        }
        if (response.contains("feature") && response["feature"].isObject()) {
            const auto feature = response["feature"].toObject();
            if (feature.contains("id") && feature["id"].isString()) {
                return feature["id"].toString();
            }
        }
        return {};
    }

    QJsonObject positionObjectFromCoordinate(const QGeoCoordinate &coordinate) {
        QJsonObject position;
        if (coordinate.isValid()) {
            position["latitude"] = coordinate.latitude();
            position["longitude"] = coordinate.longitude();
            if (!std::isnan(coordinate.altitude())) {
                position["altitude"] = coordinate.altitude();
            }
        }
        return position;
    }

    QDateTime parseIsoDateTime(const QString &value) {
        auto dateTime = QDateTime::fromString(value, Qt::ISODateWithMs);
        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(value, Qt::ISODate);
        }
        return dateTime;
    }

    QJsonArray coordinateArray(const QGeoCoordinate &coordinate) {
        QJsonArray values;
        values.append(coordinate.longitude());
        values.append(coordinate.latitude());
        if (!std::isnan(coordinate.altitude())) {
            values.append(coordinate.altitude());
        }
        return values;
    }

    QGeoCoordinate offsetCoordinate(const QGeoCoordinate &origin, const double eastMeters, const double northMeters) {
        QGeoCoordinate result = origin;
        if (!origin.isValid()) {
            return result;
        }
        if (std::abs(northMeters) > 0.001) {
            result = result.atDistanceAndAzimuth(std::abs(northMeters), northMeters >= 0.0 ? 0.0 : 180.0);
        }
        if (std::abs(eastMeters) > 0.001) {
            result = result.atDistanceAndAzimuth(std::abs(eastMeters), eastMeters >= 0.0 ? 90.0 : 270.0);
        }
        return result;
    }

    QTimeZone utcTimeZone() {
        return QTimeZone(QByteArrayLiteral("UTC"));
    }
}

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
        connect(ui->toolButton_SpiralSearch, &QToolButton::clicked, this, &POBBar::onCreateSpiralSearchClicked);
        connect(ui->toolButton_ParallelSearch, &QToolButton::clicked, this, &POBBar::onCreateParallelSearchClicked);

        // When the current POB has changed
        connect(ui->comboBox_currentPOB,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &POBBar::onCurrentIndexChanged);
        ui->comboBox_currentPOB->setAccentButton(true);

        // Not visible by default
        QWidget::setVisible(false);

        // Get the configuration json object
        const auto configurationJsonObject = configuration->getRoot();

        // Check if the configuration object contains the key 'signalk' with an object value
        if (configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];
            updateUnitLabels();

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

        applyComfortStyle();
    }

    void POBBar::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            applyComfortStyle();
        }
    }

    void POBBar::applyComfortStyle() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), true);
        for (auto *button : findChildren<QToolButton *>()) {
            const bool isTransparentIconButton =
                button == ui->toolButton_POB || button == ui->toolButton_Hide;
            fairwindsk::ui::applyBottomBarToolButtonChrome(
                button,
                colors,
                isTransparentIconButton
                    ? fairwindsk::ui::BottomBarButtonChrome::Transparent
                    : fairwindsk::ui::BottomBarButtonChrome::Flat,
                QSize(40, 40),
                88);
        }

        fairwindsk::ui::applyBottomBarPushButtonChrome(ui->pushButton_POB_Cancel, colors, true, 54);
        if (ui->comboBox_currentPOB) {
            ui->comboBox_currentPOB->setAccentButton(true);
        }

        ui->toolButton_SpiralSearch->setText(tr("Spiral"));
        ui->toolButton_ParallelSearch->setText(tr("Parallel"));
    }

    void POBBar::updateUnitLabels() const {
        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        ui->label_unitBearing->setText(m_units->getSignalKUnitLabel(configuredPath("pob.bearing"), "deg"));
        ui->label_unitDistance->setText(m_units->getSignalKUnitLabel(configuredPath("pob.distance"), configuration->getRangeUnits()));
    }

    void POBBar::refreshFromConfiguration() {
        applyComfortStyle();
        updateUnitLabels();
        refreshCurrentPobUi();
        updateBearing(m_lastBearingUpdate);
        updateDistance(m_lastDistanceUpdate);
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

        const QString uuid = createManagedPob();
        if (uuid.isEmpty()) {
            if (m_pobUUIDs.isEmpty()) {
                clearDisplayedPob();
            }
            setVisible(!m_pobUUIDs.isEmpty());
            return;
        }

        syncManagedNotificationState();
        navigateToSelectedPob();
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
            setMetricSubscriptionsActive(false);
            clearDisplayedPob();
            setVisible(true);
            return;
        }

        const auto apiKey = pobNotificationApiKey();
        if (apiKey.isEmpty()) {
            return;
        }

        const QString uuid = currentPOB.toString();
        if (uuid == standardPobUuid()) {
            signalKClient->signalkDelete(QUrl(signalKClient->server().toString() + "/signalk/v2/api/notifications/" + apiKey));
            removePobEntry(uuid);
        } else if (isManagedPob(uuid)) {
            signalKClient->deleteResource(QStringLiteral("waypoints"), uuid);
            removePobEntry(uuid);
            syncManagedNotificationState();
        } else {
            const auto url = signalKClient->server().toString() + "/signalk/v2/api/notifications/" + apiKey + "/" + uuid;
            signalKClient->signalkDelete(QUrl(url));
            removePobEntry(uuid);
        }

        if (m_pobUUIDs.isEmpty()) {
            closePanelAfterNoActivePobs();
        } else {
            navigateToSelectedPob();
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
        navigateToSelectedPob();
        refreshCurrentPobUi();
    }

    void POBBar::onCreateSpiralSearchClicked() {
        createSarSearch(SearchPattern::Spiral);
    }

    void POBBar::onCreateParallelSearchClicked() {
        createSarSearch(SearchPattern::Parallel);
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
        ui->label_Elapsed->setText(QDateTime::fromSecsSinceEpoch(elapsedSecs, utcTimeZone()).toString("hh:mm:ss"));
    }

    /*
 * updatePOB
 * Method called in accordance to signalk to update the navigation position
 */
    void POBBar::updatePOB(const QJsonObject &update) {
        if (update.isEmpty() || !update.contains("updates") || !update["updates"].isArray()) {
            return;
        }

        const auto path = pobNotificationPath();
        if (path.isEmpty()) {
            return;
        }

        for (const auto &updateItem : update["updates"].toArray()) {
            if (!updateItem.isObject()) {
                continue;
            }
            const auto updateObject = updateItem.toObject();
            if (!updateObject.contains("values") || !updateObject["values"].isArray()) {
                continue;
            }

            for (const auto &valueItem : updateObject["values"].toArray()) {
                if (!valueItem.isObject()) {
                    continue;
                }

                const auto updateValue = valueItem.toObject();
                if (!updateValue.contains("path") || !updateValue["path"].isString()) {
                    continue;
                }

                const QString valuePath = updateValue["path"].toString();
                if (!valuePath.startsWith(path)) {
                    continue;
                }
                if (!updateValue.contains("value") || !updateValue["value"].isObject()) {
                    continue;
                }

                auto value = updateValue["value"].toObject();
                const bool exactPath = valuePath == path;
                if (exactPath) {
                    applyStandardNotificationUpdate(value);
                    continue;
                }

                const QString pobUUID = valuePath.mid(path.size() + 1);
                if (pobUUID.isEmpty() || !value.contains("state") || !value["state"].isString()) {
                    continue;
                }

                const QString state = value["state"].toString();
                if (state == "normal") {
                    removePobEntry(pobUUID);
                    if (m_pobUUIDs.isEmpty()) {
                        closePanelAfterNoActivePobs();
                    } else {
                        refreshCurrentPobUi();
                    }
                    continue;
                }

                if (state != "emergency" && state != "alarm") {
                    continue;
                }

                upsertPobValue(pobUUID, value, false);
                setMetricSubscriptionsActive(true);
                refreshCurrentPobUi();
                setVisible(true);
            }
        }
    }

    /*
 * updateBearing
 * Method called in accordance to signalk to update the bearing
 */
    void POBBar::updateBearing(const QJsonObject &update) {
        m_lastBearingUpdate = update;

        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }

        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            const auto text = m_units->formatSignalKValue(configuredPath("pob.bearing"), value, "rad", "deg");

            // Set the course over ground label from the UI to the formatted value
            ui->label_Bearing->setText(text);
        }

    }

    /*
 * updateDistance
 * Method called in accordance to signalk to update the distance
 */
    void POBBar::updateDistance(const QJsonObject &update) {
        m_lastDistanceUpdate = update;


        // Check if for any reason the update is empty
        if (update.isEmpty()) {

            // Exit the method
            return;
        }


        // Get the value and check if value is valid
        if (auto value = fairwindsk::signalk::Client::getDoubleFromUpdateByPath(update); !std::isnan(value)) {

            const auto text = m_units->formatSignalKValue(
                configuredPath("pob.distance"),
                value,
                "m",
                FairWindSK::getInstance()->getConfiguration()->getRangeUnits());

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

    QString POBBar::currentPobUuid() const {
        const auto currentData = ui->comboBox_currentPOB->currentData();
        return currentData.isValid() ? currentData.toString() : QString();
    }

    bool POBBar::hasManagedPobs() const {
        for (auto it = m_pobValues.cbegin(); it != m_pobValues.cend(); ++it) {
            if (it.value().value(QStringLiteral("managedByFairWindSK")).toBool()) {
                return true;
            }
        }
        return false;
    }

    bool POBBar::isManagedPob(const QString &uuid) const {
        return m_pobValues.value(uuid).value(QStringLiteral("managedByFairWindSK")).toBool();
    }

    QGeoCoordinate POBBar::currentVesselPosition() const {
        const auto path = configuredPath("pos");
        if (path.isEmpty()) {
            return {};
        }

        const auto jsonObject = FairWindSK::getInstance()->getSignalKClient()->signalkGet("vessels.self." + path + ".value");
        QGeoCoordinate coordinate;
        if (jsonObject.contains("latitude")) {
            coordinate.setLatitude(jsonObject["latitude"].toDouble());
        }
        if (jsonObject.contains("longitude")) {
            coordinate.setLongitude(jsonObject["longitude"].toDouble());
        }
        if (jsonObject.contains("altitude")) {
            coordinate.setAltitude(jsonObject["altitude"].toDouble());
        }
        return coordinate;
    }

    QGeoCoordinate POBBar::currentPobCoordinate() const {
        const QString uuid = currentPobUuid();
        if (!uuid.isEmpty()) {
            const auto value = m_pobValues.value(uuid);
            if (value.contains("position") && value["position"].isObject()) {
                QGeoCoordinate coordinate;
                const auto position = value["position"].toObject();
                if (position.contains("latitude")) {
                    coordinate.setLatitude(position["latitude"].toDouble());
                }
                if (position.contains("longitude")) {
                    coordinate.setLongitude(position["longitude"].toDouble());
                }
                if (position.contains("altitude")) {
                    coordinate.setAltitude(position["altitude"].toDouble());
                }
                if (coordinate.isValid()) {
                    return coordinate;
                }
            }

            if (uuid != standardPobUuid()) {
                auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref(QStringLiteral("/resources/waypoints/") + uuid);
                if (!waypoint.isEmpty()) {
                    const auto coordinate = waypoint.getCoordinates();
                    if (coordinate.isValid()) {
                        return coordinate;
                    }
                }
            }
        }

        return currentVesselPosition();
    }

    QString POBBar::currentPobLabel() const {
        const QString uuid = currentPobUuid();
        if (uuid.isEmpty()) {
            return tr("POB");
        }
        return m_pobLabels.value(uuid, uuid == standardPobUuid() ? tr("Signal K MOB") : uuid);
    }

    QString POBBar::createManagedPob() {
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const auto coordinate = currentVesselPosition();
        if (!coordinate.isValid()) {
            return {};
        }

        const auto timestamp = QDateTime::currentDateTimeUtc();
        const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString name = tr("POB %1").arg(timestamp.toString(QStringLiteral("HH:mm:ss")));
        const QString description = QStringLiteral("%1 %2")
            .arg(managedPobDescriptionPrefix(), timestamp.toString(Qt::ISODate));
        fairwindsk::signalk::Waypoint waypoint(id, name, description, managedPobType(), coordinate);

        const auto response = client->createResource(QStringLiteral("waypoints"), waypoint);
        const QString persistedId = resourceIdFromResponse(response).isEmpty() ? id : resourceIdFromResponse(response);

        QJsonObject value;
        value["state"] = QStringLiteral("emergency");
        value["message"] = QStringLiteral("POB");
        value["createdAt"] = timestamp.toString(Qt::ISODateWithMs);
        value["position"] = positionObjectFromCoordinate(coordinate);
        upsertPobValue(persistedId, value, true);
        return persistedId;
    }

    bool POBBar::createSarSearch(const SearchPattern pattern) {
        const auto center = currentPobCoordinate();
        if (!center.isValid()) {
            fairwindsk::ui::drawer::warning(this, tr("POB"), tr("Unable to determine the selected POB position."));
            return false;
        }

        const QString pobUuid = currentPobUuid();
        const QString pobLabel = currentPobLabel();
        const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("HH:mm:ss"));
        const QString patternLabel = pattern == SearchPattern::Spiral ? tr("Spiral") : tr("Parallel");
        const QString regionName = tr("%1 SAR Area %2").arg(pobLabel, timestamp);
        const QString routeName = tr("%1 %2 Runlines %3").arg(pobLabel, patternLabel, timestamp);
        const QString baseDescription = tr("Generated from %1 at %2").arg(pobLabel, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const QJsonObject region = buildSarRegionResource(regionName, baseDescription, pobUuid, center, kSarHalfSideMeters);
        const QJsonArray routeCoordinates = pattern == SearchPattern::Spiral
            ? buildSpiralRouteCoordinates(center, kSarSpiralSpacingMeters, kSarSpiralLegs)
            : buildParallelRouteCoordinates(center, kSarHalfSideMeters, kSarParallelSpacingMeters);
        const QJsonObject route = buildSarRouteResource(
            routeName,
            tr("%1 using %2 runlines.").arg(baseDescription, patternLabel.toLower()),
            pattern == SearchPattern::Spiral ? QStringLiteral("sar-spiral") : QStringLiteral("sar-parallel"),
            pobUuid,
            routeCoordinates);

        const auto regionResponse = client->createResource(QStringLiteral("regions"), region);
        const auto routeResponse = client->createResource(QStringLiteral("routes"), route);
        if (regionResponse.isEmpty() || routeResponse.isEmpty()) {
            fairwindsk::ui::drawer::warning(this, tr("POB"), tr("Unable to create the SAR region and route on the Signal K server."));
            return false;
        }

        fairwindsk::ui::drawer::information(
            this,
            tr("POB"),
            tr("Created the SAR region \"%1\" and route \"%2\".").arg(regionName, routeName));
        return true;
    }

    QJsonObject POBBar::buildSarRegionResource(const QString &name,
                                               const QString &description,
                                               const QString &pobUuid,
                                               const QGeoCoordinate &center,
                                               const double halfSideMeters) const {
        QJsonObject properties;
        properties["name"] = name;
        properties["description"] = description;
        properties["source"] = QStringLiteral("fairwindsk-pob");
        properties["searchPattern"] = QStringLiteral("area");
        if (!pobUuid.isEmpty()) {
            properties["pobUuid"] = pobUuid;
        }

        QJsonObject geometry;
        geometry["type"] = QStringLiteral("Polygon");
        geometry["coordinates"] = QJsonArray{buildSquareRegionCoordinates(center, halfSideMeters)};

        QJsonObject feature;
        feature["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        feature["type"] = QStringLiteral("Feature");
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        QJsonObject resource;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        resource["name"] = name;
        resource["description"] = description;
        resource["feature"] = feature;
        return resource;
    }

    QJsonObject POBBar::buildSarRouteResource(const QString &name,
                                              const QString &description,
                                              const QString &type,
                                              const QString &pobUuid,
                                              const QJsonArray &coordinates) const {
        QJsonObject properties;
        properties["name"] = name;
        properties["description"] = description;
        properties["source"] = QStringLiteral("fairwindsk-pob");
        if (!pobUuid.isEmpty()) {
            properties["pobUuid"] = pobUuid;
        }

        QJsonObject geometry;
        geometry["type"] = QStringLiteral("LineString");
        geometry["coordinates"] = coordinates;

        QJsonObject feature;
        feature["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        feature["type"] = QStringLiteral("Feature");
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        QJsonObject resource;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        resource["name"] = name;
        resource["description"] = description;
        resource["type"] = type;
        resource["feature"] = feature;
        return resource;
    }

    QJsonArray POBBar::buildSquareRegionCoordinates(const QGeoCoordinate &center, const double halfSideMeters) const {
        const double diagonalMeters = std::sqrt(2.0) * halfSideMeters;
        QJsonArray ring;
        ring.append(coordinateArray(center.atDistanceAndAzimuth(diagonalMeters, 315.0)));
        ring.append(coordinateArray(center.atDistanceAndAzimuth(diagonalMeters, 45.0)));
        ring.append(coordinateArray(center.atDistanceAndAzimuth(diagonalMeters, 135.0)));
        ring.append(coordinateArray(center.atDistanceAndAzimuth(diagonalMeters, 225.0)));
        ring.append(coordinateArray(center.atDistanceAndAzimuth(diagonalMeters, 315.0)));
        return ring;
    }

    QJsonArray POBBar::buildSpiralRouteCoordinates(const QGeoCoordinate &center, const double spacingMeters, const int legs) const {
        QJsonArray coordinates;
        coordinates.append(coordinateArray(center));

        QGeoCoordinate current = center;
        const QVector<double> bearings = {90.0, 180.0, 270.0, 0.0};
        for (int leg = 0; leg < legs; ++leg) {
            const double distance = spacingMeters * (1 + (leg / 2));
            current = current.atDistanceAndAzimuth(distance, bearings.at(leg % bearings.size()));
            coordinates.append(coordinateArray(current));
        }

        return coordinates;
    }

    QJsonArray POBBar::buildParallelRouteCoordinates(const QGeoCoordinate &center,
                                                     const double halfSideMeters,
                                                     const double spacingMeters) const {
        QJsonArray coordinates;
        const int lineCount = std::max(2, static_cast<int>(std::floor((halfSideMeters * 2.0) / spacingMeters)) + 1);
        for (int index = 0; index < lineCount; ++index) {
            const double northMeters = halfSideMeters - (index * spacingMeters);
            const double clampedNorthMeters = std::max(-halfSideMeters, northMeters);
            const QGeoCoordinate west = offsetCoordinate(center, -halfSideMeters, clampedNorthMeters);
            const QGeoCoordinate east = offsetCoordinate(center, halfSideMeters, clampedNorthMeters);
            if (index % 2 == 0) {
                coordinates.append(coordinateArray(west));
                coordinates.append(coordinateArray(east));
            } else {
                coordinates.append(coordinateArray(east));
                coordinates.append(coordinateArray(west));
            }
        }

        return coordinates;
    }

    void POBBar::syncManagedNotificationState() const {
        const auto apiKey = pobNotificationApiKey();
        if (apiKey.isEmpty()) {
            return;
        }

        const auto client = FairWindSK::getInstance()->getSignalKClient();
        const auto notificationUrl = QUrl(client->server().toString() + "/signalk/v2/api/notifications/" + apiKey);
        if (hasManagedPobs()) {
            QString payload = QStringLiteral(R"({"message":"POB"})");
            client->signalkPut(notificationUrl, payload);
            return;
        }

        client->signalkDelete(notificationUrl);
    }

    void POBBar::navigateToSelectedPob() const {
        const QString uuid = currentPobUuid();
        if (uuid.isEmpty() || uuid == standardPobUuid()) {
            return;
        }

        FairWindSK::getInstance()->getSignalKClient()->navigateToWaypoint(QStringLiteral("/resources/waypoints/") + uuid);
    }

    void POBBar::applyStandardNotificationUpdate(const QJsonObject &value) {
        if (!value.contains("state") || !value["state"].isString()) {
            return;
        }

        const QString state = value["state"].toString();
        if (state == QStringLiteral("normal")) {
            removePobEntry(standardPobUuid());
            if (m_pobUUIDs.isEmpty()) {
                closePanelAfterNoActivePobs();
            } else {
                refreshCurrentPobUi();
            }
            return;
        }

        if ((state == QStringLiteral("emergency") || state == QStringLiteral("alarm")) &&
            !hasManagedPobs()) {
            upsertPobValue(standardPobUuid(), value, false);
            setMetricSubscriptionsActive(true);
            refreshCurrentPobUi();
            setVisible(true);
        }
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

        const auto waypointResources = client->getResources(QStringLiteral("waypoints"));
        QString preferredUuid;
        {
            const QSignalBlocker blocker(ui->comboBox_currentPOB);
            for (auto it = waypointResources.cbegin(); it != waypointResources.cend(); ++it) {
                if (!it.value().contains("type") || !it.value()["type"].isString() ||
                    it.value()["type"].toString() != managedPobType()) {
                    continue;
                }

                QJsonObject value;
                value["state"] = QStringLiteral("emergency");
                value["message"] = QStringLiteral("POB");
                value["managedByFairWindSK"] = true;
                if (it.value().contains("timestamp") && it.value()["timestamp"].isString()) {
                    value["createdAt"] = it.value()["timestamp"].toString();
                }
                if (it.value().contains("position") && it.value()["position"].isObject()) {
                    value["position"] = it.value()["position"].toObject();
                }

                m_pobUUIDs.insert(it.key());
                m_pobValues.insert(it.key(), value);
                addOrSelectPOB(it.key());
                if (it.key() == nextPointId) {
                    preferredUuid = it.key();
                }
            }
        }

        const auto pobs = client->signalkGet("vessels.self." + path);
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

                upsertPobValue(key, value, false);
                if (key == nextPointId) {
                    preferredUuid = key;
                }
            }
        }

        const auto standardNotification = client->signalkGet("vessels.self." + path + ".value");
        if (!hasManagedPobs() &&
            standardNotification.contains("state") &&
            standardNotification["state"].isString()) {
            const QString state = standardNotification["state"].toString();
            if (state == QStringLiteral("emergency") || state == QStringLiteral("alarm")) {
                upsertPobValue(standardPobUuid(), standardNotification, false);
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

    void POBBar::closePanelAfterNoActivePobs() {
        setMetricSubscriptionsActive(false);
        clearDisplayedPob();
        setVisible(false);
        emit hidden();
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
        ui->label_Latitude->setText(tr("--"));
        ui->label_Longitude->setText(tr("--"));
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

        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        ui->label_Latitude->setText(
            fairwindsk::ui::geo::formatSingleCoordinate(
                geoCoordinate.latitude(),
                true,
                configuration->getCoordinateFormat()));
        ui->label_Longitude->setText(
            fairwindsk::ui::geo::formatSingleCoordinate(
                geoCoordinate.longitude(),
                false,
                configuration->getCoordinateFormat()));
    }

    void POBBar::updateStartTime() {
        m_currentStartTimeUtc = {};

        const QString uuid = currentPobUuid();
        if (uuid.isEmpty()) {
            return;
        }

        const auto value = m_pobValues.value(uuid);
        const QStringList localTimeKeys = {
            QStringLiteral("createdAt"),
            QStringLiteral("timestamp"),
            QStringLiteral("startTime")
        };
        for (const auto &key : localTimeKeys) {
            if (value.contains(key) && value[key].isString()) {
                auto startDateTimeUtc = parseIsoDateTime(value[key].toString());
                if (startDateTimeUtc.isValid()) {
                    startDateTimeUtc.setTimeZone(utcTimeZone());
                    m_currentStartTimeUtc = startDateTimeUtc;
                    return;
                }
            }
        }

        const auto startTimePath = configuredPath("pob.startTime");
        if (startTimePath.isEmpty()) {
            return;
        }

        const auto signalKClient = FairWindSK::getInstance()->getSignalKClient();
        const auto url = signalKClient->url().toString() + "/v1/api/vessels/self/" + QString(startTimePath).replace(".", "/");
        const auto jsonObjectStartTime = signalKClient->signalkGet(QUrl(url));
        const auto startTimeIso8601 = jsonObjectStartTime["value"].toString();
        if (startTimeIso8601.isEmpty()) {
            return;
        }

        auto startDateTimeUtc = parseIsoDateTime(startTimeIso8601);
        if (startDateTimeUtc.isValid()) {
            startDateTimeUtc.setTimeZone(utcTimeZone());
            m_currentStartTimeUtc = startDateTimeUtc;
        }
    }

    void POBBar::upsertPobValue(const QString &uuid, const QJsonObject &value, const bool managed) {
        if (uuid.isEmpty()) {
            return;
        }

        auto storedValue = value;
        storedValue["managedByFairWindSK"] = managed;
        m_pobUUIDs.insert(uuid);
        m_pobValues.insert(uuid, storedValue);
        addOrSelectPOB(uuid);
    }

    void POBBar::removePobEntry(const QString &uuid) {
        m_pobUUIDs.remove(uuid);
        m_pobValues.remove(uuid);
        m_pobLabels.remove(uuid);
        removePOB(uuid);
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
            QString label;
            if (uuid == standardPobUuid()) {
                label = tr("Signal K MOB");
            } else {
                auto waypoint = FairWindSK::getInstance()->getSignalKClient()->getWaypointByHref("/resources/waypoints/" + uuid);
                label = waypoint.isEmpty() ? uuid : waypoint.getName();
            }
            m_pobLabels.insert(uuid, label);
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
