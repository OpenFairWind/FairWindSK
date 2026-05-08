#include "BarRuntime.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QSpacerItem>
#include <QToolButton>

#include "ui/widgets/DataWidget.hpp"

namespace fairwindsk::ui::layout::runtime {
    namespace {
        constexpr auto kStoredToolButtonTextProperty = "_fw_bar_layout_text";
    }

    QWidget *createSeparatorWidget(QWidget *parent,
                                   QVector<QPointer<QWidget>> &dynamicWidgets) {
        auto *line = new QFrame(parent);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Plain);
        line->setLineWidth(1);
        dynamicWidgets.append(line);
        return line;
    }

    QWidget *createDataWidget(const fairwindsk::ui::widgets::DataWidgetDefinition &definition,
                              const LayoutEntry &entry,
                              QWidget *parent,
                              QHash<QString, QPointer<fairwindsk::ui::widgets::DataWidget>> &dataWidgets) {
        if (definition.id.trimmed().isEmpty()) {
            return nullptr;
        }

        auto *widget = new fairwindsk::ui::widgets::DataWidget(definition, parent);
        widget->setDisplayOptions(entry.showIcon, entry.showText, entry.showUnits, entry.showTrend);
        dataWidgets.insert(definition.id, widget);
        return widget;
    }

    void clearConfiguredLayout(QHBoxLayout *layout,
                               QVector<QPointer<QWidget>> &dynamicWidgets,
                               QHash<QString, QPointer<fairwindsk::ui::widgets::DataWidget>> &dataWidgets) {
        if (!layout) {
            return;
        }

        while (layout->count() > 0) {
            auto *item = layout->takeAt(0);
            if (auto *widget = item->widget()) {
                widget->hide();
            }
            delete item;
        }

        for (auto &widget : dynamicWidgets) {
            if (widget) {
                widget->deleteLater();
            }
        }
        dynamicWidgets.clear();

        for (auto &widget : dataWidgets) {
            if (widget) {
                widget->deleteLater();
            }
        }
        dataWidgets.clear();
    }

    void addSeparatorToLayout(QHBoxLayout *layout,
                              QWidget *parent,
                              QVector<QPointer<QWidget>> &dynamicWidgets,
                              const LayoutEntry &entry) {
        if (!layout) {
            return;
        }

        auto *separator = createSeparatorWidget(parent, dynamicWidgets);
        QSizePolicy policy(QSizePolicy::Fixed,
                           entry.expandVertically ? QSizePolicy::Expanding : QSizePolicy::Fixed);
        separator->setSizePolicy(policy);
        layout->addWidget(separator,
                          entry.expandHorizontally ? 1 : 0,
                          entry.expandVertically ? Qt::Alignment() : Qt::AlignVCenter);
    }

    void addStretchToLayout(QHBoxLayout *layout,
                            const LayoutEntry &entry) {
        if (!layout) {
            return;
        }

        layout->addSpacerItem(
            new QSpacerItem(0,
                            0,
                            entry.expandHorizontally ? QSizePolicy::Expanding : QSizePolicy::Preferred,
                            entry.expandVertically ? QSizePolicy::Expanding : QSizePolicy::Minimum));
    }

    void addWidgetToLayout(QHBoxLayout *layout,
                           const LayoutEntry &entry,
                           const QString &itemId,
                           QWidget *widget,
                           QHash<QString, QSizePolicy> &baseSizePolicies) {
        if (!widget || !layout) {
            return;
        }

        if (!baseSizePolicies.contains(itemId)) {
            baseSizePolicies.insert(itemId, widget->sizePolicy());
        }

        const QSizePolicy basePolicy = baseSizePolicies.value(itemId, widget->sizePolicy());
        QSizePolicy effectivePolicy = basePolicy;
        if (entry.expandHorizontally) {
            effectivePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        }
        if (entry.expandVertically) {
            effectivePolicy.setVerticalPolicy(QSizePolicy::Expanding);
        }
        widget->setSizePolicy(effectivePolicy);
        widget->setVisible(true);

        const Qt::Alignment alignment = entry.expandVertically ? Qt::Alignment() : Qt::AlignVCenter;
        layout->addWidget(widget, entry.expandHorizontally ? 1 : 0, alignment);
    }

    void applyToolButtonDisplayOptions(QToolButton *button,
                                       const LayoutEntry &entry) {
        if (!button) {
            return;
        }

        QString storedText = button->property(kStoredToolButtonTextProperty).toString();
        if (storedText.trimmed().isEmpty() && !button->text().trimmed().isEmpty()) {
            storedText = button->text();
            button->setProperty(kStoredToolButtonTextProperty, storedText);
        }
        if (storedText.trimmed().isEmpty()) {
            storedText = button->toolTip();
            button->setProperty(kStoredToolButtonTextProperty, storedText);
        }

        button->setText(entry.showText ? storedText : QString());
        if (button->accessibleName().trimmed().isEmpty() && !storedText.trimmed().isEmpty()) {
            button->setAccessibleName(storedText);
        }

        if (entry.showIcon && entry.showText) {
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        } else if (entry.showIcon) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        } else {
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        }
    }
}
