//
// Created by Raffaele Montella on 18/04/21.
//

#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <iomanip>
#include <sstream>

#include "Document.hpp"
#include "Subscription.hpp"

namespace fairwindsk::signalk {
    Document::Document() {
        insert("version", "1.0.0");
    }

    Document::~Document() {}

    void Document::update(QJsonObject &jsonObjectUpdate) {
        auto context = jsonObjectUpdate["context"].toString();
        auto updates = jsonObjectUpdate["updates"].toArray();
        for (auto updateItem: updates) {
            QJsonObject source = updateItem.toObject()["source"].toObject();
            QString timeStamp = updateItem.toObject()["timestamp"].toString();
            auto values = updateItem.toObject()["values"].toArray();
            for (auto valueItem: values) {
                QString fullPath = context + "." + valueItem.toObject()["path"].toString();
                QJsonValue value = valueItem.toObject()["value"];
                QJsonObject jsonObject;
                jsonObject["value"] = value;
                jsonObject["source"] = source;
                jsonObject["timestamp"] = timeStamp;
                insert(fullPath,jsonObject);
            }
        }
        //qDebug() << m_jsonDocument.toJson();

    }

    void Document::modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue) {
        const int indexOfDot = path.indexOf('.');
        const QString propertyName = path.left(indexOfDot);
        const QString subPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

        QJsonValue subValue = obj[propertyName];

        if (subPath.isEmpty()) {
            subValue = newValue;
        } else {
            QJsonObject obj = subValue.toObject();
            modifyJsonValue(obj, subPath, newValue);
            subValue = obj;
        }

        obj[propertyName] = subValue;
    }

    void Document::object2pathAndValue(QMap<QString, QJsonValue> &map, const QString &path, const QJsonValue value) {
        QJsonValue result = value;
        if (value.isObject()) {
            auto object = value.toObject();
            for (auto key: object.keys()) {
                auto newPath = path;
                if (!newPath.isEmpty()) {
                    newPath = newPath + ".";
                }
                newPath = newPath + key;
                if (object[key].isObject()) {
                    object2pathAndValue(map, newPath, object[key]);
                } else {
                    map[newPath] = object[key];
                }
            }
        }
    }

    QJsonValue Document::set(const QString &fullPath, const QJsonValue &newValue) {
        return insert(fullPath, newValue, 2);
    }

    QJsonValue Document::get(const QString &path) {
        return emit fetched(path);
    }

    QJsonValue Document::insert(const QString &fullPath, const QJsonValue &newValue, int mode) {
        QJsonValue result;
        // Check if the full path is an empty string
        if (fullPath.isEmpty()) {

            // The full path is an empty string the newValue must be an object inserted in the root.

            QMap<QString, QJsonValue> map;
            object2pathAndValue(map, fullPath, newValue);
            for (const auto &key: map.keys()) {
                //qDebug() << key << " : " << map[key];
                insert(key, map[key], mode);
            }

        } else {
            QString processedFullPath = fullPath;
            processedFullPath = processedFullPath.replace("${id}", QUuid::createUuid().toString(QUuid::WithoutBraces));
            processedFullPath = processedFullPath.replace("${self}", getSelf());

            auto previousValue = subtree(processedFullPath);

            if (mode & 1) {
                modifyJsonValue(m_root, processedFullPath, newValue);

                //qDebug() << "SignalKDocument::insert: " << fullPath;
                emit changed(processedFullPath);

                if (processedFullPath.indexOf(getSelf() + ".navigation.position") >= 0) {
                    emit updatedNavigationPosition();
                } else if (processedFullPath.indexOf(getSelf() + "navigation.courseOverGroundTrue") >= 0) {
                    emit updatedNavigationCourseOverGroundTrue();
                } else if (processedFullPath.indexOf(getSelf() + "navigation.speedOverGround") >= 0) {
                    emit updatedNavigationSpeedOverGround();
                }

                for (auto subscription: subscriptions) {
                    subscription.match(this, processedFullPath);
                }
            }
            if (mode & 2) {
                if (previousValue.isNull()) {
                    result = emit created(processedFullPath, newValue);
                } else {
                    result = emit updated(processedFullPath, newValue);
                }
            }
        }
        return result;
    }

/*
 * getSelf
 * Returns the SignalK document self key
 */
    QString Document::getSelf() {
        return m_root["self"].toString();
    }

/*
 * getVersion
 * Returns the SignalK document version
 */
    QString Document::getVersion() {
        return m_root["version"].toString();
    }

    QJsonValue Document::subtree(const QString &path) {
        QStringList parts = path.split(".");
        QJsonValue jsonValue = m_root;
        for (const auto &part: parts) {
            if (jsonValue.isObject()) {
                QJsonObject jsonObject = jsonValue.toObject();
                if (jsonObject.contains(part)) {
                    jsonValue = jsonObject[part];
                } else {
                    return {};
                }
            } else {
                break;
            }
        }

        return jsonValue;
    }

/*
 * save
 * Saves the current SignalK document state in a local file
 */
    void Document::save(QString fileName) {
        QFile jsonFile(fileName);
        jsonFile.open(QFile::WriteOnly);
        QJsonDocument jsonDocument;
        jsonDocument.setObject(m_root);
        jsonFile.write(jsonDocument.toJson());
    }

/*
 * load
 * Loads a new SignalK document state from a local file
 */
    void Document::load(QString fileName) {
        QFile jsonFile(fileName);
        jsonFile.open(QFile::ReadOnly);
        QString jsonString = jsonFile.readAll();
        QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());
    }

/*
 * setSelf
 * Inserts a new self key inside the SignalK document
 */
    void Document::setSelf(QString self) {
        insert("self", self);
    }

