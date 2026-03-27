//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QAbstractButton>
#include <QVector>
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
        void setCurrentWidget(QWidget *currentWidget);
        bool hasPendingChanges();
        void markDirty();
        void saveChanges();
        void discardChanges();
        void resetFromCurrentConfiguration(int currentIndex = -1);

        //AppItem *getAppItemByHash(QString hash);
        //QList<QString> getAppsHashes();

        Configuration *getConfiguration();
        void resetToCurrentConfiguration();
        void restoreDefaultConfiguration();
        void restartApplication();
        void quitApplication();


    public slots:
        void onClicked(QAbstractButton *button);
        void onTabChanged(int index);

    private:
        void initTabs(int currentIndex);
        void removeTabs();
        void applyConfiguration();
        QWidget *createTabWidget(int index);
        void ensureTabCreated(int index);

    private:
        Ui::Settings *ui;

        Configuration m_configuration;
        Configuration *m_currentConfiguration;
        QWidget *m_currentWidget = nullptr;
        QVector<QWidget *> m_tabPages;
        bool m_rebuildingTabs = false;
        bool m_hasPendingUiChanges = false;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
