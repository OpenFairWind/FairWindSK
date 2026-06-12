//
// Created by Raffaele Montella on 31/03/24.
//



#include "Subscription.hpp"

#include <QByteArray>

namespace fairwindsk::signalk {
    Subscription::Subscription(const QString &requestedContext,
                               const QString &context,
                               const QString &path,
                               QObject *receiver,
                               const char *member,
                               const int period,
                               const QString &policy,
                               const int minPeriod) {
        m_requestedContext = requestedContext;
        m_context = context;
        m_path = path;
        this->m_receiver = receiver;
        m_memberName = QString::fromLatin1(member ? member : "");
        m_period = period;
        const QString normalizedPolicy = policy.trimmed().toLower();
        m_policy = normalizedPolicy == QStringLiteral("fixed") ? QStringLiteral("fixed") : QStringLiteral("instant");
        m_minPeriod = minPeriod;

        // QMetaObject::invokeMethod() expects the bare member name; Qt will match
        // the QJsonObject argument supplied by Q_ARG below.
        if (!m_memberName.isEmpty() && m_memberName.front().isDigit()) {
            m_memberName.remove(0, 1);
        }
        const int classSeparator = m_memberName.lastIndexOf(QStringLiteral("::"));
        if (classSeparator >= 0) {
            m_memberName = m_memberName.mid(classSeparator + 2);
        }
        const int argumentList = m_memberName.indexOf(QLatin1Char('('));
        if (argumentList >= 0) {
            m_memberName = m_memberName.left(argumentList);
        }
        m_memberName = m_memberName.trimmed();

        rebuildRegularExpression();
    }

    Subscription::Subscription(const Subscription &other) {
        this->m_regularExpression = other.m_regularExpression;
        this->m_receiver = other.m_receiver;
        this->m_memberName = other.m_memberName;
        this->m_path = other.m_path;
        this->m_context = other.m_context;
        this->m_requestedContext = other.m_requestedContext;
        this->m_period = other.m_period;
        this->m_policy = other.m_policy;
        this->m_minPeriod = other.m_minPeriod;
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
    int Subscription::getPeriod() const { return m_period; }
    QString Subscription::getPolicy() const { return m_policy; }
    int Subscription::getMinPeriod() const { return m_minPeriod; }

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
        if (!m_receiver || m_memberName.isEmpty()) {
            return false;
        }

        const QByteArray memberName = m_memberName.toLatin1();
        const bool invoked = QMetaObject::invokeMethod(
                m_receiver, memberName.constData(),
                Qt::AutoConnection, Q_ARG(QJsonObject, updateObject));

        if (!invoked) {
            qWarning() << "Subscription::match invokeMethod failed for receiver"
                       << m_receiver << "method" << m_memberName
                       << "on path" << fullPath;
        }

        return invoked;
    }
}
