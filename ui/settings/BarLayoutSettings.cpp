#include "BarLayoutSettings.hpp"

#include <QAbstractItemModel>
#include <QGridLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QScroller>
#include <QScrollerProperties>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    using fairwindsk::ui::layout::BarId;
    using fairwindsk::ui::layout::EntryKind;
    using fairwindsk::ui::layout::LayoutEntry;

    BarLayoutSettings::BarLayoutSettings(Settings *settings,
                                         const BarId barId,
                                         QWidget *parent)
        : QWidget(parent),
          m_settings(settings),
          m_barId(barId) {
        buildUi();
        populateFromConfiguration();
    }

    void BarLayoutSettings::refreshFromConfiguration() {
        populateFromConfiguration();
    }

    void BarLayoutSettings::buildUi() {
        constexpr int kControlHeight = 52;
        constexpr int kRowHeight = 76;

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        rootLayout->setSpacing(12);

        m_titleLabel = new QLabel(layout::barLabel(m_barId), this);
        rootLayout->addWidget(m_titleLabel);

        m_hintLabel = new QLabel(
            tr("Tap a checkbox to place a widget on this bar. Press and drag a row to reorder it. Add separators and elastic extenders to build clear MFD-style groups."),
            this);
        m_hintLabel->setWordWrap(true);
        rootLayout->addWidget(m_hintLabel);

        auto *buttonGrid = new QGridLayout();
        buttonGrid->setContentsMargins(0, 0, 0, 0);
        buttonGrid->setHorizontalSpacing(8);
        buttonGrid->setVerticalSpacing(8);
        rootLayout->addLayout(buttonGrid);

        m_addSeparatorButton = new QPushButton(tr("Add Separator"), this);
        m_addStretchButton = new QPushButton(tr("Add Extender"), this);
        m_removeSelectedButton = new QPushButton(tr("Remove Selected"), this);
        m_resetDefaultsButton = new QPushButton(tr("Reset Defaults"), this);
        for (auto *button : {m_addSeparatorButton, m_addStretchButton, m_removeSelectedButton, m_resetDefaultsButton}) {
            button->setMinimumHeight(kControlHeight);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        buttonGrid->addWidget(m_addSeparatorButton, 0, 0);
        buttonGrid->addWidget(m_addStretchButton, 0, 1);
        buttonGrid->addWidget(m_removeSelectedButton, 1, 0);
        buttonGrid->addWidget(m_resetDefaultsButton, 1, 1);

        m_listWidget = new QListWidget(this);
        m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
        m_listWidget->setDragEnabled(true);
        m_listWidget->setAcceptDrops(true);
        m_listWidget->setDragDropOverwriteMode(false);
        m_listWidget->setDefaultDropAction(Qt::MoveAction);
        m_listWidget->setDropIndicatorShown(true);
        m_listWidget->setAlternatingRowColors(true);
        m_listWidget->setSpacing(8);
        m_listWidget->setUniformItemSizes(false);
        m_listWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::LeftMouseButtonGesture);
        auto scrollerProperties = QScroller::scroller(m_listWidget->viewport())->scrollerProperties();
        scrollerProperties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
        scrollerProperties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.55);
        QScroller::scroller(m_listWidget->viewport())->setScrollerProperties(scrollerProperties);
        m_listWidget->setStyleSheet(QStringLiteral(
            "QListWidget {"
            " border-radius: 10px;"
            " }"
            "QListWidget::item {"
            " padding: 12px 14px;"
            " border: 1px solid palette(mid);"
            " border-radius: 8px;"
            " }"
            "QListWidget::item:selected {"
            " border-color: palette(highlight);"
            " font-weight: 600;"
            " }"));
        rootLayout->addWidget(m_listWidget, 1);

        connect(m_listWidget, &QListWidget::itemChanged, this, &BarLayoutSettings::onItemChanged);
        connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &BarLayoutSettings::onSelectionChanged);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
            if (!m_populating) {
                persistToConfiguration();
            }
        });
        connect(m_addSeparatorButton, &QPushButton::clicked, this, &BarLayoutSettings::onAddSeparator);
        connect(m_addStretchButton, &QPushButton::clicked, this, &BarLayoutSettings::onAddStretch);
        connect(m_removeSelectedButton, &QPushButton::clicked, this, &BarLayoutSettings::onRemoveSelected);
        connect(m_resetDefaultsButton, &QPushButton::clicked, this, &BarLayoutSettings::onResetDefaults);

        updateActions();
    }

    QListWidgetItem *BarLayoutSettings::createItem(const LayoutEntry &entry) {
        constexpr int kRowHeight = 76;

        auto *item = new QListWidgetItem(layout::entryLabel(entry));
        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled |
                       Qt::ItemIsUserCheckable);
        item->setCheckState(entry.enabled ? Qt::Checked : Qt::Unchecked);
        item->setSizeHint(QSize(0, kRowHeight));
        return item;
    }

    LayoutEntry BarLayoutSettings::entryForItem(const QListWidgetItem *item) const {
        LayoutEntry entry;
        if (!item) {
            return entry;
        }

        entry.kind = static_cast<EntryKind>(item->data(RoleKind).toInt());
        entry.widgetId = item->data(RoleWidgetId).toString();
        entry.instanceId = item->data(RoleInstanceId).toString();
        entry.enabled = item->checkState() == Qt::Checked;
        return entry;
    }

    void BarLayoutSettings::populateFromConfiguration() {
        if (!m_settings || !m_listWidget) {
            return;
        }

        m_populating = true;
        const QSignalBlocker blocker(m_listWidget);
        m_listWidget->clear();

        const auto entries = layout::entriesForBar(m_settings->getConfiguration()->getRoot(), m_barId);
        for (const auto &entry : entries) {
            m_listWidget->addItem(createItem(entry));
        }

        m_populating = false;
        updateActions();
    }

    void BarLayoutSettings::persistToConfiguration() {
        if (m_populating || !m_settings) {
            return;
        }

        QList<LayoutEntry> entries;
        entries.reserve(m_listWidget->count());
        for (int index = 0; index < m_listWidget->count(); ++index) {
            auto *item = m_listWidget->item(index);
            if (!item) {
                continue;
            }
            entries.append(entryForItem(item));
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        layout::setEntriesForBar(root, m_barId, entries);

        const auto otherBarId = m_barId == BarId::Top ? BarId::Bottom : BarId::Top;
        for (const auto &entry : entries) {
            if (entry.kind == EntryKind::Widget && entry.enabled) {
                layout::removeWidgetFromBar(root, otherBarId, entry.widgetId);
            }
        }

        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void BarLayoutSettings::updateActions() {
        const auto *selectedItem = m_listWidget ? m_listWidget->currentItem() : nullptr;
        const auto selectedEntry = entryForItem(selectedItem);
        const bool canRemove = selectedItem && layout::isPlaceholderEntry(selectedEntry);
        if (m_removeSelectedButton) {
            m_removeSelectedButton->setEnabled(canRemove);
        }
    }

    void BarLayoutSettings::appendPlaceholder(const EntryKind kind) {
        if (!m_listWidget) {
            return;
        }

        LayoutEntry entry;
        entry.kind = kind;
        entry.instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.enabled = true;

        m_listWidget->addItem(createItem(entry));
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onItemChanged(QListWidgetItem *item) {
        Q_UNUSED(item)
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onSelectionChanged() {
        updateActions();
    }

    void BarLayoutSettings::onAddSeparator() {
        appendPlaceholder(EntryKind::Separator);
    }

    void BarLayoutSettings::onAddStretch() {
        appendPlaceholder(EntryKind::Stretch);
    }

    void BarLayoutSettings::onRemoveSelected() {
        const int row = m_listWidget ? m_listWidget->currentRow() : -1;
        if (row < 0) {
            return;
        }

        const auto entry = entryForItem(m_listWidget->item(row));
        if (!layout::isPlaceholderEntry(entry)) {
            return;
        }

        delete m_listWidget->takeItem(row);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onResetDefaults() {
        if (!m_settings) {
            return;
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        const auto defaults = layout::defaultEntries(m_barId);
        layout::setEntriesForBar(root, m_barId, defaults);
        const auto otherBarId = m_barId == BarId::Top ? BarId::Bottom : BarId::Top;
        for (const auto &entry : defaults) {
            if (entry.kind == EntryKind::Widget && entry.enabled) {
                layout::removeWidgetFromBar(root, otherBarId, entry.widgetId);
            }
        }
        populateFromConfiguration();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }
}
