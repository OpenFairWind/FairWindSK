//
// Created by Raffaele Montella on 31/03/24.
//



#include "Subscription.hpp"

namespace fairwindsk::signalk {
    Subscription::Subscription(const QString &context, const QString &path,
                                             QObject *receiver, const char *member) {



        m_context = context;
        m_path = path;

        auto fullPath = m_context + "." + m_path;

        QString re = fullPath.replace(".", "[.]").replace(":", "[:]").replace("*", ".*");

        //qDebug() << "Subscription::Subscription re: " << re;
        m_regularExpression = QRegularExpression(re);

        this->m_receiver = receiver;
        m_memberName = QString(member);


        int pos = m_memberName.lastIndexOf("::");
        m_memberName = m_memberName.right(m_memberName.length() - pos - 2);

    }

    Subscription::Subscription(const Subscription &other) {
        this->m_regularExpression = other.m_regularExpression;
        this->m_receiver = other.m_receiver;
        this->m_memberName = other.m_memberName;
    }

    bool Subscription::checkReceiver(QObject *receiver) {
        if (this->m_receiver == receiver) return true;
        return false;
    }

    QString Subscription::getPath() { return m_path; }
    QString Subscription::getContext() { return m_context; }
    QRegularExpression Subscription::getRegex() { return m_regularExpression; }
    QObject *Subscription::getReceiver() const { return m_receiver; }

    Subscription::~Subscription() = default;

    bool Subscription::match(const QString &fullPath, const QJsonObject& updateObject) {
        //qDebug() << "Subscription::match : " << regularExpression << " ? " << fullPath;

        if (m_regularExpression.match(fullPath).hasMatch()) {

            //qDebug() << "Subscription::match : " << regularExpression << " = " << fullPath << " !!!";

            //qDebug() << "Subscription::match invokeMethod: " << memberName.toStdString().c_str();
            bool invokeResult = QMetaObject::invokeMethod(
                    m_receiver, m_memberName.toStdString().c_str(),
                    Qt::AutoConnection,Q_ARG(QJsonObject, updateObject));

            return invokeResult;
        }

        return false;
    }
}