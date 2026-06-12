//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QAbstractButton>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <QtCore/qjsonobject.h>
#include "AppItem.hpp"
#include "Configuration.hpp"
#include "ui/layout/BarLayout.hpp"

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
        void markDirty(quint32 runtimeChanges = 0, int delayMs = -1);
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
        void refreshLayoutEditHighlightMode();

    signals:
        void layoutEditHighlightModeChanged(bool topBarActive, bool bottomBarActive);

    private:
        void initTabs(int currentIndex);
        void removeTabs();
        void applyConfiguration();
        void scheduleApplyConfiguration(int delayMs = 150);
        QWidget *createTabWidget(int index);
        void ensureTabCreated(int index);
        void emitLayoutEditHighlightModeChanged();

    private:
        Ui::Settings *ui;

        Configuration m_configuration;
        Configuration *m_currentConfiguration;
        QWidget *m_currentWidget = nullptr;
        QVector<QPointer<QWidget>> m_tabPages;
        bool m_rebuildingTabs = false;
        bool m_hasPendingUiChanges = false;
        QTimer *m_applyTimer = nullptr;
        quint32 m_pendingRuntimeChanges = 0;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
