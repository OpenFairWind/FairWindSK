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

        QString getPath();
        QString getContext();
        QRegularExpression getRegex();
        QObject *getReceiver() const;

    private:
        QRegularExpression m_regularExpression;
        QObject *m_receiver = nullptr;
        QString m_memberName;
        QString m_path;
        QString m_context;

    };
}

#endif //WEB_UI_JSON_SIGNALKSUBSCRIPTION_HPP
