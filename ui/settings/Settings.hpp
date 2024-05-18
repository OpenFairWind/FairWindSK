//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QAbstractButton>
#include <QtZeroConf/qzeroconf.h>
#include <QtCore/qjsonobject.h>
#include "AppItem.hpp"
#include "Configuration.hpp"

//namespace Ui { class Settings; }

namespace fairwindsk::ui::settings {

    QT_BEGIN_NAMESPACE
    namespace Ui { class Settings; }
    QT_END_NAMESPACE


    class Settings : public QWidget {
    Q_OBJECT

    public:
        explicit Settings(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~Settings() override;

        QWidget *getCurrentWidget();

        //AppItem *getAppItemByHash(QString hash);
        //QList<QString> getAppsHashes();

        Configuration *getConfiguration();


    public slots:
        void onAccepted();
        void onRejected();
        void onClicked(QAbstractButton *button);

    signals:
        void accepted(Settings *);
        void rejected(Settings *);

    private:
        void initTabs();

    private:
        Ui::Settings *ui;

        Configuration m_configuration;
        //QMap<QString, AppItem *> m_mapHash2AppItem;
        QWidget *m_currentWidget;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
