#include "LayoutPreviewListWidget.hpp"

#include <algorithm>
#include <utility>

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QUuid>

#include "WidgetPalette.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr auto kItemModelMimeType = "application/x-qabstractitemmodeldatalist";
        constexpr auto kPreviewItemMimeType = "application/x-fairwindsk-layout-preview-entry";
        constexpr auto kPreviewItemRowMimeType = "application/x-fairwindsk-layout-preview-row";

        QString mimeTypeName(const char *mimeType) {
            return QString::fromLatin1(mimeType);
        }
    }

    LayoutPreviewListWidget::LayoutPreviewListWidget(QWidget *parent)
        : QListWidget(parent) {
        setViewMode(QListView::IconMode);
        setFlow(QListView::LeftToRight);
        setWrapping(false);
        setResizeMode(QListView::Adjust);
        setMovement(QListView::Snap);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDragDropOverwriteMode(false);
        setDefaultDropAction(Qt::MoveAction);
        setDragEnabled(true);
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setTextElideMode(Qt::ElideRight);
    }

    void LayoutPreviewListWidget::setItemFactory(ItemFactory factory) {
        m_itemFactory = std::move(factory);
    }

    void LayoutPreviewListWidget::setEditedCallback(EditedCallback callback) {
        m_editedCallback = std::move(callback);
    }

    bool LayoutPreviewListWidget::addOrSelectEntry(layout::LayoutEntry entry, const int insertRow) {
        entry.enabled = true;
        if (entry.kind == layout::EntryKind::Widget) {
            if (entry.instanceId.isEmpty()) {
                entry.instanceId = entry.widgetId;
            }

            if (auto *existingItem = findWidgetItem(entry.widgetId)) {
                const int oldRow = row(existingItem);
                int targetRow = insertRow < 0 ? oldRow : std::clamp(insertRow, 0, count());
                if (oldRow < targetRow) {
                    --targetRow;
                }
                targetRow = std::clamp(targetRow, 0, count() - 1);

                if (oldRow != targetRow) {
                    const QSignalBlocker blocker(model());
                    auto *movedItem = takeItem(oldRow);
                    insertItem(targetRow, movedItem);
                    setCurrentItem(movedItem);
                    notifyEdited();
                    return true;
                }

                setCurrentItem(existingItem);
                return false;
            }
        } else {
            entry.instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }

        const int targetRow = insertRow < 0 ? count() : std::clamp(insertRow, 0, count());
        auto *newItem = createItem(entry);
        {
            const QSignalBlocker blocker(model());
            insertItem(targetRow, newItem);
            setCurrentItem(newItem);
        }
        notifyEdited();
        return true;
    }

    bool LayoutPreviewListWidget::moveCurrentItemBy(const int offset) {
        if (offset == 0 || count() <= 1) {
            return false;
        }

        const int current = currentRow();
        if (current < 0) {
            return false;
        }

        const int target = std::clamp(current + offset, 0, count() - 1);
        if (current == target) {
            return false;
        }

        QListWidgetItem *item = nullptr;
        {
            const QSignalBlocker blocker(model());
            item = takeItem(current);
            insertItem(target, item);
            setCurrentItem(item);
        }
        notifyEdited();
        return true;
    }

    QListWidgetItem *LayoutPreviewListWidget::findWidgetItem(const QString &widgetId) const {
        if (widgetId.isEmpty()) {
            return nullptr;
        }

        for (int row = 0; row < count(); ++row) {
            auto *item = this->item(row);
            if (item && item->data(RoleWidgetId).toString() == widgetId) {
                return item;
            }
        }

        return nullptr;
    }

    layout::LayoutEntry LayoutPreviewListWidget::entryForItem(const QListWidgetItem *item) {
        layout::LayoutEntry entry;
        if (!item) {
            return entry;
        }

        entry.kind = static_cast<layout::EntryKind>(item->data(RoleKind).toInt());
        entry.widgetId = item->data(RoleWidgetId).toString();
        entry.instanceId = item->data(RoleInstanceId).toString();
        entry.enabled = true;
        entry.expandHorizontally = item->data(RoleExpandHorizontally).toBool();
        entry.expandVertically = item->data(RoleExpandVertically).toBool();
        return entry;
    }

    void LayoutPreviewListWidget::applyEntryData(QListWidgetItem *item, const layout::LayoutEntry &entry) {
        if (!item) {
            return;
        }

        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setData(RoleExpandHorizontally, entry.expandHorizontally);
        item->setData(RoleExpandVertically, entry.expandVertically);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled |
                       Qt::ItemIsDropEnabled);
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setTextAlignment(Qt::AlignCenter);
    }

    void LayoutPreviewListWidget::mousePressEvent(QMouseEvent *event) {
        m_draggedRow = event ? row(itemAt(event->position().toPoint())) : -1;
        QListWidget::mousePressEvent(event);
    }

    void LayoutPreviewListWidget::startDrag(Qt::DropActions supportedActions) {
        Q_UNUSED(supportedActions)

        auto *item = currentItem();
        if (!item) {
            return;
        }

        auto *mimeData = new QMimeData();
        mimeData->setData(mimeTypeName(kPreviewItemMimeType), WidgetPalette::encodeEntry(entryForItem(item)));
        mimeData->setData(mimeTypeName(kPreviewItemRowMimeType), QByteArray::number(row(item)));

        auto *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        if (!item->icon().isNull()) {
            drag->setPixmap(item->icon().pixmap(iconSize()));
        }
        drag->exec(Qt::MoveAction, Qt::MoveAction);
        m_draggedRow = -1;
    }

    void LayoutPreviewListWidget::dragEnterEvent(QDragEnterEvent *event) {
        if (event && event->mimeData()->hasFormat(mimeTypeName(kPreviewItemMimeType))) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        if (event && event->mimeData()->hasFormat(WidgetPalette::mimeType())) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
            return;
        }

        if (isInternalDrop(event)) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        QListWidget::dragEnterEvent(event);
    }

    void LayoutPreviewListWidget::dragMoveEvent(QDragMoveEvent *event) {
        if (event && event->mimeData()->hasFormat(mimeTypeName(kPreviewItemMimeType))) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        if (event && event->mimeData()->hasFormat(WidgetPalette::mimeType())) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
            return;
        }

        if (isInternalDrop(event)) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        QListWidget::dragMoveEvent(event);
    }

    void LayoutPreviewListWidget::dropEvent(QDropEvent *event) {
        if (event && event->mimeData()->hasFormat(mimeTypeName(kPreviewItemMimeType))) {
            const int oldRow = rowForPreviewDrag(event->mimeData());
            if (oldRow < 0 || oldRow >= count()) {
                m_draggedRow = -1;
                event->ignore();
                return;
            }

            moveRowToPosition(oldRow, event->position().toPoint());
            m_draggedRow = -1;
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        if (isInternalDrop(event)) {
            const int oldRow = m_draggedRow >= 0 ? m_draggedRow : currentRow();
            if (oldRow < 0 || oldRow >= count()) {
                m_draggedRow = -1;
                event->ignore();
                return;
            }

            moveRowToPosition(oldRow, event->position().toPoint());
            m_draggedRow = -1;
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        if (!event || !event->mimeData()->hasFormat(WidgetPalette::mimeType())) {
            QListWidget::dropEvent(event);
            return;
        }

        layout::LayoutEntry entry = WidgetPalette::decodeEntry(event->mimeData()->data(WidgetPalette::mimeType()));
        const int insertRow = insertRowForPosition(event->position().toPoint());
        const bool movingExistingWidget = entry.kind == layout::EntryKind::Widget && findWidgetItem(entry.widgetId);
        addOrSelectEntry(entry, insertRow);

        event->setDropAction(movingExistingWidget ? Qt::MoveAction : Qt::CopyAction);
        event->accept();
    }

    int LayoutPreviewListWidget::insertRowForPosition(const QPoint &position) const {
        if (count() <= 0) {
            return 0;
        }

        for (int row = 0; row < count(); ++row) {
            const QRect rect = visualItemRect(item(row));
            if (rect.isValid() && position.x() < rect.center().x()) {
                return row;
            }
        }

        int insertRow = count();
        const QModelIndex targetIndex = indexAt(position);
        if (targetIndex.isValid()) {
            insertRow = targetIndex.row();
            if (position.x() > visualRect(targetIndex).center().x()) {
                ++insertRow;
            }
        }

        return std::clamp(insertRow, 0, count());
    }

    int LayoutPreviewListWidget::rowForPreviewDrag(const QMimeData *mimeData) const {
        if (!mimeData) {
            return -1;
        }

        bool rowOk = false;
        const int encodedRow = QString::fromUtf8(mimeData->data(mimeTypeName(kPreviewItemRowMimeType))).toInt(&rowOk);
        if (rowOk && encodedRow >= 0 && encodedRow < count()) {
            return encodedRow;
        }

        const auto draggedEntry = WidgetPalette::decodeEntry(mimeData->data(mimeTypeName(kPreviewItemMimeType)));
        for (int row = 0; row < count(); ++row) {
            const auto itemEntry = entryForItem(item(row));
            if (!draggedEntry.instanceId.isEmpty() && itemEntry.instanceId == draggedEntry.instanceId) {
                return row;
            }
            if (draggedEntry.kind == layout::EntryKind::Widget &&
                itemEntry.kind == layout::EntryKind::Widget &&
                itemEntry.widgetId == draggedEntry.widgetId) {
                return row;
            }
        }

        return m_draggedRow >= 0 ? m_draggedRow : currentRow();
    }

    bool LayoutPreviewListWidget::moveRowToPosition(const int oldRow, const QPoint &position) {
        if (oldRow < 0 || oldRow >= count()) {
            return false;
        }

        int insertRow = insertRowForPosition(position);
        if (oldRow < insertRow) {
            --insertRow;
        }
        insertRow = std::clamp(insertRow, 0, count() - 1);

        if (oldRow == insertRow) {
            setCurrentRow(oldRow);
            return false;
        }

        const QSignalBlocker blocker(model());
        auto *movedItem = takeItem(oldRow);
        if (!movedItem) {
            return false;
        }

        insertItem(insertRow, movedItem);
        setCurrentItem(movedItem);
        notifyEdited();
        return true;
    }

    bool LayoutPreviewListWidget::isInternalDrop(const QDropEvent *event) const {
        return event &&
               !event->mimeData()->hasFormat(WidgetPalette::mimeType()) &&
               !event->mimeData()->hasFormat(mimeTypeName(kPreviewItemMimeType)) &&
               (event->source() == this ||
                event->source() == viewport() ||
                (m_draggedRow >= 0 && event->mimeData()->hasFormat(mimeTypeName(kItemModelMimeType))));
    }

    bool LayoutPreviewListWidget::isInternalDrop(const QDragMoveEvent *event) const {
        return event &&
               !event->mimeData()->hasFormat(WidgetPalette::mimeType()) &&
               !event->mimeData()->hasFormat(mimeTypeName(kPreviewItemMimeType)) &&
               (event->source() == this ||
                event->source() == viewport() ||
                (m_draggedRow >= 0 && event->mimeData()->hasFormat(mimeTypeName(kItemModelMimeType))));
    }

    QListWidgetItem *LayoutPreviewListWidget::createItem(const layout::LayoutEntry &entry) const {
        auto *item = m_itemFactory ? m_itemFactory(entry) : new QListWidgetItem(layout::entryLabel(entry));
        applyEntryData(item, entry);
        return item;
    }

    void LayoutPreviewListWidget::notifyEdited() const {
        if (m_editedCallback) {
            m_editedCallback();
        }
    }
}
