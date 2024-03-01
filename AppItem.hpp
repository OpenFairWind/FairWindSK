//
// Created by Raffaele Montella on 27/03/21.
//

#ifndef APPITEM_HPP
#define APPITEM_HPP


#include <QMap>
#include <QJsonObject>
#include <QImage>


namespace fairwindsk {
    class AppItem: QObject {
    Q_OBJECT
    public:
    AppItem();

    AppItem(QJsonObject *appJson, bool active=false, int order=1);

    AppItem(const AppItem &app);

    QString getHash();
    QString getExtension();
    bool getActive();
    void setActive(bool active);
    int getOrder();
    void setOrder(int order);
    QString getName();
    QString getDesc();
    QString getVersion();
    QString getVendor();
    QString getCopyright();
    QString getLicense();
    QImage getIcon();
    QString getRoute();
    QMap<QString, QVariant> getArgs();

    bool operator<(const AppItem& o) const;

    private:
    void generateHash();

    QString m_hash;
    QString m_extension;
    bool m_active;
    int m_order;
    QString m_name;
    QString m_desc;
    QString m_version;
    QString m_vendor;
    QString m_copyright;
    QString m_license;
    QImage m_icon;
    QString m_route;
    QMap<QString, QVariant> m_args;
};
}

#endif //APPITEM_HPP