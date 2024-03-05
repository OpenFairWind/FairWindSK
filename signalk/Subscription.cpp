//
// Created by Raffaele Montella on 01/03/22.
//

#include "Subscription.hpp"
#include "../FairWindSK.hpp"


namespace fairwindsk::signalk {
/*
 * Subscription - Public Constructor
 */
    Subscription::Subscription(const QString &fullPath, QObject *receiver, const char *member) {

        auto fairWindSK = FairWindSK::getInstance();
        auto signalK = fairWindSK->getSignalKDocument();
        auto self = signalK->getSelf();

        QString fullPathEx = QString(fullPath).replace("${self}",self);
        QString re = QString(fullPathEx).replace(".", "[.]").replace(":", "[:]").replace("*", ".*");
        //qDebug() << "Subscription::Subscription re: " << re;
        regularExpression = QRegularExpression(re);
        this->receiver = receiver;
        memberName = QString(member);


        int pos = memberName.lastIndexOf("::");
        memberName = memberName.right(memberName.length() - pos - 2);

        // Get the initial value invoking the SignalK API
        auto initialValue = signalK->get(fullPathEx);

        // Inject the value in the local document
        signalK->insert(fullPathEx,initialValue);
    }

/*
 * ~Subscription - Destructor
 */
    Subscription::~Subscription() = default;

/*
 * checkReceiver
 * Returns true is the provided receiver matches with the existent receiver in use
 */
    bool Subscription::checkReceiver(QObject *receiver) {
        if (this->receiver == receiver) return true;
        return false;
    }

    bool Subscription::match(Document *document, const QString &fullPath) {
        //qDebug() << "Subscription::match : " << regularExpression << " ? " << fullPath;
        if (regularExpression.match(fullPath).hasMatch()) {
            //qDebug() << "Subscription::match : " << regularExpression << " = " << fullPath << " !!!";
            QJsonObject updateObject = document->makeUpdate(fullPath);
            //qDebug() << "Subscription::match invokeMethod: " << memberName.toStdString().c_str();
            bool invokeResult = QMetaObject::invokeMethod(
                    receiver, memberName.toStdString().c_str(),
                    Qt::AutoConnection,Q_ARG(QJsonObject, updateObject));

            return invokeResult;
        }

        return false;
    }

/*
 * Subscription - Public Constructor
 */
    Subscription::Subscription(Subscription const &other) {
        this->regularExpression = other.regularExpression;
        this->receiver = other.receiver;
        this->memberName = other.memberName;
    }
}



