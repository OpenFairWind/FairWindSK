//
// Created by Raffaele Montella on 12/04/21.
//

#ifndef TOPBAR_HPP
#define TOPBAR_HPP

#include <QIcon>
#include <QHBoxLayout>
#include <QPointer>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_TopBar.h"
#include "ui/layout/BarLayout.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"
#include "ui/widgets/SignalKStatusIconsWidget.hpp"

namespace Ui { class TopBar; }
class QGraphicsEffect;

namespace fairwindsk::ui::widgets {
    class DataWidget;
}

namespace fairwindsk::ui::topbar {


    class TopBar : public QWidget {
    Q_OBJECT
    public:
        explicit TopBar(QWidget *parent = nullptr);

        ~TopBar() override;
        static TopBar *instance();

        void setCurrentApp(AppItem *appItem);
        void setCurrentAppStatusSummary(const QString &summary);
        void setCurrentContext(const QString &name,
                               const QString &tooltip = QString(),
                               const QIcon &icon = QIcon(),
                               bool enableButton = false);
        void refreshFromConfiguration();
        void setLayoutEditHighlightEnabled(bool enabled);
        QWidget *widgetForItemId(const QString &itemId) const;

    public slots:
        void toolbuttonUL_clicked();
        void toolbuttonUR_clicked();

        void updateTime();

        signals:
        void clickedToolbuttonUL();
        void clickedToolbuttonUR();

    private:
        void changeEvent(QEvent *event) override;
        void updateComfortViewIcon() const;
        void applyFramelessRuntimeChrome() const;
        void resetCurrentAppPresentation() const;
        bool isLayoutWidgetActive(const QString &itemId) const;
        void rebuildLayout();
        QWidget *createContextWidget();
        QWidget *createDataWidget(const fairwindsk::ui::widgets::DataWidgetDefinition &definition,
                                  const fairwindsk::ui::layout::LayoutEntry &entry);
        QWidget *createSeparatorWidget();
        void clearConfiguredLayout();
        void applyEntrySizing(const fairwindsk::ui::layout::LayoutEntry &entry,
                              const QString &itemId,
                              QWidget *widget,
                              QHBoxLayout *layout);
        void clearLayoutEditHints();
        void applyLayoutEditHints(const QList<fairwindsk::ui::layout::LayoutEntry> &entries);

        Ui::TopBar *ui;
        QPointer<AppItem> m_currentApp;
        widgets::SignalKStatusIconsWidget *m_signalKStatusIcons = nullptr;
        QTimer *m_timer = nullptr;
        QWidget *m_contextWidget = nullptr;
        QHBoxLayout *m_contextLayout = nullptr;
        QHash<QString, QPointer<fairwindsk::ui::widgets::DataWidget>> m_dataWidgets;
        QVector<QPointer<QWidget>> m_dynamicLayoutWidgets;
        QHash<QString, QSizePolicy> m_baseSizePolicies;
        QHash<QWidget *, QPointer<QGraphicsEffect>> m_layoutHintEffects;
        QSet<QString> m_activeLayoutWidgetIds;
        bool m_layoutEditHighlightEnabled = false;
        inline static TopBar *s_instance = nullptr;
    };
}

#endif //TOPBAR_HPP
