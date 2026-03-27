//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_APPS_HPP
#define FAIRWINDSK_APPS_HPP

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QMouseEvent>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>
#include <functional>
#include <nlohmann/json.hpp>

#include "Settings.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Apps; }
    QT_END_NAMESPACE

    class AvailableAppsListWidget final : public QListWidget {
        Q_OBJECT

    public:
        explicit AvailableAppsListWidget(QWidget *parent = nullptr);

    protected:
        void startDrag(Qt::DropActions supportedActions) override;
    };

    class LauncherPageGridWidget final : public QTableWidget {
        Q_OBJECT

    public:
        explicit LauncherPageGridWidget(QWidget *parent = nullptr);

        void setGridSize(int rows, int columns);
        void setAppResolver(std::function<QPair<QString, QPixmap>(const QString &)> resolver);
        void setPageItems(const QStringList &items);
        QStringList pageItems() const;
        void clearSelectedSlot();
        bool hasSelectedSlot() const;
        void assignSelectedSlot(const QString &appName);

    signals:
        void itemsChanged(const QStringList &items);
        void appDoubleClicked(const QString &appName);

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void startDrag(Qt::DropActions supportedActions) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mouseDoubleClickEvent(QMouseEvent *event) override;

    private:
        void setSlotApp(int row, int column, const QString &appName);
        QString slotApp(int row, int column) const;
        void emitItemsChanged();

    private:
        std::function<QPair<QString, QPixmap>(const QString &)> m_appResolver;
    };

    class Apps : public QWidget {
        Q_OBJECT

    public:
        explicit Apps(Settings *settings, QWidget *parent = nullptr);
        ~Apps() override;

    private slots:
        void onAvailableAppSelectionChanged();
        void onAvailableAppItemChanged(QListWidgetItem *listWidgetItem);
        void onAvailableAppDoubleClicked(QListWidgetItem *listWidgetItem);
        void onAppsEditSaveClicked();
        void onAppsDetailsFieldsTextChanged(const QString &text);
        void onAppsAppIconBrowse();
        void onAppsNameBrowse();
        void onAddAppClicked();
        void onRemoveAppClicked();
        void onAddPageClicked();
        void onAddFolderClicked();
        void onRemoveNodeClicked();
        void onMoveNodeUpClicked();
        void onMoveNodeDownClicked();
        void onPageTreeSelectionChanged();
        void onPageTreeItemChanged(QTreeWidgetItem *item, int column);
        void onPageGridItemsChanged(const QStringList &items);
        void onPageGridAppDoubleClicked(const QString &appName);
        void onClearPageSlotClicked();
        void onBackToLayoutClicked();

    private:
        bool eventFilter(QObject *object, QEvent *event) override;
        void setAppsEditMode(bool appsEditMode);
        void saveAppsDetails();
        QString uniqueAppName(const QString &baseName) const;
        void refreshAvailableAppActionButtons() const;
        void refreshPageTreeActionButtons() const;
        void rebuildAvailableAppsList();
        void rebuildPageTree();
        void rebuildPageEditor();
        void refreshDetailActionButtons() const;
        void showDetailsForApp(const QString &appName, bool startEditing = false);
        void showLayoutEditor();
        void syncAppActiveState(const QString &appName, bool active);
        void removeAppFromLauncherNodes(const QString &appName);
        void renameAppInLauncherNodes(const QString &oldName, const QString &newName);
        void markSettingsDirty();
        void ensureLauncherLayout();
        int launcherItemsPerPage() const;
        QString defaultNodeTitle(const nlohmann::json &node) const;
        QString selectedPageId() const;
        QTreeWidgetItem *addTreeNode(const nlohmann::json &node, QTreeWidgetItem *parentItem);
        nlohmann::json makePageNode(const QString &name = QString()) const;
        nlohmann::json makeFolderNode(const QString &name = QString()) const;
        nlohmann::json *launcherLayoutNodes();
        const nlohmann::json *launcherLayoutNodes() const;
        nlohmann::json *findNodeById(nlohmann::json &nodes, const QString &id) const;
        const nlohmann::json *findNodeById(const nlohmann::json &nodes, const QString &id) const;
        bool findNodeParent(nlohmann::json &nodes, const QString &id, nlohmann::json **parentChildren, int *index) const;
        QStringList pageItemsFromNode(const nlohmann::json &node) const;
        void setPageItemsForNode(nlohmann::json &node, const QStringList &items) const;
        QPair<QString, QPixmap> resolveAppPresentation(const QString &appName) const;
        nlohmann::json *selectedNode();
        const nlohmann::json *selectedNode() const;
        void selectTreeItemById(const QString &id);
        QString nextNodeId(const QString &prefix) const;

    private:
        Ui::Apps *ui = nullptr;
        Settings *m_settings = nullptr;
        bool m_appsEditMode = false;
        bool m_appsEditChanged = false;
        QString m_currentDetailAppName;
        QString m_selectedPageNodeId;
        AvailableAppsListWidget *m_availableAppsList = nullptr;
        QTreeWidget *m_pageTree = nullptr;
        LauncherPageGridWidget *m_pageGrid = nullptr;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_APPS_HPP
