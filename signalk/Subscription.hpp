//
// Created by Raffaele Montella on 31/03/24.
//

#ifndef WEB_UI_JSON_SIGNALKSUBSCRIPTION_HPP
#define WEB_UI_JSON_SIGNALKSUBSCRIPTION_HPP

#include <QString>
#include <QRegularExpression>
#include <QJsonObject>


namespace fairwindsk::signalk {
    class Subscription {

    public:
        Subscription() = default;

        Subscription(const QString &context, const QString &path, QObject *receiver, const char *member);

        Subscription(Subscription const &other);

        ~Subscription();

        bool match(const QString &fullPath, const QJsonObject& updateObject);

        bool checkReceiver(QObject *);

    private:
        QRegularExpression regularExpression;
        QObject *receiver;
        QString memberName;

    };
}

#endif //WEB_UI_JSON_SIGNALKSUBSCRIPTION_HPP
