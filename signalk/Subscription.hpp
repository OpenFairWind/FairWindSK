//
// Created by Raffaele Montella on 01/03/22.
//

#ifndef FAIRWINDSK_SIGNALK_SUBSCRIPTION_HPP
#define FAIRWINDSK_SIGNALK_SUBSCRIPTION_HPP

#include <QRegularExpression>
#include <QJsonObject>
#include "../FairWindSK.hpp"
#include "Document.hpp"


namespace fairwindsk::signalk {

    class Document;

    class  Subscription {
    public:
        Subscription() = default;

        Subscription(const QString &fullPath, QObject *receiver, const char *member);

        Subscription(Subscription const &other);

        ~Subscription();

        bool match(Document *document, const QString &fullPath);

        bool checkReceiver(QObject *);

    private:
        QRegularExpression regularExpression;
        QObject *receiver;
        QString memberName;
    };
}

#endif //FAIRWIND_SDK_SIGNALK_SUBSCRIPTION_HPP
