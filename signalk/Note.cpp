//
// Created by Raffaele Montella on 01/03/22.
//

#include <math.h>
#include "Document.hpp"
#include "Note.hpp"

namespace fairwindsk::signalk {



    Note::Note(const QString &title, const QString &description, const QGeoCoordinate &position) {

        setValueStringByKeyIfAny("title", title);
        setValueStringByKeyIfAny("description", description);
        setValueStringByKeyIfAny("mimeType", "text/plain");
        setValueStringByKeyIfAny("source", "fairwind++");

        QJsonObject jsonObjectPosition;
        jsonObjectPosition["latitude"] = position.latitude();
        jsonObjectPosition["longitude"] = position.longitude();
        if (!isnan(position.altitude())) {
            jsonObjectPosition["altitude"] = position.altitude();
        }

        this->operator[]("position") = jsonObjectPosition;

        this->operator[]("timestamp") = Document::currentISO8601TimeUTC();
    }

    QString Note::getValueStringByKeyOrEmpty(QString key) {
        if (this->contains(key) && this->operator[](key).isString()) {
            return this->operator[](key).toString();
        }
        return QString();
    }

    void Note::setValueStringByKeyIfAny(QString key, QString value) {
        if (value != nullptr && !value.isEmpty()) {
            this->operator[](key) = value;
        }
    }

    QString Note::title() { return getValueStringByKeyOrEmpty("title"); }

    QString Note::description() { return getValueStringByKeyOrEmpty("description"); }

    QString Note::region() { return getValueStringByKeyOrEmpty("region"); }

    QString Note::geohash() { return getValueStringByKeyOrEmpty("geohash"); }

    QString Note::mimeType() { return getValueStringByKeyOrEmpty("mimeType"); }

    QString Note::group() { return getValueStringByKeyOrEmpty("group"); }

    QString Note::url() { return getValueStringByKeyOrEmpty("url"); }

    QString Note::source() { return getValueStringByKeyOrEmpty("source"); }

    QDateTime Note::timestamp() { return QDateTime::fromString(getValueStringByKeyOrEmpty("timestamp"), "%FT%TZ"); }

    QStringList Note::authors() {
        QStringList authors;
        if (this->contains("authors") && this->operator[]("authors").isArray()) {
            for (auto item: this->operator[]("authors").toArray()) {
                if (item.isString()) {
                    authors.append(item.toString());
                }
            }
        }
        return authors;
    }

    QJsonObject Note::properties() {
        if (this->contains("properties") && this->operator[]("properties").isObject()) {
            return this->operator[]("properties").toObject();
        }
        return {};
    }

    QGeoCoordinate Note::position() {
        if (this->contains("position") && this->operator[]("position").isObject()) {
            auto jsonObjectPosition = this->operator[]("position").toObject();
            if (jsonObjectPosition.contains("latitude") && jsonObjectPosition.contains("longitude") &&
                jsonObjectPosition.contains("altitude") &&
                jsonObjectPosition["latitude"].isDouble() && jsonObjectPosition["longitude"].isDouble() &&
                jsonObjectPosition["altitude"].isDouble()) {
                return {
                        jsonObjectPosition["latitude"].toDouble(),
                        jsonObjectPosition["longitude"].toDouble(),
                        jsonObjectPosition["altitude"].toDouble()};
            } else if (jsonObjectPosition.contains("latitude") && jsonObjectPosition.contains("longitude") &&
                       jsonObjectPosition["latitude"].isDouble() && jsonObjectPosition["longitude"].isDouble()) {
                return {
                        jsonObjectPosition["latitude"].toDouble(),
                        jsonObjectPosition["longitude"].toDouble()};
            }
        }
        return {};
    }

    void Note::setTitle(QString value) { setValueStringByKeyIfAny("title", value); }

    void Note::setDescription(QString value) { setValueStringByKeyIfAny("description", value); }

    void Note::setRegion(QString value) { setValueStringByKeyIfAny("region", value); }

    void Note::setGeohash(QString value) { setValueStringByKeyIfAny("geohash", value); }

    void Note::setMimeType(QString value) { setValueStringByKeyIfAny("mimeType", value); }

    void Note::setGroup(QString value) { setValueStringByKeyIfAny("group", value); }

    void Note::setUrl(QString value) { setValueStringByKeyIfAny("url", value); }

    void Note::setSource(QString value) { setValueStringByKeyIfAny("source", value); }

    void Note::setProperties(QJsonObject properties) {
        this->operator[]("properties") = properties;
    }

    void Note::setAuthors(QStringList authors) {
        if (authors.size() > 0) {
            QJsonArray jsonArrayAuthors;
            for (auto author: authors) {
                jsonArrayAuthors.append(author);
            }

            this->operator[]("authors") = jsonArrayAuthors;
        }
    }

    void Note::setPosition(QGeoCoordinate position) {

        if (position.isValid()) {
            QJsonObject jsonObjectPosition;
            jsonObjectPosition["latitude"] = position.latitude();
            jsonObjectPosition["longitude"] = position.longitude();
            if (!isnan(position.altitude())) {
                jsonObjectPosition["altitude"] = position.altitude();
            }
        }
    }
}