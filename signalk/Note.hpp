//
// Created by Raffaele Montella on 01/03/22.
//

#ifndef FAIRWINDSK_NOTE_HPP
#define FAIRWINDSK_NOTE_HPP

#include <QJsonObject>
#include <QJsonArray>
#include <QGeoCoordinate>
#include "../FairWindSK.hpp"

namespace fairwindsk::signalk {


    class Note : public QJsonObject {
    public:
        Note(const QString &title, const QString &description, const QGeoCoordinate &position);

        QString title();

        QString description();

        QGeoCoordinate position();

        QString region();

        QString geohash();

        QString mimeType();

        QString group();

        QString url();

        QStringList authors();

        QJsonObject properties();

        QString source();

        QDateTime timestamp();

        void setTitle(QString value);

        void setDescription(QString value);

        void setPosition(QGeoCoordinate position);

        void setRegion(QString value);

        void setGeohash(QString value);

        void setMimeType(QString value);

        void setGroup(QString value);

        void setUrl(QString value);

        void setAuthors(QStringList authors);

        void setProperties(QJsonObject properties);

        void setSource(QString value);

    private:
        QString getValueStringByKeyOrEmpty(QString key);

        void setValueStringByKeyIfAny(QString key, QString value);
    };
}

#endif //FAIRWINDSK_NOTE_HPP
