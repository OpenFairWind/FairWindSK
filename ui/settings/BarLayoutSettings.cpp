#include "BarLayoutSettings.hpp"

#include <QAbstractItemModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QMimeData>
#include <QScroller>
#include <QScrollerProperties>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "BarSettingsUi.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::settings {
    using fairwindsk::ui::layout::BarId;
    using fairwindsk::ui::layout::EntryKind;
    using fairwindsk::ui::layout::LayoutEntry;

    namespace {
        class PaletteAwareListWidget final : public QListWidget {
        public:
            explicit PaletteAwareListWidget(QWidget *parent = nullptr)
                : QListWidget(parent) {
                setViewMode(QListView::IconMode);
                setFlow(QListView::LeftToRight);
                setWrapping(false);
                setResizeMode(QListView::Adjust);
                setMovement(QListView::Snap);
                setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                setTextElideMode(Qt::ElideRight);
            }

        protected:
            void dragEnterEvent(QDragEnterEvent *event) override {
                if (event && (event->mimeData()->hasFormat(WidgetPalette::mimeType().toUtf8()) || event->source() == this)) {
                    event->acceptProposedAction();
                    return;
                }

                QListWidget::dragEnterEvent(event);
            }

            void dragMoveEvent(QDragMoveEvent *event) override {
                if (event && (event->mimeData()->hasFormat(WidgetPalette::mimeType().toUtf8()) || event->source() == this)) {
                    event->acceptProposedAction();
                    return;
                }

                QListWidget::dragMoveEvent(event);
            }

            void dropEvent(QDropEvent *event) override {
                if (!event || !event->mimeData()->hasFormat(WidgetPalette::mimeType().toUtf8())) {
                    QListWidget::dropEvent(event);
                    return;
                }

                LayoutEntry entry = WidgetPalette::decodeEntry(event->mimeData()->data(WidgetPalette::mimeType().toUtf8()));
                if (entry.kind != EntryKind::Widget && entry.instanceId.isEmpty()) {
                    entry.instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                }

                int insertRow = count();
                const QModelIndex targetIndex = indexAt(event->position().toPoint());
                if (targetIndex.isValid()) {
                    insertRow = targetIndex.row();
                }

                if (entry.kind == EntryKind::Widget) {
                    for (int row = 0; row < count(); ++row) {
                        auto *existingItem = item(row);
                        if (!existingItem || existingItem->data(Qt::UserRole + 2).toString() != entry.widgetId) {
                            continue;
                        }

                        QListWidgetItem *movedItem = takeItem(row);
                        if (row < insertRow) {
                            --insertRow;
                        }
                        insertItem(std::clamp(insertRow, 0, count()), movedItem);
                        setCurrentItem(movedItem);
                        event->setDropAction(Qt::MoveAction);
                        event->accept();
                        return;
                    }
                }

                auto *newItem = new QListWidgetItem(layout::entryLabel(entry));
                newItem->setData(Qt::UserRole + 1, static_cast<int>(entry.kind));
                newItem->setData(Qt::UserRole + 2, entry.widgetId);
                newItem->setData(Qt::UserRole + 3, entry.instanceId);
                newItem->setData(Qt::UserRole + 4, entry.expandHorizontally);
                newItem->setData(Qt::UserRole + 5, entry.expandVertically);
                newItem->setFlags(Qt::ItemIsEnabled |
                                  Qt::ItemIsSelectable |
                                  Qt::ItemIsDragEnabled);
                newItem->setIcon(WidgetPalette::entryIcon(entry));
                newItem->setSizeHint(QSize(90, 74));
                insertItem(std::clamp(insertRow, 0, count()), newItem);
                setCurrentItem(newItem);
                event->setDropAction(Qt::CopyAction);
                event->accept();
            }
        };
    }

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
        m_previewFrame->setMinimumHeight(86);
        m_previewFrame->setMaximumHeight(86);
        auto *previewLayout = new QVBoxLayout(m_previewFrame);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(0);

        m_minimumWidthButton = new QToolButton(this);
        m_maximumWidthButton = new QToolButton(this);
        m_expandHeightButton = new QToolButton(this);
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
        barsettings::configureHeightButton(m_expandHeightButton,
                                           tr("Extend Height"),
                                           kControlHeight);
        barsettings::configureActionButton(m_removeSelectedButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg"),
                                           tr("Remove Selected"),
                                           kControlHeight);
        barsettings::configureActionButton(m_resetDefaultsButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg"),
                                           tr("Reset Defaults"),
                                           kControlHeight);

        m_listWidget = new PaletteAwareListWidget(this);
        m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
        m_listWidget->setDragEnabled(true);
        m_listWidget->setAcceptDrops(true);
        m_listWidget->setDragDropOverwriteMode(false);
        m_listWidget->setDefaultDropAction(Qt::MoveAction);
        m_listWidget->setDropIndicatorShown(true);
        m_listWidget->setSpacing(6);
        m_listWidget->setUniformItemSizes(false);
        m_listWidget->setIconSize(QSize(32, 32));
        m_listWidget->setMinimumHeight(82);
        m_listWidget->setMaximumHeight(82);
        m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_listWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::LeftMouseButtonGesture);
        auto scrollerProperties = QScroller::scroller(m_listWidget->viewport())->scrollerProperties();
        scrollerProperties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
        scrollerProperties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.55);
        QScroller::scroller(m_listWidget->viewport())->setScrollerProperties(scrollerProperties);
        m_listWidget->setToolTip(tr("Tap a widget to edit it. Drag within the preview to reorder the bottom bar."));
        previewLayout->addWidget(m_listWidget);
        rootLayout->addWidget(m_previewFrame);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);
        controlsLayout->addWidget(m_minimumWidthButton);
        controlsLayout->addWidget(m_maximumWidthButton);
        controlsLayout->addWidget(m_expandHeightButton);
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
        connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
            if (!m_populating) {
                persistToConfiguration();
            }
        });
        connect(m_paletteWidget, &WidgetPalette::entryActivated, this, &BarLayoutSettings::onPaletteEntryActivated);
        connect(m_minimumWidthButton, &QToolButton::clicked, this, &BarLayoutSettings::onMinimumWidthSelected);
        connect(m_maximumWidthButton, &QToolButton::clicked, this, &BarLayoutSettings::onMaximumWidthSelected);
        connect(m_expandHeightButton, &QToolButton::toggled, this, &BarLayoutSettings::onExpandHeightToggled);
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
        barsettings::applyActionButtonChrome(m_expandHeightButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_removeSelectedButton, chrome, 52);
        barsettings::applyActionButtonChrome(m_resetDefaultsButton, chrome, 52);
    }

    QListWidgetItem *BarLayoutSettings::createItem(const LayoutEntry &entry) {
        auto *item = new QListWidgetItem(layout::entryLabel(entry));
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setData(RoleExpandHorizontally, entry.expandHorizontally);
        item->setData(RoleExpandVertically, entry.expandVertically);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(itemSizeHint(entry));
        refreshPreviewItem(item);
        return item;
    }

    void BarLayoutSettings::refreshPreviewItem(QListWidgetItem *item) const {
        if (!item) {
            return;
        }

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

        return QSize(width, entry.expandVertically ? 78 : 74);
    }

    LayoutEntry BarLayoutSettings::entryForItem(const QListWidgetItem *item) const {
        LayoutEntry entry;
        if (!item) {
            return entry;
        }

        entry.kind = static_cast<EntryKind>(item->data(RoleKind).toInt());
        entry.widgetId = item->data(RoleWidgetId).toString();
        entry.instanceId = item->data(RoleInstanceId).toString();
        entry.enabled = true;
        entry.expandHorizontally = item->data(RoleExpandHorizontally).toBool();
        entry.expandVertically = item->data(RoleExpandVertically).toBool();
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
        if (m_removeSelectedButton) {
            m_removeSelectedButton->setEnabled(m_listWidget && m_listWidget->currentItem());
        }
        const auto entry = entryForItem(m_listWidget ? m_listWidget->currentItem() : nullptr);
        const bool hasWidgetSelection = m_listWidget &&
                                        m_listWidget->currentItem() &&
                                        entry.kind == EntryKind::Widget;
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
        if (m_expandHeightButton) {
            const QSignalBlocker blocker(m_expandHeightButton);
            m_expandHeightButton->setEnabled(m_listWidget && m_listWidget->currentItem());
            m_expandHeightButton->setChecked(m_listWidget && m_listWidget->currentItem() && entry.expandVertically);
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
        entry.expandHorizontally = kind == EntryKind::Stretch;
        entry.expandVertically = false;

        m_listWidget->addItem(createItem(entry));
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onItemChanged(QListWidgetItem *item) {
        refreshPreviewItem(item);
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

        item->setData(RoleExpandHorizontally, false);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onMaximumWidthSelected() {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandHorizontally, true);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::onExpandHeightToggled(const bool checked) {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandVertically, checked);
        refreshPreviewItem(item);
        persistToConfiguration();
        updateActions();
    }

    void BarLayoutSettings::activateWidgetEntry(const QString &widgetId) {
        if (widgetId.isEmpty() || !m_listWidget) {
            return;
        }

        for (int row = 0; row < m_listWidget->count(); ++row) {
            auto *item = m_listWidget->item(row);
            if (!item || item->data(RoleWidgetId).toString() != widgetId) {
                continue;
            }

            m_listWidget->setCurrentItem(item);
            refreshPreviewItem(item);
            persistToConfiguration();
            updateActions();
            return;
        }

        LayoutEntry entry;
        entry.kind = EntryKind::Widget;
        entry.widgetId = widgetId;
        entry.instanceId = widgetId;
        entry.enabled = true;
        entry.expandHorizontally = defaultExpandHorizontally(widgetId);
        m_listWidget->addItem(createItem(entry));
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
        persistToConfiguration();
        updateActions();
    }

    bool BarLayoutSettings::defaultExpandHorizontally(const QString &widgetId) const {
        return layout::defaultExpandHorizontally(m_barId, widgetId);
    }

    void BarLayoutSettings::onPaletteEntryActivated(const LayoutEntry &entry) {
        if (entry.kind == EntryKind::Widget) {
            activateWidgetEntry(entry.widgetId);
            return;
        }

        appendPlaceholder(entry.kind);
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

        persistToConfiguration();
        updateActions();
    }
}
