//
// Created by Raffaele Montella on 31/03/24.
//



#include "Subscription.hpp"

namespace fairwindsk::signalk {
    Subscription::Subscription(const QString &requestedContext,
                               const QString &context,
                               const QString &path,
                               QObject *receiver,
                               const char *member) {
        m_requestedContext = requestedContext;
        m_context = context;
        m_path = path;
        this->m_receiver = receiver;
        m_memberName = QString(member);

        int pos = m_memberName.lastIndexOf("::");
        m_memberName = m_memberName.right(m_memberName.length() - pos - 2);
        rebuildRegularExpression();
    }

    Subscription::Subscription(const Subscription &other) {
        this->m_regularExpression = other.m_regularExpression;
        this->m_receiver = other.m_receiver;
        this->m_memberName = other.m_memberName;
        this->m_path = other.m_path;
        this->m_context = other.m_context;
        this->m_requestedContext = other.m_requestedContext;
    }

    bool Subscription::checkReceiver(QObject *receiver) {
        if (this->m_receiver == receiver) return true;
        return false;
    }

    QString Subscription::getPath() const { return m_path; }
    QString Subscription::getContext() const { return m_context; }
    QString Subscription::getRequestedContext() const { return m_requestedContext; }
    QRegularExpression Subscription::getRegex() const { return m_regularExpression; }
    QObject *Subscription::getReceiver() const { return m_receiver; }

    void Subscription::retargetContext(const QString &context) {
        if (m_context == context) {
            return;
        }

        m_context = context;
        rebuildRegularExpression();
    }

    Subscription::~Subscription() = default;

    void Subscription::rebuildRegularExpression() {
        auto fullPath = m_context + "." + m_path;
        QString re = fullPath.replace(".", "[.]").replace(":", "[:]").replace("*", ".*");
        m_regularExpression = QRegularExpression(re);
    }

    bool Subscription::match(const QString &fullPath, const QJsonObject& updateObject) {
        if (!m_regularExpression.match(fullPath).hasMatch()) {
            return false;
        }

        const bool invoked = QMetaObject::invokeMethod(
                m_receiver, m_memberName.toStdString().c_str(),
                Qt::AutoConnection, Q_ARG(QJsonObject, updateObject));

        if (!invoked) {
            qWarning() << "Subscription::match invokeMethod failed for receiver"
                       << m_receiver << "method" << m_memberName
                       << "on path" << fullPath;
        }

        return invoked;
    }
}
