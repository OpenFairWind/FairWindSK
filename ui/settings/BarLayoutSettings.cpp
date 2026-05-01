#include "BarLayoutSettings.hpp"

#include <QAbstractItemModel>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QScroller>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "BarSettingsUi.hpp"
#include "LayoutPreviewListWidget.hpp"
#include "ui/IconUtils.hpp"

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

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(8);

        m_previewFrame = new QFrame(this);
        m_previewFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_previewFrame->setMinimumHeight(100);
        m_previewFrame->setMaximumHeight(100);
        auto *previewLayout = new QVBoxLayout(m_previewFrame);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(0);

        m_minimumWidthButton = new QToolButton(this);
        m_maximumWidthButton = new QToolButton(this);
        m_minimumHeightButton = new QToolButton(this);
        m_expandHeightButton = new QToolButton(this);
        m_moveLeftButton = new QToolButton(this);
        m_moveRightButton = new QToolButton(this);
        m_removeSelectedButton = new QToolButton(this);
        m_resetDefaultsButton = new QToolButton(this);
        barsettings::configureSizeButton(m_minimumWidthButton,
                                         QStringLiteral(":/resources/svg/OpenBridge/layout-horizontal-minimize.svg"),
                                         tr("Keep the selected widget at the minimum needed size"),
                                         tr("Minimize horizontally"),
                                         kControlHeight);
        barsettings::configureSizeButton(m_maximumWidthButton,
                                         QStringLiteral(":/resources/svg/OpenBridge/layout-horizontal-maximize.svg"),
                                         tr("Let the selected widget grow to the maximum possible size"),
                                         tr("Maximize horizontally"),
                                         kControlHeight);
        barsettings::configureSizeButton(m_minimumHeightButton,
                                         QStringLiteral(":/resources/svg/OpenBridge/layout-vertical-minimize.svg"),
                                         tr("Keep the selected widget at the normal bar height"),
                                         tr("Minimize vertically"),
                                         kControlHeight);
        barsettings::configureSizeButton(m_expandHeightButton,
                                         QStringLiteral(":/resources/svg/OpenBridge/layout-vertical-maximize.svg"),
                                         tr("Let the selected widget use the full bar height"),
                                         tr("Maximize vertically"),
                                         kControlHeight);
        barsettings::configureActionButton(m_moveLeftButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg"),
                                           tr("Move selected widget left"),
                                           kControlHeight);
        barsettings::configureActionButton(m_moveRightButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg"),
                                           tr("Move selected widget right"),
                                           kControlHeight);
        barsettings::configureActionButton(m_removeSelectedButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg"),
                                           tr("Remove Selected"),
                                           kControlHeight);
        barsettings::configureActionButton(m_resetDefaultsButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg"),
                                           tr("Reset Defaults"),
                                           kControlHeight);

        m_listWidget = new LayoutPreviewListWidget(this);
        m_listWidget->setItemFactory([this](const LayoutEntry &entry) {
            return createItem(entry);
        });
        m_listWidget->setEditedCallback([this]() { onPreviewEdited(); });
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_listWidget->setSpacing(6);
        m_listWidget->setUniformItemSizes(false);
        m_listWidget->setIconSize(QSize(32, 32));
        m_listWidget->setMinimumHeight(94);
        m_listWidget->setMaximumHeight(94);
        m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_listWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::TouchGesture);
        m_listWidget->setToolTip(tr("Tap a widget to edit it. Drag within the preview to reorder the bottom bar."));
        previewLayout->addWidget(m_listWidget);
        rootLayout->addWidget(m_previewFrame);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);
        controlsLayout->addWidget(m_minimumWidthButton);
        controlsLayout->addWidget(m_maximumWidthButton);
        controlsLayout->addWidget(m_minimumHeightButton);
        controlsLayout->addWidget(m_expandHeightButton);
        controlsLayout->addWidget(m_moveLeftButton);
        controlsLayout->addWidget(m_moveRightButton);
        controlsLayout->addWidget(m_removeSelectedButton);
        controlsLayout->addWidget(m_resetDefaultsButton);
        controlsLayout->addStretch(1);
        rootLayout->addLayout(controlsLayout);

        m_paletteWidget = new WidgetPalette(this);
        m_paletteWidget->setToolTip(tr("Tap or drag a palette item to place it on the preview bar."));
        rootLayout->addWidget(m_paletteWidget);

        QList<LayoutEntry> paletteEntries;
        LayoutEntry separatorEntry;
        separatorEntry.kind = EntryKind::Separator;
        separatorEntry.instanceId = QStringLiteral("separator");
        paletteEntries.append(separatorEntry);

        LayoutEntry stretchEntry;
        stretchEntry.kind = EntryKind::Stretch;
        stretchEntry.instanceId = QStringLiteral("stretch");
        stretchEntry.expandHorizontally = true;
        paletteEntries.append(stretchEntry);

        const auto definitions = layout::widgetDefinitions();
        for (const auto &definition : definitions) {
            LayoutEntry entry;
            entry.kind = EntryKind::Widget;
            entry.widgetId = definition.id;
            entry.instanceId = definition.id;
            entry.expandHorizontally = defaultExpandHorizontally(definition.id);
            paletteEntries.append(entry);
        }
        m_paletteWidget->setEntries(paletteEntries);

        connect(m_listWidget, &QListWidget::itemChanged, this, &BarLayoutSettings::onItemChanged);
        connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &BarLayoutSettings::onSelectionChanged);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsInserted, this, &BarLayoutSettings::onPreviewEdited);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsRemoved, this, &BarLayoutSettings::onPreviewEdited);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this, &BarLayoutSettings::onPreviewEdited);
        connect(m_paletteWidget, &WidgetPalette::entryActivated, this, &BarLayoutSettings::onPaletteEntryActivated);
        connect(m_minimumWidthButton, &QToolButton::clicked, this, &BarLayoutSettings::onMinimumWidthSelected);
        connect(m_maximumWidthButton, &QToolButton::clicked, this, &BarLayoutSettings::onMaximumWidthSelected);
        connect(m_minimumHeightButton, &QToolButton::clicked, this, &BarLayoutSettings::onMinimumHeightSelected);
        connect(m_expandHeightButton, &QToolButton::clicked, this, &BarLayoutSettings::onMaximumHeightSelected);
        connect(m_moveLeftButton, &QToolButton::clicked, this, &BarLayoutSettings::onMoveSelectedLeft);
        connect(m_moveRightButton, &QToolButton::clicked, this, &BarLayoutSettings::onMoveSelectedRight);
        connect(m_removeSelectedButton, &QToolButton::clicked, this, &BarLayoutSettings::onRemoveSelected);
        connect(m_resetDefaultsButton, &QToolButton::clicked, this, &BarLayoutSettings::onResetDefaults);

        applyChrome();
        updateActions();
    }

    void BarLayoutSettings::applyChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration() : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 18.0);
        barsettings::applySecondaryLabelStyle(m_previewLabel, chrome);
        barsettings::applySecondaryLabelStyle(m_paletteLabel, chrome);
        barsettings::applyHintLabelStyle(m_hintLabel, chrome);
        if (m_previewFrame) {
            m_previewFrame->setStyleSheet(barsettings::previewFrameChrome(chrome));
        }
        if (m_listWidget) {
            m_listWidget->setStyleSheet(barsettings::listWidgetChrome(chrome, false));
        }
        if (m_paletteWidget) {
            m_paletteWidget->setStyleSheet(barsettings::paletteChrome(chrome));
        }

        barsettings::applySizeButtonChrome(m_minimumWidthButton, chrome, 52);
        barsettings::applySizeButtonChrome(m_maximumWidthButton, chrome, 52);
        barsettings::applySizeButtonChrome(m_minimumHeightButton, chrome, 52);
        barsettings::applySizeButtonChrome(m_expandHeightButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_moveLeftButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_moveRightButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_removeSelectedButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_resetDefaultsButton, chrome, 52);
    }

    QListWidgetItem *BarLayoutSettings::createItem(const LayoutEntry &entry) {
        auto *item = new QListWidgetItem(layout::entryLabel(entry));
        item->setIcon(WidgetPalette::entryIcon(entry));
        LayoutPreviewListWidget::applyEntryData(item, entry);
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(itemSizeHint(entry));
        refreshPreviewItem(item);
        return item;
    }

    void BarLayoutSettings::refreshPreviewItem(QListWidgetItem *item) const {
        if (!item) {
            return;
        }

        const QSignalBlocker blocker(m_listWidget);
        const auto entry = entryForItem(item);
        item->setText(barsettings::previewEntryText(entry));
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(itemSizeHint(entry));
    }

    QSize BarLayoutSettings::itemSizeHint(const LayoutEntry &entry) const {
        int width = entry.expandHorizontally ? 124 : 90;
        if (entry.kind == EntryKind::Separator) {
            width = 48;
        } else if (entry.kind == EntryKind::Stretch) {
            width = 124;
        }

        return QSize(width, entry.expandVertically ? 82 : 78);
    }

    LayoutEntry BarLayoutSettings::entryForItem(const QListWidgetItem *item) const {
        return LayoutPreviewListWidget::entryForItem(item);
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
            if (!entry.enabled) {
                continue;
            }
            m_listWidget->addItem(createItem(entry));
        }

        m_populating = false;
        if (m_paletteWidget) {
            m_paletteWidget->setActiveEntries(entries);
        }
        updateActions();
    }

    void BarLayoutSettings::persistToConfiguration() {
        if (m_populating || !m_settings) {
            return;
        }

        QList<LayoutEntry> entries;
        entries.reserve(m_listWidget->count() + layout::widgetDefinitions().size());
        QStringList enabledWidgetIds;
        for (int index = 0; index < m_listWidget->count(); ++index) {
            auto *item = m_listWidget->item(index);
            if (!item) {
                continue;
            }
            const auto entry = entryForItem(item);
            entries.append(entry);
            if (entry.kind == EntryKind::Widget) {
                enabledWidgetIds.append(entry.widgetId);
            }
        }

        const auto definitions = layout::widgetDefinitions();
        for (const auto &definition : definitions) {
            if (enabledWidgetIds.contains(definition.id)) {
                continue;
            }

            LayoutEntry entry;
            entry.kind = EntryKind::Widget;
            entry.widgetId = definition.id;
            entry.instanceId = definition.id;
            entry.enabled = false;
            entry.expandHorizontally = defaultExpandHorizontally(definition.id);
            entries.append(entry);
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
        if (m_paletteWidget) {
            m_paletteWidget->setActiveEntries(entries);
        }
    }

    void BarLayoutSettings::updateActions() {
        const auto entry = entryForItem(m_listWidget ? m_listWidget->currentItem() : nullptr);
        const bool hasSelection = m_listWidget && m_listWidget->currentItem();
        const bool hasWidgetSelection = hasSelection && entry.kind == EntryKind::Widget;
        const int currentRow = m_listWidget ? m_listWidget->currentRow() : -1;
        const int itemCount = m_listWidget ? m_listWidget->count() : 0;

        if (m_minimumWidthButton) {
            const QSignalBlocker blocker(m_minimumWidthButton);
            m_minimumWidthButton->setEnabled(hasWidgetSelection);
            m_minimumWidthButton->setChecked(hasWidgetSelection && !entry.expandHorizontally);
        }
        if (m_maximumWidthButton) {
            const QSignalBlocker blocker(m_maximumWidthButton);
            m_maximumWidthButton->setEnabled(hasWidgetSelection);
            m_maximumWidthButton->setChecked(hasWidgetSelection && entry.expandHorizontally);
        }
        if (m_minimumHeightButton) {
            const QSignalBlocker blocker(m_minimumHeightButton);
            m_minimumHeightButton->setEnabled(hasSelection);
            m_minimumHeightButton->setChecked(hasSelection && !entry.expandVertically);
        }
        if (m_expandHeightButton) {
            const QSignalBlocker blocker(m_expandHeightButton);
            m_expandHeightButton->setEnabled(hasSelection);
            m_expandHeightButton->setChecked(hasSelection && entry.expandVertically);
        }
        if (m_moveLeftButton) {
            m_moveLeftButton->setEnabled(hasSelection && currentRow > 0);
        }
        if (m_moveRightButton) {
            m_moveRightButton->setEnabled(hasSelection && currentRow >= 0 && currentRow < itemCount - 1);
        }
        if (m_removeSelectedButton) {
            m_removeSelectedButton->setEnabled(hasSelection);
        }
    }

    void BarLayoutSettings::appendPlaceholder(const EntryKind kind) {
        if (!m_listWidget) {
            return;
        }

        LayoutEntry entry;
        entry.kind = kind;
        entry.enabled = true;
        entry.expandHorizontally = kind == EntryKind::Stretch;
        entry.expandVertically = false;

        m_listWidget->addOrSelectEntry(entry);
    }

    void BarLayoutSettings::onItemChanged(QListWidgetItem *item) {
        Q_UNUSED(item)
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onSelectionChanged() {
        updateActions();
    }

    void BarLayoutSettings::onMinimumWidthSelected() {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(LayoutPreviewListWidget::RoleExpandHorizontally, false);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onMaximumWidthSelected() {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(LayoutPreviewListWidget::RoleExpandHorizontally, true);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onMinimumHeightSelected() {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(LayoutPreviewListWidget::RoleExpandVertically, false);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onMaximumHeightSelected() {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(LayoutPreviewListWidget::RoleExpandVertically, true);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::moveCurrentItem(const int offset) {
        if (!m_listWidget) {
            return;
        }

        m_listWidget->moveCurrentItemBy(offset);
    }

    void BarLayoutSettings::onMoveSelectedLeft() {
        moveCurrentItem(-1);
    }

    void BarLayoutSettings::onMoveSelectedRight() {
        moveCurrentItem(1);
    }

    void BarLayoutSettings::activateWidgetEntry(const QString &widgetId) {
        if (widgetId.isEmpty() || !m_listWidget) {
            return;
        }

        LayoutEntry entry;
        entry.kind = EntryKind::Widget;
        entry.widgetId = widgetId;
        entry.instanceId = widgetId;
        entry.enabled = true;
        entry.expandHorizontally = defaultExpandHorizontally(widgetId);
        m_listWidget->addOrSelectEntry(entry);
    }

    bool BarLayoutSettings::defaultExpandHorizontally(const QString &widgetId) const {
        return layout::defaultExpandHorizontally(m_barId, widgetId);
    }

    void BarLayoutSettings::onPaletteEntryActivated(const LayoutEntry &entry) {
        if (entry.kind == EntryKind::Widget) {
            activateWidgetEntry(entry.widgetId);
        } else {
            appendPlaceholder(entry.kind);
        }
    }

    void BarLayoutSettings::onRemoveSelected() {
        const int row = m_listWidget ? m_listWidget->currentRow() : -1;
        if (row < 0) {
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

    void BarLayoutSettings::onPreviewEdited() {
        if (m_populating || !m_listWidget) {
            return;
        }

        for (int row = 0; row < m_listWidget->count(); ++row) {
            refreshPreviewItem(m_listWidget->item(row));
        }
        m_listWidget->doItemsLayout();
        m_listWidget->viewport()->update();

        persistToConfiguration();
        updateActions();
    }
}
