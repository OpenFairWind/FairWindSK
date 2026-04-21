#include "TopBar.hpp"

#include <algorithm>
#include <QAbstractItemModel>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMimeData>
#include <QScroller>
#include <QScrollerProperties>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "ui/layout/BarLayout.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        using fairwindsk::ui::layout::BarId;
        using fairwindsk::ui::layout::EntryKind;
        using fairwindsk::ui::layout::LayoutEntry;

        constexpr auto kTopBarPaletteMimeType = "application/x-fairwindsk-topbar-entry";

        QByteArray mimePayloadForEntry(const LayoutEntry &entry) {
            QJsonObject payload;
            payload.insert(QStringLiteral("kind"), static_cast<int>(entry.kind));
            payload.insert(QStringLiteral("widgetId"), entry.widgetId);
            payload.insert(QStringLiteral("instanceId"), entry.instanceId);
            payload.insert(QStringLiteral("expandHorizontally"), entry.expandHorizontally);
            payload.insert(QStringLiteral("expandVertically"), entry.expandVertically);
            return QJsonDocument(payload).toJson(QJsonDocument::Compact);
        }

        LayoutEntry entryFromMimePayload(const QByteArray &payload) {
            LayoutEntry entry;
            const auto document = QJsonDocument::fromJson(payload);
            if (!document.isObject()) {
                return entry;
            }

            const auto object = document.object();
            entry.kind = static_cast<EntryKind>(object.value(QStringLiteral("kind")).toInt());
            entry.widgetId = object.value(QStringLiteral("widgetId")).toString();
            entry.instanceId = object.value(QStringLiteral("instanceId")).toString();
            entry.expandHorizontally = object.value(QStringLiteral("expandHorizontally")).toBool();
            entry.expandVertically = object.value(QStringLiteral("expandVertically")).toBool();
            return entry;
        }

        class PaletteListWidget final : public QListWidget {
        public:
            explicit PaletteListWidget(QWidget *parent = nullptr)
                : QListWidget(parent) {
                setViewMode(QListView::IconMode);
                setFlow(QListView::LeftToRight);
                setWrapping(false);
                setResizeMode(QListView::Adjust);
                setMovement(QListView::Static);
                setSelectionMode(QAbstractItemView::SingleSelection);
                setDragEnabled(true);
                setAcceptDrops(false);
                setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            }

        protected:
            void startDrag(Qt::DropActions supportedActions) override {
                Q_UNUSED(supportedActions)

                auto *item = currentItem();
                if (!item) {
                    return;
                }

                LayoutEntry entry;
                entry.kind = static_cast<EntryKind>(item->data(TopBar::RolePaletteKind).toInt());
                entry.widgetId = item->data(TopBar::RolePaletteWidgetId).toString();
                entry.instanceId = entry.kind == EntryKind::Widget
                                       ? entry.widgetId
                                       : QUuid::createUuid().toString(QUuid::WithoutBraces);
                entry.enabled = true;
                entry.expandHorizontally = entry.kind == EntryKind::Stretch;

                auto *mimeData = new QMimeData();
                mimeData->setData(kTopBarPaletteMimeType, mimePayloadForEntry(entry));

                auto *drag = new QDrag(this);
                drag->setMimeData(mimeData);
                drag->exec(Qt::CopyAction, Qt::CopyAction);
            }
        };

        class PreviewListWidget final : public QListWidget {
        public:
            explicit PreviewListWidget(QWidget *parent = nullptr)
                : QListWidget(parent) {
                setViewMode(QListView::IconMode);
                setFlow(QListView::LeftToRight);
                setWrapping(false);
                setResizeMode(QListView::Adjust);
                setMovement(QListView::Snap);
                setSelectionMode(QAbstractItemView::SingleSelection);
                setDragDropMode(QAbstractItemView::DragDrop);
                setDefaultDropAction(Qt::MoveAction);
                setDragEnabled(true);
                setAcceptDrops(true);
                setDropIndicatorShown(true);
                setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            }

        protected:
            void dragEnterEvent(QDragEnterEvent *event) override {
                if (event && (event->mimeData()->hasFormat(kTopBarPaletteMimeType) || event->source() == this)) {
                    event->acceptProposedAction();
                    return;
                }

                QListWidget::dragEnterEvent(event);
            }

            void dragMoveEvent(QDragMoveEvent *event) override {
                if (event && (event->mimeData()->hasFormat(kTopBarPaletteMimeType) || event->source() == this)) {
                    event->acceptProposedAction();
                    return;
                }

                QListWidget::dragMoveEvent(event);
            }

            void dropEvent(QDropEvent *event) override {
                if (!event || !event->mimeData()->hasFormat(kTopBarPaletteMimeType)) {
                    QListWidget::dropEvent(event);
                    return;
                }

                LayoutEntry entry = entryFromMimePayload(event->mimeData()->data(kTopBarPaletteMimeType));
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
                        if (!existingItem) {
                            continue;
                        }
                        if (existingItem->data(TopBar::RoleWidgetId).toString() != entry.widgetId) {
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

                auto *newItem = new QListWidgetItem();
                newItem->setData(TopBar::RoleKind, static_cast<int>(entry.kind));
                newItem->setData(TopBar::RoleWidgetId, entry.widgetId);
                newItem->setData(TopBar::RoleInstanceId, entry.instanceId);
                newItem->setData(TopBar::RoleExpandHorizontally, entry.expandHorizontally);
                newItem->setData(TopBar::RoleExpandVertically, entry.expandVertically);
                insertItem(std::clamp(insertRow, 0, count()), newItem);
                setCurrentItem(newItem);
                event->setDropAction(Qt::CopyAction);
                event->accept();
            }
        };
    }

    TopBar::TopBar(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
        buildUi();
        populateFromConfiguration();
    }

    void TopBar::refreshFromConfiguration() {
        populateFromConfiguration();
    }

    void TopBar::buildUi() {
        constexpr int kControlHeight = 52;

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        rootLayout->setSpacing(12);

        m_titleLabel = new QLabel(tr("Top Bar"), this);
        rootLayout->addWidget(m_titleLabel);

        m_hintLabel = new QLabel(
            tr("Drag items from the bottom palette onto the preview bar, then drag within the preview to reorder. Use Width and Height to decide which items should grow to fill available space."),
            this);
        m_hintLabel->setWordWrap(true);
        rootLayout->addWidget(m_hintLabel);

        m_previewLabel = new QLabel(tr("Preview"), this);
        rootLayout->addWidget(m_previewLabel);

        m_previewWidget = new PreviewListWidget(this);
        m_previewWidget->setMinimumHeight(124);
        m_previewWidget->setStyleSheet(QStringLiteral(
            "QListWidget {"
            " border: 1px solid palette(mid);"
            " border-radius: 12px;"
            " padding: 8px;"
            " background: palette(base);"
            " }"
            "QListWidget::item {"
            " margin: 4px;"
            " padding: 10px 12px;"
            " border: 1px solid palette(mid);"
            " border-radius: 10px;"
            " background: palette(alternate-base);"
            " }"
            "QListWidget::item:selected {"
            " border-color: palette(highlight);"
            " background: palette(light);"
            " font-weight: 600;"
            " }"));
        m_previewWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::LeftMouseButtonGesture);
        rootLayout->addWidget(m_previewWidget);

        auto *controlsLayout = new QGridLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setHorizontalSpacing(8);
        controlsLayout->setVerticalSpacing(8);

        m_expandWidthButton = new QPushButton(tr("Extend Width"), this);
        m_expandHeightButton = new QPushButton(tr("Extend Height"), this);
        m_removeSelectedButton = new QPushButton(tr("Remove Selected"), this);
        m_resetDefaultsButton = new QPushButton(tr("Reset Defaults"), this);
        for (auto *button : {m_expandWidthButton, m_expandHeightButton, m_removeSelectedButton, m_resetDefaultsButton}) {
            button->setMinimumHeight(kControlHeight);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        m_expandWidthButton->setCheckable(true);
        m_expandHeightButton->setCheckable(true);
        controlsLayout->addWidget(m_expandWidthButton, 0, 0);
        controlsLayout->addWidget(m_expandHeightButton, 0, 1);
        controlsLayout->addWidget(m_removeSelectedButton, 1, 0);
        controlsLayout->addWidget(m_resetDefaultsButton, 1, 1);
        rootLayout->addLayout(controlsLayout);

        m_paletteLabel = new QLabel(tr("Widget Palette"), this);
        rootLayout->addWidget(m_paletteLabel);

        m_paletteWidget = new PaletteListWidget(this);
        m_paletteWidget->setMinimumHeight(124);
        m_paletteWidget->setStyleSheet(QStringLiteral(
            "QListWidget {"
            " border: 1px solid palette(mid);"
            " border-radius: 12px;"
            " padding: 8px;"
            " background: palette(window);"
            " }"
            "QListWidget::item {"
            " margin: 4px;"
            " padding: 10px 12px;"
            " border: 1px dashed palette(mid);"
            " border-radius: 10px;"
            " }"
            "QListWidget::item:selected {"
            " border-color: palette(highlight);"
            " font-weight: 600;"
            " }"));
        m_paletteWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_paletteWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_paletteWidget->viewport(), QScroller::LeftMouseButtonGesture);
        rootLayout->addWidget(m_paletteWidget);

        LayoutEntry separatorEntry;
        separatorEntry.kind = EntryKind::Separator;
        separatorEntry.instanceId = QStringLiteral("separator");
        m_paletteWidget->addItem(createPaletteItem(separatorEntry));

        LayoutEntry stretchEntry;
        stretchEntry.kind = EntryKind::Stretch;
        stretchEntry.instanceId = QStringLiteral("stretch");
        stretchEntry.expandHorizontally = true;
        m_paletteWidget->addItem(createPaletteItem(stretchEntry));

        const auto definitions = layout::widgetDefinitions();
        for (const auto &definition : definitions) {
            LayoutEntry entry;
            entry.kind = EntryKind::Widget;
            entry.widgetId = definition.id;
            entry.instanceId = definition.id;
            m_paletteWidget->addItem(createPaletteItem(entry));
        }

        connect(m_previewWidget, &QListWidget::itemSelectionChanged, this, &TopBar::onPreviewSelectionChanged);
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsInserted, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsRemoved, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() { onPreviewEdited(); });
        connect(m_expandWidthButton, &QPushButton::toggled, this, &TopBar::onExpandWidthToggled);
        connect(m_expandHeightButton, &QPushButton::toggled, this, &TopBar::onExpandHeightToggled);
        connect(m_removeSelectedButton, &QPushButton::clicked, this, &TopBar::onRemoveSelected);
        connect(m_resetDefaultsButton, &QPushButton::clicked, this, &TopBar::onResetDefaults);

        updateInspector();
    }

    fairwindsk::ui::layout::LayoutEntry TopBar::entryForPreviewItem(const QListWidgetItem *item) const {
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

    QString TopBar::itemSummary(const LayoutEntry &entry) const {
        QStringList summary;
        if (entry.expandHorizontally) {
            summary.append(tr("Width"));
        }
        if (entry.expandVertically) {
            summary.append(tr("Height"));
        }

        if (entry.kind == EntryKind::Stretch && summary.isEmpty()) {
            summary.append(tr("Elastic"));
        }

        return summary.isEmpty() ? tr("Fixed") : summary.join(QStringLiteral(" + "));
    }

    QSize TopBar::itemSizeHint(const LayoutEntry &entry) const {
        int width = 120;
        if (entry.kind == EntryKind::Separator) {
            width = 36;
        } else if (entry.kind == EntryKind::Stretch) {
            width = entry.expandHorizontally ? 180 : 120;
        } else if (entry.expandHorizontally) {
            width = 180;
        }

        int height = entry.expandVertically ? 88 : 68;
        if (entry.kind == EntryKind::Separator && entry.expandVertically) {
            height = 96;
        }
        return QSize(width, height);
    }

    QListWidgetItem *TopBar::createPreviewItem(const LayoutEntry &entry) const {
        auto *item = new QListWidgetItem();
        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setData(RoleExpandHorizontally, entry.expandHorizontally);
        item->setData(RoleExpandVertically, entry.expandVertically);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled |
                       Qt::ItemIsDropEnabled);
        item->setText(QStringLiteral("%1\n%2")
                          .arg(layout::entryLabel(entry), itemSummary(entry)));
        item->setSizeHint(itemSizeHint(entry));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    }

    QListWidgetItem *TopBar::createPaletteItem(const LayoutEntry &entry) const {
        auto *item = new QListWidgetItem(QStringLiteral("%1\n%2")
                                             .arg(layout::entryLabel(entry),
                                                  entry.kind == EntryKind::Stretch ? tr("Drag to add") : tr("Available")));
        item->setData(RolePaletteKind, static_cast<int>(entry.kind));
        item->setData(RolePaletteWidgetId, entry.widgetId);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        item->setSizeHint(QSize(entry.kind == EntryKind::Separator ? 44 : 140, 68));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    }

    QListWidgetItem *TopBar::findPreviewWidgetItem(const QString &widgetId) const {
        if (widgetId.isEmpty() || !m_previewWidget) {
            return nullptr;
        }

        for (int row = 0; row < m_previewWidget->count(); ++row) {
            auto *item = m_previewWidget->item(row);
            if (item && item->data(RoleWidgetId).toString() == widgetId) {
                return item;
            }
        }

        return nullptr;
    }

    void TopBar::populateFromConfiguration() {
        if (!m_settings || !m_previewWidget) {
            return;
        }

        m_populating = true;
        const QSignalBlocker blocker(m_previewWidget);
        m_previewWidget->clear();

        const auto entries = layout::entriesForBar(m_settings->getConfiguration()->getRoot(), BarId::Top);
        for (const auto &entry : entries) {
            if (!entry.enabled) {
                continue;
            }
            m_previewWidget->addItem(createPreviewItem(entry));
        }

        m_populating = false;
        updatePaletteState();
        updateInspector();
    }

    void TopBar::persistToConfiguration() {
        if (m_populating || !m_settings) {
            return;
        }

        QList<LayoutEntry> entries;
        entries.reserve(m_previewWidget->count() + layout::widgetDefinitions().size());

        QStringList enabledWidgetIds;
        for (int row = 0; row < m_previewWidget->count(); ++row) {
            auto *item = m_previewWidget->item(row);
            if (!item) {
                continue;
            }
            const auto entry = entryForPreviewItem(item);
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
            entries.append(entry);
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        layout::setEntriesForBar(root, BarId::Top, entries);
        for (const auto &entry : entries) {
            if (entry.kind == EntryKind::Widget && entry.enabled) {
                layout::removeWidgetFromBar(root, BarId::Bottom, entry.widgetId);
            }
        }

        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
        updatePaletteState();
    }

    void TopBar::updateInspector() {
        const auto entry = entryForPreviewItem(m_previewWidget ? m_previewWidget->currentItem() : nullptr);
        const bool hasSelection = m_previewWidget && m_previewWidget->currentItem();

        if (m_expandWidthButton) {
            const QSignalBlocker blocker(m_expandWidthButton);
            m_expandWidthButton->setEnabled(hasSelection);
            m_expandWidthButton->setChecked(hasSelection && entry.expandHorizontally);
        }
        if (m_expandHeightButton) {
            const QSignalBlocker blocker(m_expandHeightButton);
            m_expandHeightButton->setEnabled(hasSelection);
            m_expandHeightButton->setChecked(hasSelection && entry.expandVertically);
        }
        if (m_removeSelectedButton) {
            m_removeSelectedButton->setEnabled(hasSelection);
        }
    }

    void TopBar::updatePaletteState() {
        if (!m_paletteWidget) {
            return;
        }

        for (int row = 0; row < m_paletteWidget->count(); ++row) {
            auto *item = m_paletteWidget->item(row);
            if (!item) {
                continue;
            }

            const auto kind = static_cast<EntryKind>(item->data(RolePaletteKind).toInt());
            if (kind != EntryKind::Widget) {
                continue;
            }

            const QString widgetId = item->data(RolePaletteWidgetId).toString();
            const bool inUse = findPreviewWidgetItem(widgetId) != nullptr;
            item->setText(QStringLiteral("%1\n%2")
                              .arg(layout::entryLabel(LayoutEntry{EntryKind::Widget, widgetId, widgetId, false, false, false}),
                                   inUse ? tr("Already on bar") : tr("Available")));
            item->setToolTip(inUse
                                 ? tr("Drag here again to move the existing item in the preview.")
                                 : tr("Drag onto the preview bar to add this item."));
        }
    }

    void TopBar::onPreviewSelectionChanged() {
        updateInspector();
    }

    void TopBar::onExpandWidthToggled(const bool checked) {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandHorizontally, checked);
        const auto entry = entryForPreviewItem(item);
        item->setText(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), itemSummary(entry)));
        item->setSizeHint(itemSizeHint(entry));
        persistToConfiguration();
    }

    void TopBar::onExpandHeightToggled(const bool checked) {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandVertically, checked);
        const auto entry = entryForPreviewItem(item);
        item->setText(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), itemSummary(entry)));
        item->setSizeHint(itemSizeHint(entry));
        persistToConfiguration();
    }

    void TopBar::onRemoveSelected() {
        if (!m_previewWidget) {
            return;
        }

        const int row = m_previewWidget->currentRow();
        if (row < 0) {
            return;
        }

        delete m_previewWidget->takeItem(row);
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::onResetDefaults() {
        if (!m_settings) {
            return;
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        const auto defaults = layout::defaultEntries(BarId::Top);
        layout::setEntriesForBar(root, BarId::Top, defaults);
        for (const auto &entry : defaults) {
            if (entry.kind == EntryKind::Widget && entry.enabled) {
                layout::removeWidgetFromBar(root, BarId::Bottom, entry.widgetId);
            }
        }
        populateFromConfiguration();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void TopBar::onPreviewEdited() {
        if (m_populating) {
            return;
        }

        for (int row = 0; row < m_previewWidget->count(); ++row) {
            auto *item = m_previewWidget->item(row);
            if (!item) {
                continue;
            }
            const auto entry = entryForPreviewItem(item);
            item->setText(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), itemSummary(entry)));
            item->setSizeHint(itemSizeHint(entry));
        }

        persistToConfiguration();
        updateInspector();
    }
}