/*
 * getNavigationPosition
 * Helper method, calls the actual getNavigationPosition(QString) method
 */
    QGeoCoordinate Document::getNavigationPosition() {
        return getNavigationPosition(getSelf());
    }

/*
 * getNavigationPosition
 * Returns the navigation information received from the SignalK settings in form of a QGV GeoPos object
 */
    QGeoCoordinate Document::getNavigationPosition(const QString &uuid) {
        QGeoCoordinate result;
        QString path = uuid + ".navigation.position.value";

        QJsonValue positionValue = subtree(path);
        if (positionValue.isObject()) {
            double latitude = positionValue.toObject()["latitude"].toDouble();
            double longitude = positionValue.toObject()["longitude"].toDouble();
            result.setLatitude(latitude);
            result.setLongitude(longitude);
            if (positionValue.toObject().contains("altitude")) {
                result.setAltitude(positionValue.toObject()["altitude"].toDouble());
            }
        }
        return result;
    }

/*
 * getNavigationCourseOverGroundTrue
 * Helper method, calls the actual getNavigationCourseOverGroundTrue(QString) method
 */
    double Document::getNavigationCourseOverGroundTrue() {
        return getNavigationCourseOverGroundTrue(getSelf());
    }

    double Document::getNavigationCourseOverGroundTrue(const QString &uuid) {
        QString path = uuid + ".navigation.courseOverGroundTrue";
        double courseOverGroundTrue = subtree(path)["value"].toDouble();
        return courseOverGroundTrue;
    }

/*
 * getNavigationSpeedOverGround
 * Helper method, calls the actual getNavigationSpeedOverGround(QString) method
 */
    double Document::getNavigationSpeedOverGround() {
        return getNavigationSpeedOverGround(getSelf());
    }

    double Document::getNavigationSpeedOverGround(const QString &uuid) {
        QString path = uuid + ".navigation.speedOverGround";
        double speedOverGround = subtree(path)["value"].toDouble();
        return speedOverGround;
    }

/*
 * subscribe
 * Subscribes to a new SignalK key to receive new navigation infos
 */
    void Document::subscribe(const QString &fullPath, QObject *receiver, const char *member) {
        Subscription subscription(fullPath, receiver, member);
        subscriptions.append(subscription);
        connect(receiver, &QObject::destroyed, this, &Document::unsubscribe);
    }

/*
 * unsubscribe
 * Unsubscribes from a SignalK key to stop receiving navigation infos
 */
    void Document::unsubscribe(QObject *receiver) {
        QMutableListIterator<Subscription> i(subscriptions);
        while (i.hasNext()) {
            auto subscription = i.next();
            if (subscription.checkReceiver(receiver)) {
                i.remove();
            }
        }
    }

/**
 * Generate a UTC ISO8601-formatted timestamp
 * and return as std::string
 */
    QString Document::currentISO8601TimeUTC() {
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(gmtime(&itt), "%FT%TZ");
        return QString(ss.str().c_str());
    }

    QJsonObject Document::makeUpdate(const QString &fullPath) {
        QJsonObject updateObject;
        QStringList parts = fullPath.split(".");

        if (parts[0] == "vessels") {
            QString context = parts[0] + "." + parts[1];
            updateObject["context"] = context;
            QJsonArray updates;
            QJsonObject update;
            QJsonObject source;
            source["label"] = "FairWind++";
            source["type"] = "SignalK";
            update["source"] = source;
            update["timestamp"] = currentISO8601TimeUTC();
            QJsonArray values;
            QJsonObject valueObject;

            QString path = fullPath;
            QJsonValue value = subtree(fullPath);
            path = path.replace(context + ".", "");

            if (path.endsWith(".value")) {
                path = path.left(path.length() - 6);
            }
            valueObject["path"] = path;
            valueObject["value"] = value;
            values.append(valueObject);
            update["values"] = values;
            updates.append(update);
            updateObject["updates"] = updates;
        }

        return updateObject;
    }

/*
 * getRoot
 * Returns the root object
 */
    QJsonObject Document::getRoot() {
        return m_root;
    }

/*
 * setRoot
 * Sets the root object
 */
    void Document::setRoot(QJsonObject root) {
        this->m_root = root;
    }

/*
 * getMmsi(QString)
 * Returns the Marine Mobile Service Identity
 */
    QString Document::getMmsi(const QString &typeUuid) {
        QString result = "";
        QJsonValue jsonValue = subtree(typeUuid + ".mmsi");

        if (!jsonValue.isNull() && jsonValue.isString()) {
            result = jsonValue.toString();
        }

        return result;
    }

/*
 * getMmsi
 * Helper method, calls the actual getMmsi(QString) method
 */
    QString Document::getMmsi() {
        return getMmsi(getSelf());
    }

/*
 * getNavigationState(QString)
 * Returns the vessel's navigation state
 */
    QString Document::getNavigationState(const QString &typeUuid) {
        QString result = "";
        QJsonValue jsonValue = subtree(typeUuid + ".navigation.state");

        if (!jsonValue.isNull() && jsonValue.isObject() && jsonValue.toObject().contains("value")) {
            result = jsonValue.toObject()["value"].toString();
        }

        return result;
    }

/*
 * getNavigationState
 * Helper method, calls the actual getNavigationState(QString) method
 */
    QString Document::getNavigationState() {
        return getNavigationState(getSelf());
    }

    QString Document::generateUUID() {
        auto uuid = QUuid::createUuid();
        return "urn:mrn:signalk:uuid:" + uuid.toString(QUuid::WithoutBraces);
    }
}









