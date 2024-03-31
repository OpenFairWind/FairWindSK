//
// Created by Raffaele Montella on 31/03/24.
//



#include "Subscription.hpp"

namespace fairwindsk::signalk {
    Subscription::Subscription(const QString &context, const QString &path,
                                             QObject *receiver, const char *member) {

        auto fullPath = context + "." + path;

        QString re = fullPath.replace(".", "[.]").replace(":", "[:]").replace("*", ".*");

        //qDebug() << "Subscription::Subscription re: " << re;
        regularExpression = QRegularExpression(re);

        this->receiver = receiver;
        memberName = QString(member);


        int pos = memberName.lastIndexOf("::");
        memberName = memberName.right(memberName.length() - pos - 2);

    }

    Subscription::Subscription(const Subscription &other) {
        this->regularExpression = other.regularExpression;
        this->receiver = other.receiver;
        this->memberName = other.memberName;
    }

    bool Subscription::checkReceiver(QObject *receiver) {
        if (this->receiver == receiver) return true;
        return false;
    }

    Subscription::~Subscription() = default;

    bool Subscription::match(const QString &fullPath, const QJsonObject& updateObject) {
        //qDebug() << "Subscription::match : " << regularExpression << " ? " << fullPath;

        if (regularExpression.match(fullPath).hasMatch()) {

            //qDebug() << "Subscription::match : " << regularExpression << " = " << fullPath << " !!!";

            //qDebug() << "Subscription::match invokeMethod: " << memberName.toStdString().c_str();
            bool invokeResult = QMetaObject::invokeMethod(
                    receiver, memberName.toStdString().c_str(),
                    Qt::AutoConnection,Q_ARG(QJsonObject, updateObject));

            return invokeResult;
        }

        return false;
    }
}