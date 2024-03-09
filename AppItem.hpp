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

            explicit AppItem(QJsonObject jsonApp, bool active=true, int order=1);

            AppItem(const AppItem &app);

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

            void setWeb(ui::web::Web *pWeb);
            ui::web::Web *getWeb();

            bool operator<(const AppItem& o) const;

        private:
            QJsonObject m_jsonApp;
            bool m_active;
            int m_order;

            ui::web::Web *m_pWeb;
    };
}

#endif //APPITEM_HPP