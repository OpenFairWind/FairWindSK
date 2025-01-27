//
// Created by Raffaele Montella on 27/03/21.
//

#ifndef APPITEM_HPP
#define APPITEM_HPP

#include <QObject>
#include <QMap>
#include <QImage>
#include <QProcess>
#include <QString>
#include <nlohmann/json.hpp>

namespace fairwindsk::ui::web { class Web; }

namespace fairwindsk {

    class AppItem: QObject {
        Q_OBJECT



    public:
            AppItem();

            explicit AppItem(nlohmann::json jsonApp);

            // Destructor
            ~AppItem() override;

            AppItem(const AppItem &app);

            void update(const nlohmann::json&  jsonApp);

            QString getDisplayName();
            void setDisplayName(const QString& displayName);

            bool getActive();
            void setActive(bool active);
            int getOrder();
            void setOrder(int order);

            QString getName();
            void setName(const QString& name);

            QString getDescription();
            void setDescription(const QString& description);

            QString getVersion();
            QString getVendor();
            QString getCopyright();
            QString getLicense();
            QString getAuthor();
            QVector<QString> getContributors();
            QString getUrl();
            QPixmap getIcon();
            QString getAppIcon();
            void setAppIcon(const QString& appIcon);

            QString getSettingsUrl(const QString& pluginUrl);
            void setSettingsUrl(const QString& settingsUrl);

            QString getAboutUrl(const QString& pluginUrl);
            void setAboutUrl(const QString& aboutUrl);

            QString getHelpUrl(const QString& pluginUrl);
            void setHelpUrl(const QString& helpUrl);

            QStringList getArguments();

            void setWidget(QWidget *pWidget);
            QWidget *getWidget();

            void setProcess(QProcess *pProcess);
            QProcess *getProcess();

            nlohmann::json asJson();

            bool operator<(const AppItem& o) const;

        private:
            nlohmann::json m_jsonApp;

            QWidget *m_pWidget = nullptr;
            QProcess *m_pProcess = nullptr;
    };
}

#endif //APPITEM_HPP