#ifndef FAIRWINDSK_UI_LAYOUT_BARRUNTIME_HPP
#define FAIRWINDSK_UI_LAYOUT_BARRUNTIME_HPP

#include <QHash>
#include <QPointer>
#include <QSizePolicy>
#include <QString>
#include <QVector>
#include <QWidget>

#include "BarLayout.hpp"
#include "ui/widgets/DataWidgetConfig.hpp"

class QHBoxLayout;
class QToolButton;

namespace fairwindsk::ui::widgets {
    class DataWidget;
}

namespace fairwindsk::ui::layout::runtime {
    QWidget *createSeparatorWidget(QWidget *parent,
                                   QVector<QPointer<QWidget>> &dynamicWidgets);
    QWidget *createDataWidget(const fairwindsk::ui::widgets::DataWidgetDefinition &definition,
                              const LayoutEntry &entry,
                              QWidget *parent,
                              QHash<QString, QPointer<fairwindsk::ui::widgets::DataWidget>> &dataWidgets);
    void clearConfiguredLayout(QHBoxLayout *layout,
                               QVector<QPointer<QWidget>> &dynamicWidgets,
                               QHash<QString, QPointer<fairwindsk::ui::widgets::DataWidget>> &dataWidgets);
    void addSeparatorToLayout(QHBoxLayout *layout,
                              QWidget *parent,
                              QVector<QPointer<QWidget>> &dynamicWidgets,
                              const LayoutEntry &entry);
    void addStretchToLayout(QHBoxLayout *layout,
                            const LayoutEntry &entry);
    void addWidgetToLayout(QHBoxLayout *layout,
                           const LayoutEntry &entry,
                           const QString &itemId,
                           QWidget *widget,
                           QHash<QString, QSizePolicy> &baseSizePolicies);
    void applyToolButtonDisplayOptions(QToolButton *button,
                                       const LayoutEntry &entry);
}

#endif // FAIRWINDSK_UI_LAYOUT_BARRUNTIME_HPP
