//
// Created by Raffaele Montella on 17/01/25.
//

#ifndef QPLIST_H
#define QPLIST_H

#include <QIODevice>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDomElement>



class PList {

    public:
        explicit PList(QIODevice *device);
        QString toPList(const QVariant &variant);
        QMap<QString, QVariant> toMap();

    private:
        QVariant parseElement(const QDomElement &e);
        QVariantList parseArrayElement(const QDomElement& node);
        QVariantMap parseDictElement(const QDomElement& element);

        QDomElement serializePrimitive(QDomDocument &doc, const QVariant &variant);
        QDomElement serializeElement(QDomDocument &doc, const QVariant &variant);
        QDomElement serializeMap(QDomDocument &doc, const QVariantMap &map);
        QDomElement serializeList(QDomDocument &doc, const QVariantList &list);

private:

        QVariant m_plist;
};



#endif //QPLIST_H
