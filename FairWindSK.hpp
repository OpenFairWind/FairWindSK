//
// Created by Raffaele Montella on 03/04/21.
//

#ifndef FAIRWINDSK_FAIRWINDSK_HPP
#define FAIRWINDSK_FAIRWINDSK_HPP

#include <QString>
#include <QList>
#include <QMap>
#include <QJsonDocument>
#include "AppItem.hpp"
#include "signalk/Document.hpp"
#include "SignalKClient.hpp"

namespace fairwindsk {
    /*
     * FairWind
     * Singleton used to handle the entire FairWind ecosystem in a centralized way
     */
    class FairWindSK: public QObject {
        Q_OBJECT




    public:
        static FairWindSK *getInstance();

        QJsonObject getConfig();

        void startSignalK();


        void loadApps();
        AppItem *getAppItemByHash(QString hash);
        QString getAppHashById(QString appId);
        QList<QString> getAppsHashes();

        bool useVirtualKeyboard();

        signalk::Document *getSignalKDocument();

    private:

        QMap<QString, AppItem *> m_mapHash2AppItem;
        QMap<QString, QString> m_mapAppId2Hash;


        FairWindSK();
        inline static FairWindSK *m_instance = nullptr;

        signalk::Document m_signalkDocument;

        SignalKClient m_signalkClient;
    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP