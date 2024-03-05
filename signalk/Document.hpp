//
// Created by Raffaele Montella on 18/04/21.
//

#ifndef FAIRWINDSK_SIGNALK_DOCUMENT_HPP
#define FAIRWINDSK_SIGNALK_DOCUMENT_HPP

#include <QRegularExpression>
#include <QJsonObject>
#include <QGeoCoordinate>

typedef void (*myfunc)();

namespace fairwindsk::signalk {

    class Subscription;

    class Document : public QObject {
    Q_OBJECT
    public:
        explicit Document();

        ~Document();

        void save(QString fileName);

        void load(QString fileName);

        typedef QString (*FunctionType)(void);

        void subscribe(const QString &fullPath, QObject *receiver, const char *member);

        QJsonObject makeUpdate(const QString &fullPath);

        void update(QJsonObject &jsonObjectUpdate);

        QJsonValue insert(const QString &path, const QJsonValue &newValue, int mode = 1);

        QJsonValue set(const QString &fullPath, const QJsonValue &newValue);

        QJsonValue get(const QString &fullPath);

        QJsonValue subtree(const QString &path);

        QString getSelf();

        void setSelf(QString self);

        QString getVersion();

        QJsonObject getRoot();

        void setRoot(QJsonObject root);

        QGeoCoordinate getNavigationPosition(const QString &uuid);

        QGeoCoordinate getNavigationPosition();

        double getNavigationCourseOverGroundTrue(const QString &uuid);

        double getNavigationCourseOverGroundTrue();

        double getNavigationSpeedOverGround(const QString &uuid);

        double getNavigationSpeedOverGround();

        QString getMmsi();

        QString getMmsi(const QString &typeUuid);

        QString getNavigationState();

        QString getNavigationState(const QString &typeUuid);

        static QString currentISO8601TimeUTC();

        QString generateUUID();

    public slots:

        void unsubscribe(QObject *receiver);

    signals:

        QJsonValue created(const QString &path, const QJsonValue &newValue);

        QJsonValue updated(const QString &path, const QJsonValue &newValue);

        QJsonValue fetched(const QString &path);

        void changed(const QString &path);

        void updatedNavigationPosition();

        void updatedNavigationCourseOverGroundTrue();

        void updatedNavigationSpeedOverGround();

    private:
        QJsonObject m_root;
        QList<Subscription> subscriptions;

        void modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue);

        void object2pathAndValue(QMap<QString, QJsonValue> &map, const QString &path, const QJsonValue value);
    };
}
#endif //FAIRWINDSK_SIGNALK_DOCUMENT_HPP