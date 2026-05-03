#ifndef FAIRWINDSK_UI_SETTINGS_LAYOUTPREVIEWLISTWIDGET_HPP
#define FAIRWINDSK_UI_SETTINGS_LAYOUTPREVIEWLISTWIDGET_HPP

#include <functional>

#include <QListWidget>

#include "ui/layout/BarLayout.hpp"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;
class QMouseEvent;

namespace fairwindsk::ui::settings {
    class LayoutPreviewListWidget final : public QListWidget {
    public:
        enum ItemRole {
            RoleKind = Qt::UserRole + 1,
            RoleWidgetId,
            RoleInstanceId,
            RoleExpandHorizontally,
            RoleExpandVertically
        };

        using ItemFactory = std::function<QListWidgetItem *(const fairwindsk::ui::layout::LayoutEntry &)>;
        using EditedCallback = std::function<void()>;

        explicit LayoutPreviewListWidget(QWidget *parent = nullptr);

        void setItemFactory(ItemFactory factory);
        void setEditedCallback(EditedCallback callback);
        bool addOrSelectEntry(fairwindsk::ui::layout::LayoutEntry entry, int insertRow = -1);
        bool moveCurrentItemBy(int offset);
        QListWidgetItem *findWidgetItem(const QString &widgetId) const;

        static fairwindsk::ui::layout::LayoutEntry entryForItem(const QListWidgetItem *item);
        static void applyEntryData(QListWidgetItem *item, const fairwindsk::ui::layout::LayoutEntry &entry);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void startDrag(Qt::DropActions supportedActions) override;
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;

    private:
        int insertRowForPosition(const QPoint &position) const;
        int rowForPreviewDrag(const QMimeData *mimeData) const;
        bool moveRowToPosition(int oldRow, const QPoint &position);
        bool isInternalDrop(const QDropEvent *event) const;
        bool isInternalDrop(const QDragMoveEvent *event) const;
        QListWidgetItem *createItem(const fairwindsk::ui::layout::LayoutEntry &entry) const;
        void notifyEdited() const;

        int m_draggedRow = -1;
        ItemFactory m_itemFactory;
        EditedCallback m_editedCallback;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_LAYOUTPREVIEWLISTWIDGET_HPP
