//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QAbstractButton>
#include <QPushButton>
#include <QtZeroConf/qzeroconf.h>
#include <QtCore/qjsonobject.h>
#include "AppItem.hpp"
#include "Configuration.hpp"

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
        void initTabs(int currentIndex);
        void removeTabs();

    private:
        Ui::Settings *ui;

	QPushButton *m_pushButtonQuit;

        Configuration m_configuration;
        Configuration *m_currentConfiguration;
        QWidget *m_currentWidget;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
