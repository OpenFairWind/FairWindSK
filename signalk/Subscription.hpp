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

        Subscription(const QString &requestedContext,
                     const QString &context,
                     const QString &path,
                     QObject *receiver,
                     const char *member);

        Subscription(Subscription const &other);

        ~Subscription();

        bool match(const QString &fullPath, const QJsonObject& updateObject);

        bool checkReceiver(QObject *);

        QString getPath() const;
        QString getContext() const;
        QString getRequestedContext() const;
        QRegularExpression getRegex() const;
        QObject *getReceiver() const;
        void retargetContext(const QString &context);

    private:
        void rebuildRegularExpression();

        QRegularExpression m_regularExpression;
        QObject *m_receiver = nullptr;
        QString m_memberName;
        QString m_path;
        QString m_requestedContext;
        QString m_context;

    };
}

#endif //WEB_UI_JSON_SIGNALKSUBSCRIPTION_HPP
