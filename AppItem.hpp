//
// Created by Raffaele Montella on 27/03/21.
//

#ifndef APPITEM_HPP
#define APPITEM_HPP

#include <QMap>
#include <QJsonObject>
#include <QImage>

namespace fairwindsk::ui::web { class Web; }

namespace fairwindsk {

    class AppItem: QObject {
        Q_OBJECT



    public:
            AppItem();

            explicit AppItem(QJsonObject jsonApp);

            AppItem(const AppItem &app);

            void update(QJsonObject jsonApp);

            QString getDisplayName();

            bool getActive();
            void setActive(bool active);
            int getOrder();
            void setOrder(int order);
            QString getName();
            QString getDescription();
            QString getVersion();
            QString getVendor();
            QString getCopyright();
            QString getLicense();
            QString getAuthor();
            QVector<QString> getContributors();
            QString getUrl();
            QPixmap getIcon();
            QString getAppIcon();

            QStringList getArguments();

            //void setWeb(ui::web::Web *pWeb);
            //ui::web::Web *getWeb();

            void setWidget(QWidget *pWidget);
            QWidget *getWidget();

            void modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue);

            bool operator<(const AppItem& o) const;

        private:
            QJsonObject m_jsonApp;
            //bool m_active;
            //int m_order;

            //ui::web::Web *m_pWeb;
            QWidget *m_pWidget;
    };
}

#endif //APPITEM_HPP