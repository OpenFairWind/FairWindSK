//
// Created by Raffaele Montella on 06/05/24.
//

#include "Apps.hpp"

#include <algorithm>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDrag>
#include <QFileDialog>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QSignalBlocker>
#include <QScroller>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "AppItem.hpp"
#include "FairWindSK.hpp"
#include "PList.hpp"
#include "AppDetailsWidget.hpp"
#include "PageDetailsWidget.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/TouchScrollArea.hpp"
#include "ui_AppDetailsWidget.h"
#include "ui_Apps.h"
#include "ui_PageDetailsWidget.h"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr auto kAppMimeType = "application/x-fairwind-app-name";
        constexpr auto kAppSourceRowMimeType = "application/x-fairwind-source-row";
        constexpr auto kAppSourceColumnMimeType = "application/x-fairwind-source-column";
        constexpr auto kLauncherLayoutKey = "launcherLayout";
        constexpr auto kLauncherNodesKey = "nodes";
        constexpr auto kLauncherNextNodeIdKey = "nextNodeId";
        constexpr auto kNodeTypeKey = "type";
        constexpr auto kNodeNameKey = "name";
        constexpr auto kNodeIdKey = "id";
        constexpr auto kNodeIconKey = "icon";
        constexpr auto kNodeItemsKey = "items";
        constexpr auto kNodeChildrenKey = "children";
        constexpr auto kFolderReferencePrefix = "@folder:";
        constexpr auto kParentReferenceToken = "@parent";
        constexpr const char *kNodeTypePage = "page";
        constexpr const char *kNodeTypeFolder = "folder";
        constexpr auto kDefaultPageIconPath = ":/resources/svg/OpenBridge/home.svg";
        constexpr auto kParentNavigationIconPath = ":/resources/svg/OpenBridge/arrow-left-google.svg";
        constexpr auto kTreeIdRole = Qt::UserRole;

        QString nodeType(const nlohmann::json &node) {
            if (node.contains(kNodeTypeKey) && node[kNodeTypeKey].is_string()) {
                return QString::fromStdString(node[kNodeTypeKey].get<std::string>());
            }
            return {};
        }

        QString nodeName(const nlohmann::json &node) {
            if (node.contains(kNodeNameKey) && node[kNodeNameKey].is_string()) {
                return QString::fromStdString(node[kNodeNameKey].get<std::string>());
            }
            return {};
        }

        QString nodeId(const nlohmann::json &node) {
            if (node.contains(kNodeIdKey) && node[kNodeIdKey].is_string()) {
                return QString::fromStdString(node[kNodeIdKey].get<std::string>());
            }
            return {};
        }

        QString nodeIconPath(const nlohmann::json &node) {
            if (node.contains(kNodeIconKey) && node[kNodeIconKey].is_string()) {
                return QString::fromStdString(node[kNodeIconKey].get<std::string>());
            }
            return QLatin1String(kDefaultPageIconPath);
        }

        QPixmap pageIconForPath(const QString &iconPath) {
            const QString trimmed = iconPath.trimmed();
            if (trimmed.isEmpty()) {
                return QPixmap(QLatin1String(kDefaultPageIconPath));
            }

            if (trimmed.startsWith(QStringLiteral("file://"))) {
                const QString localPath = trimmed.mid(QStringLiteral("file://").size());
                const QFileInfo localInfo(localPath);
                QPixmap pixmap(localPath);
                if (!pixmap.isNull()) {
                    return pixmap;
                }
                if (!localInfo.isAbsolute()) {
                    pixmap.load(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(localPath));
                    if (!pixmap.isNull()) {
                        return pixmap;
                    }
                }
            }

            QPixmap pixmap(trimmed);
            if (!pixmap.isNull()) {
                return pixmap;
            }

            return QPixmap(QLatin1String(kDefaultPageIconPath));
        }

        bool isPageNode(const nlohmann::json &node) {
            return nodeType(node) == QLatin1String(kNodeTypePage);
        }

        bool isFolderNode(const nlohmann::json &node) {
            return nodeType(node) == QLatin1String(kNodeTypeFolder);
        }

        bool isParentReferenceValue(const QString &value) {
            return value == QLatin1String(kParentReferenceToken);
        }

        bool isProtectedPageEntryValue(const QString &value) {
            return isParentReferenceValue(value);
        }

        void ensureJsonObject(nlohmann::json &jsonObject, const char *key) {
            if (!jsonObject.contains(key) || !jsonObject[key].is_object()) {
                jsonObject[key] = nlohmann::json::object();
            }
        }

        void ensureJsonArray(nlohmann::json &jsonObject, const char *key) {
            if (!jsonObject.contains(key) || !jsonObject[key].is_array()) {
                jsonObject[key] = nlohmann::json::array();
            }
        }

        QList<QString> collectActiveAppNames(const fairwindsk::Configuration &configuration) {
            QList<QString> appNames;
            auto &root = const_cast<fairwindsk::Configuration &>(configuration).getRoot();
            if (!root.contains("apps") || !root["apps"].is_array()) {
                return appNames;
            }

            QList<nlohmann::json> jsonApps;
            for (const auto &jsonApp : root["apps"]) {
                if (jsonApp.is_object()) {
                    jsonApps.append(jsonApp);
                }
            }

            std::sort(jsonApps.begin(), jsonApps.end(), [](const auto &left, const auto &right) {
                AppItem leftItem(left);
                AppItem rightItem(right);
                if (leftItem.getOrder() != rightItem.getOrder()) {
                    return leftItem.getOrder() < rightItem.getOrder();
                }
                return QString::compare(leftItem.getDisplayName(), rightItem.getDisplayName(), Qt::CaseInsensitive) < 0;
            });

            for (const auto &jsonApp : jsonApps) {
                AppItem appItem(jsonApp);
                if (appItem.getActive()) {
                    appNames.append(appItem.getName());
                }
            }

            return appNames;
        }

        QTreeWidgetItem *firstPageItem(QTreeWidget *treeWidget) {
            if (!treeWidget) {
                return nullptr;
            }

            QList<QTreeWidgetItem *> pending;
            for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
                pending.append(treeWidget->topLevelItem(i));
            }

            while (!pending.isEmpty()) {
                auto *item = pending.takeFirst();
                if (item && item->data(0, kTreeIdRole + 1).toString() == QLatin1String(kNodeTypePage)) {
                    return item;
                }
                if (!item) {
                    continue;
                }
                for (int i = 0; i < item->childCount(); ++i) {
                    pending.append(item->child(i));
                }
            }

            return nullptr;
        }

        class LauncherTileDelegate final : public QStyledItemDelegate {
        public:
            explicit LauncherTileDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

            void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override {
                if (!painter) {
                    return;
                }

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

                const QRect rect = option.rect.adjusted(3, 3, -3, -3);
                const bool isSelected = option.state & QStyle::State_Selected;

                QPainterPath clipPath;
                clipPath.addRoundedRect(rect, 3.0, 3.0);
                painter->setClipPath(clipPath);

                painter->fillRect(rect, QColor(16, 22, 32));

                const QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
                if (!pixmap.isNull()) {
                    const QPixmap scaled = pixmap.scaled(rect.size(),
                                                         Qt::KeepAspectRatioByExpanding,
                                                         Qt::SmoothTransformation);
                    const QRect sourceRect((scaled.width() - rect.width()) / 2,
                                           (scaled.height() - rect.height()) / 2,
                                           rect.width(),
                                           rect.height());
                    painter->drawPixmap(rect, scaled, sourceRect);
                }

                QLinearGradient overlay(rect.topLeft(), QPointF(rect.left(), rect.bottom()));
                overlay.setColorAt(0.0, QColor(0, 0, 0, 0));
                overlay.setColorAt(0.55, QColor(0, 0, 0, 10));
                overlay.setColorAt(1.0, QColor(0, 0, 0, 170));
                painter->fillRect(rect, overlay);

                painter->setClipping(false);
                painter->setPen(QPen(isSelected ? QColor(255, 255, 255) : QColor(230, 231, 235), isSelected ? 2.0 : 1.0));
                painter->drawRoundedRect(rect, 3.0, 3.0);

                QFont font = option.font;
                font.setPointSizeF(std::max<qreal>(10.0, font.pointSizeF()));
                painter->setFont(font);
                painter->setPen(QColor(248, 250, 252));
                painter->drawText(rect.adjusted(10, 10, -10, -10),
                                  Qt::AlignLeft | Qt::AlignBottom | Qt::TextWordWrap,
                                  index.data(Qt::DisplayRole).toString());

                painter->restore();
            }
        };
    }

    AvailableAppsListWidget::AvailableAppsListWidget(QWidget *parent) : QListWidget(parent) {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(false);
        setDragDropMode(QAbstractItemView::DragOnly);
        setDefaultDropAction(Qt::CopyAction);
        setAcceptDrops(false);
        setDropIndicatorShown(false);
        setUniformItemSizes(false);
        setResizeMode(QListView::Adjust);
        setLayoutMode(QListView::Batched);
        setMovement(QListView::Static);
        setWrapping(true);
        setWordWrap(false);
        setViewMode(QListView::IconMode);
        setFlow(QListView::LeftToRight);
        setGridSize(QSize(88, 96));
        setIconSize(QSize(52, 52));
        setSpacing(4);
        setAlternatingRowColors(false);
        setTextElideMode(Qt::ElideRight);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        QScroller::grabGesture(viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);
    }

    void AvailableAppsListWidget::mousePressEvent(QMouseEvent *event) {
        if (event && event->button() == Qt::LeftButton) {
            m_dragStartPosition = event->pos();
        }
        QListWidget::mousePressEvent(event);
    }

    void AvailableAppsListWidget::mouseMoveEvent(QMouseEvent *event) {
        if (!event || !(event->buttons() & Qt::LeftButton)) {
            QListWidget::mouseMoveEvent(event);
            return;
        }

        if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            QListWidget::mouseMoveEvent(event);
            return;
        }

        if (itemAt(m_dragStartPosition)) {
            startDrag(Qt::CopyAction);
            return;
        }

        QListWidget::mouseMoveEvent(event);
    }

    void AvailableAppsListWidget::startDrag(Qt::DropActions supportedActions) {
        auto *item = currentItem();
        if (!item) {
            return;
        }

        const QString appName = item->data(Qt::UserRole).toString();
        if (appName.isEmpty()) {
            return;
        }

        auto *mimeData = new QMimeData();
        mimeData->setData(kAppMimeType, appName.toUtf8());

        auto *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        const QPixmap pixmap = item->icon().pixmap(iconSize());
        if (!pixmap.isNull()) {
            drag->setPixmap(pixmap);
        }
        drag->exec(Qt::CopyAction, Qt::CopyAction);
        Q_UNUSED(supportedActions);
    }

    LauncherPageGridWidget::LauncherPageGridWidget(QWidget *parent) : QTableWidget(parent) {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setDragEnabled(true);
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDefaultDropAction(Qt::MoveAction);
        setIconSize(QSize(96, 96));
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        horizontalHeader()->setVisible(false);
        verticalHeader()->setVisible(false);
        setShowGrid(true);
        setItemDelegate(new LauncherTileDelegate(this));
    }

    void LauncherPageGridWidget::setGridSize(const int rows, const int columns) {
        const QSignalBlocker blocker(this);
        setRowCount(std::max(1, rows));
        setColumnCount(std::max(1, columns));
        for (int row = 0; row < rowCount(); ++row) {
            for (int column = 0; column < columnCount(); ++column) {
                if (!item(row, column)) {
                    auto *tableItem = new QTableWidgetItem();
                    tableItem->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
                    tableItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                    setItem(row, column, tableItem);
                }
            }
        }
    }

    void LauncherPageGridWidget::setAppResolver(std::function<QPair<QString, QPixmap>(const QString &)> resolver) {
        m_appResolver = std::move(resolver);
    }

    void LauncherPageGridWidget::setPageItems(const QStringList &items) {
        const QSignalBlocker blocker(this);
        int index = 0;
        for (int row = 0; row < rowCount(); ++row) {
            for (int column = 0; column < columnCount(); ++column) {
                setSlotApp(row, column, index < items.size() ? items.at(index) : QString());
                ++index;
            }
        }
    }

    QStringList LauncherPageGridWidget::pageItems() const {
        QStringList items;
        items.reserve(rowCount() * columnCount());
        for (int row = 0; row < rowCount(); ++row) {
            for (int column = 0; column < columnCount(); ++column) {
                items.append(slotApp(row, column));
            }
        }
        return items;
    }

    void LauncherPageGridWidget::clearSelectedSlot() {
        const auto selectedIndexes = selectionModel() ? selectionModel()->selectedIndexes() : QModelIndexList{};
        if (selectedIndexes.isEmpty()) {
            return;
        }
        const QModelIndex index = selectedIndexes.first();
        if (isProtectedPageEntryValue(slotApp(index.row(), index.column()))) {
            return;
        }
        setSlotApp(index.row(), index.column(), QString());
        emitItemsChanged();
    }

    bool LauncherPageGridWidget::hasSelectedSlot() const {
        return selectionModel() && !selectionModel()->selectedIndexes().isEmpty();
    }

    void LauncherPageGridWidget::assignSelectedSlot(const QString &appName) {
        const auto selectedIndexes = selectionModel() ? selectionModel()->selectedIndexes() : QModelIndexList{};
        if (selectedIndexes.isEmpty()) {
            return;
        }
        const QModelIndex index = selectedIndexes.first();
        setSlotApp(index.row(), index.column(), appName);
        emitItemsChanged();
    }

    void LauncherPageGridWidget::dragEnterEvent(QDragEnterEvent *event) {
        if (event && event->mimeData()->hasFormat(kAppMimeType)) {
            event->acceptProposedAction();
            return;
        }
        QTableWidget::dragEnterEvent(event);
    }

    void LauncherPageGridWidget::dragMoveEvent(QDragMoveEvent *event) {
        if (event && event->mimeData()->hasFormat(kAppMimeType)) {
            event->acceptProposedAction();
            return;
        }
        QTableWidget::dragMoveEvent(event);
    }

    void LauncherPageGridWidget::dropEvent(QDropEvent *event) {
        if (!event || !event->mimeData()->hasFormat(kAppMimeType)) {
            QTableWidget::dropEvent(event);
            return;
        }

        const QModelIndex targetIndex = indexAt(event->position().toPoint());
        if (!targetIndex.isValid()) {
            event->ignore();
            return;
        }

        const QString appName = QString::fromUtf8(event->mimeData()->data(kAppMimeType));
        if (appName.isEmpty()) {
            event->ignore();
            return;
        }

        const int sourceRow = event->mimeData()->hasFormat(kAppSourceRowMimeType)
                                  ? QString::fromUtf8(event->mimeData()->data(kAppSourceRowMimeType)).toInt()
                                  : -1;
        const int sourceColumn = event->mimeData()->hasFormat(kAppSourceColumnMimeType)
                                     ? QString::fromUtf8(event->mimeData()->data(kAppSourceColumnMimeType)).toInt()
                                     : -1;

        const QString targetEntry = slotApp(targetIndex.row(), targetIndex.column());
        if (isProtectedPageEntryValue(targetEntry) &&
            !(event->source() == this && sourceRow == targetIndex.row() && sourceColumn == targetIndex.column())) {
            event->ignore();
            return;
        }

        if (event->source() == this && sourceRow >= 0 && sourceColumn >= 0) {
            const QString sourceEntry = slotApp(sourceRow, sourceColumn);
            if (isProtectedPageEntryValue(sourceEntry)) {
                event->ignore();
                return;
            }
        }

        if (event->source() == this && sourceRow >= 0 && sourceColumn >= 0 &&
            !(sourceRow == targetIndex.row() && sourceColumn == targetIndex.column())) {
            setSlotApp(sourceRow, sourceColumn, QString());
        }

        setSlotApp(targetIndex.row(), targetIndex.column(), appName);
        emitItemsChanged();
        event->acceptProposedAction();
    }

    void LauncherPageGridWidget::startDrag(Qt::DropActions supportedActions) {
        auto *current = currentItem();
        if (!current) {
            return;
        }

        const QString appName = current->data(Qt::UserRole).toString();
        if (appName.isEmpty() || isProtectedPageEntryValue(appName)) {
            return;
        }

        auto *mimeData = new QMimeData();
        mimeData->setData(kAppMimeType, appName.toUtf8());
        mimeData->setData(kAppSourceRowMimeType, QByteArray::number(currentRow()));
        mimeData->setData(kAppSourceColumnMimeType, QByteArray::number(currentColumn()));

        auto *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(current->icon().pixmap(iconSize()));
        drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::MoveAction);
        Q_UNUSED(supportedActions);
    }

    void LauncherPageGridWidget::keyPressEvent(QKeyEvent *event) {
        if (event && (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)) {
            clearSelectedSlot();
            event->accept();
            return;
        }
        QTableWidget::keyPressEvent(event);
    }

    void LauncherPageGridWidget::mouseDoubleClickEvent(QMouseEvent *event) {
        QTableWidget::mouseDoubleClickEvent(event);
        auto *current = currentItem();
        if (!current) {
            return;
        }
        const QString appName = current->data(Qt::UserRole).toString();
        if (!appName.isEmpty()) {
            emit appDoubleClicked(appName);
        }
    }

    void LauncherPageGridWidget::setSlotApp(const int row, const int column, const QString &appName) {
        auto *tableItem = item(row, column);
        if (!tableItem) {
            tableItem = new QTableWidgetItem();
            tableItem->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
            tableItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            setItem(row, column, tableItem);
        }

        tableItem->setData(Qt::UserRole, appName);
        if (appName.isEmpty()) {
            tableItem->setText(QString());
            tableItem->setData(Qt::DecorationRole, QPixmap());
            tableItem->setToolTip(QString());
            return;
        }

        const auto presentation = m_appResolver ? m_appResolver(appName) : qMakePair(appName, QPixmap());
        tableItem->setText(presentation.first);
        tableItem->setData(Qt::DecorationRole, presentation.second);
        tableItem->setToolTip(presentation.first);
    }

    QString LauncherPageGridWidget::slotApp(const int row, const int column) const {
        const auto *tableItem = item(row, column);
        return tableItem ? tableItem->data(Qt::UserRole).toString() : QString();
    }

    void LauncherPageGridWidget::emitItemsChanged() {
        emit itemsChanged(pageItems());
    }

    Apps::Apps(Settings *settings, QWidget *parent) : QWidget(parent), ui(new Ui::Apps), m_settings(settings) {
        ui->setupUi(this);

        ui->splitter_Main->setStretchFactor(0, 1);
        ui->splitter_Main->setStretchFactor(1, 2);
        ui->verticalLayout_LeftPane->setStretch(0, 3);
        ui->verticalLayout_LeftPane->setStretch(1, 2);
        ui->label_PageTitle->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));
        ui->label_PageTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->label_PageIcon->setAlignment(Qt::AlignCenter);

        m_availableAppsList = new AvailableAppsListWidget(this);
        auto *availableAppsLayout = new QVBoxLayout(ui->widget_AvailableAppsHost);
        availableAppsLayout->setContentsMargins(0, 0, 0, 0);
        availableAppsLayout->addWidget(m_availableAppsList);

        m_pageTree = new QTreeWidget(this);
        m_pageTree->setColumnCount(1);
        m_pageTree->setHeaderHidden(true);
        m_pageTree->setEditTriggers(QAbstractItemView::EditKeyPressed);
        m_pageTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_pageTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_pageTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        QScroller::grabGesture(m_pageTree->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_pageTree->viewport(), QScroller::LeftMouseButtonGesture);
        auto *pageTreeLayout = new QVBoxLayout(ui->widget_PageTreeHost);
        pageTreeLayout->setContentsMargins(0, 0, 0, 0);
        pageTreeLayout->addWidget(m_pageTree);

        m_pageGrid = new LauncherPageGridWidget(this);
        m_pageGrid->setAppResolver([this](const QString &appName) { return resolveAppPresentation(appName); });
        auto *pageGridLayout = new QVBoxLayout(ui->widget_PageGridHost);
        pageGridLayout->setContentsMargins(0, 0, 0, 0);
        pageGridLayout->addWidget(m_pageGrid);

        m_appDetailsWidget = new AppDetailsWidget(this);
        auto *appDetailsScrollArea = new widgets::TouchScrollArea(this);
        appDetailsScrollArea->setBorderless(true);
        appDetailsScrollArea->setWidgetResizable(true);
        appDetailsScrollArea->setFrameShape(QFrame::NoFrame);
        appDetailsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        appDetailsScrollArea->setWidget(m_appDetailsWidget);
        auto *appDetailsLayout = new QVBoxLayout(ui->widget_AppDetailsHost);
        appDetailsLayout->setContentsMargins(0, 0, 0, 0);
        appDetailsLayout->addWidget(appDetailsScrollArea);

        m_pageDetailsWidget = new PageDetailsWidget(this);
        auto *pageDetailsScrollArea = new widgets::TouchScrollArea(this);
        pageDetailsScrollArea->setBorderless(true);
        pageDetailsScrollArea->setWidgetResizable(true);
        pageDetailsScrollArea->setFrameShape(QFrame::NoFrame);
        pageDetailsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageDetailsScrollArea->setWidget(m_pageDetailsWidget);
        auto *pageDetailsLayout = new QVBoxLayout(ui->widget_PageDetailsHost);
        pageDetailsLayout->setContentsMargins(0, 0, 0, 0);
        pageDetailsLayout->addWidget(pageDetailsScrollArea);
        m_appDetailsSaveTimer = new QTimer(this);
        m_appDetailsSaveTimer->setSingleShot(true);
        m_appDetailsSaveTimer->setInterval(300);
        m_pageDetailsSaveTimer = new QTimer(this);
        m_pageDetailsSaveTimer->setSingleShot(true);
        m_pageDetailsSaveTimer->setInterval(300);

        connect(m_availableAppsList, &QListWidget::itemSelectionChanged, this, &Apps::onAvailableAppSelectionChanged);
        connect(m_availableAppsList, &QListWidget::itemDoubleClicked, this, &Apps::onAvailableAppDoubleClicked);
        connect(m_appDetailsWidget->ui->lineEdit_Apps_Name, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(m_appDetailsWidget->ui->lineEdit_Apps_Description, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(m_appDetailsWidget->ui->lineEdit_Apps_DisplayName, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent,
                qOverload<double>(&fairwindsk::ui::widgets::TouchSpinBox::valueChanged),
                this,
                [this](double) { onAppsDetailsFieldsTextChanged(QString()); });
        m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent->setSingleStep(25.0);
        connect(m_appDetailsWidget->ui->pushButton_Apps_AppIcon_Browse, &QPushButton::clicked, this, &Apps::onAppsAppIconBrowse);
        connect(m_appDetailsWidget->ui->pushButton_Apps_Name_Browse, &QPushButton::clicked, this, &Apps::onAppsNameBrowse);
        connect(ui->toolButton_AddAllApps, &QToolButton::clicked, this, &Apps::onAddAllAppsClicked);
        connect(ui->toolButton_Add, &QToolButton::clicked, this, &Apps::onAddAppClicked);
        connect(ui->toolButton_Remove, &QToolButton::clicked, this, &Apps::onRemoveAppClicked);
        connect(ui->toolButton_EditApp, &QToolButton::clicked, this, [this]() {
            auto *item = m_availableAppsList->currentItem();
            if (item) {
                showDetailsForApp(item->data(Qt::UserRole).toString(), false);
            }
        });
        connect(ui->toolButton_AddPage, &QToolButton::clicked, this, &Apps::onAddPageClicked);
        connect(ui->toolButton_AddSelectedAppToPage, &QToolButton::clicked, this, &Apps::onAddSelectedAppToPageClicked);
        connect(ui->toolButton_ClearCurrentPageApps, &QToolButton::clicked, this, &Apps::onClearCurrentPageAppsClicked);
        connect(ui->toolButton_ClearAllPagesApps, &QToolButton::clicked, this, &Apps::onClearAllPagesAppsClicked);
        connect(ui->toolButton_RemoveNode, &QToolButton::clicked, this, &Apps::onRemoveNodeClicked);
        connect(ui->toolButton_MoveNodeLeft, &QToolButton::clicked, this, &Apps::onMoveNodeLeftClicked);
        connect(ui->toolButton_MoveNodeRight, &QToolButton::clicked, this, &Apps::onMoveNodeRightClicked);
        connect(ui->toolButton_MoveNodeUp, &QToolButton::clicked, this, &Apps::onMoveNodeUpClicked);
        connect(ui->toolButton_MoveNodeDown, &QToolButton::clicked, this, &Apps::onMoveNodeDownClicked);
        connect(m_pageTree, &QTreeWidget::itemSelectionChanged, this, &Apps::onPageTreeSelectionChanged);
        connect(m_pageTree, &QTreeWidget::itemDoubleClicked, this, &Apps::onPageTreeItemDoubleClicked);
        connect(m_pageTree, &QTreeWidget::itemChanged, this, &Apps::onPageTreeItemChanged);
        connect(m_pageGrid, &LauncherPageGridWidget::itemsChanged, this, &Apps::onPageGridItemsChanged);
        connect(m_pageGrid, &LauncherPageGridWidget::appDoubleClicked, this, &Apps::onPageGridAppDoubleClicked);
        connect(m_pageGrid, &QTableWidget::itemSelectionChanged, this, [this]() {
            refreshPageTreeActionButtons();
        });
        connect(ui->toolButton_PageDetails, &QToolButton::clicked, this, &Apps::onShowSelectedPageDetails);
        connect(ui->toolButton_SelectedSlotDetails, &QToolButton::clicked, this, &Apps::onShowSelectedGridItemDetails);
        connect(ui->toolButton_ClearPageSlot, &QToolButton::clicked, this, &Apps::onClearPageSlotClicked);
        connect(m_appDetailsWidget->ui->toolButton_BackToLayout, &QToolButton::clicked, this, &Apps::onBackToLayoutClicked);
        connect(m_appDetailsWidget, &AppDetailsWidget::iconPathSelected, this, [this](const QString &) {
            if (m_currentDetailAppName.isEmpty()) {
                return;
            }
            m_appsEditChanged = true;
            saveAppsDetails();
            showDetailsForApp(m_currentDetailAppName, false);
        });
        connect(m_appDetailsSaveTimer, &QTimer::timeout, this, [this]() {
            if (!m_appsEditChanged || m_currentDetailAppName.isEmpty()) {
                return;
            }
            saveAppsDetails();
        });
        connect(m_pageDetailsWidget->ui->toolButton_BackFromPageDetails, &QToolButton::clicked, this, &Apps::onBackToLayoutClicked);
        connect(m_pageDetailsWidget->ui->lineEdit_Page_Name, &QLineEdit::textChanged, this, &Apps::onPageDetailsFieldsTextChanged);
        connect(m_pageDetailsWidget, &PageDetailsWidget::iconPathSelected, this, [this](const QString &) {
            if (m_currentDetailPageId.isEmpty()) {
                return;
            }
            m_pageEditChanged = true;
            savePageDetails();
            showDetailsForPage(m_currentDetailPageId, false);
        });
        connect(m_pageDetailsSaveTimer, &QTimer::timeout, this, [this]() {
            if (!m_pageEditChanged || m_currentDetailPageId.isEmpty()) {
                return;
            }
            savePageDetails();
        });

        synchronizeAvailableApps(false);
        ensureLauncherLayout();
        normalizeLauncherLayout();
        rebuildAvailableAppsList();

        retintToolButtons();
        rebuildPageTree();
        showLayoutEditor();
        refreshAvailableAppActionButtons();
        refreshPageTreeActionButtons();
        refreshDetailActionButtons();
    }

    Apps::~Apps() {
        delete ui;
        ui = nullptr;
    }

    void Apps::refreshFromConfiguration() {
        synchronizeAvailableApps(false);
        ensureLauncherLayout();
        normalizeLauncherLayout();
        rebuildAvailableAppsList();
        rebuildPageTree();
        rebuildPageEditor();
        retintToolButtons();
    }

    void Apps::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
        }
    }

    void Apps::retintToolButtons() {
        const QColor buttonIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        for (auto *button : findChildren<QToolButton *>()) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(24, 24));
        }
    }

    bool Apps::eventFilter(QObject *object, QEvent *event) {
        Q_UNUSED(object);
        Q_UNUSED(event);
        return QWidget::eventFilter(object, event);
    }

    void Apps::setAppsEditMode(const bool editMode) {
        if (m_appsEditMode && m_appsEditChanged) {
            saveAppsDetails();
        }

        m_appsEditMode = editMode;
        m_appDetailsWidget->ui->lineEdit_Apps_Name->setReadOnly(false);
        m_appDetailsWidget->ui->lineEdit_Apps_Description->setReadOnly(false);
        m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->setReadOnly(false);
        m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent->setEnabled(!m_currentDetailAppName.isEmpty());
        m_appDetailsWidget->ui->pushButton_Apps_Name_Browse->setEnabled(!m_currentDetailAppName.isEmpty());
        m_appDetailsWidget->ui->pushButton_Apps_AppIcon_Browse->setEnabled(!m_currentDetailAppName.isEmpty());
        m_appsEditChanged = false;
        refreshDetailActionButtons();
    }

    void Apps::setPageEditMode(const bool editMode) {
        if (m_pageEditMode && m_pageEditChanged) {
            savePageDetails();
        }

        m_pageEditMode = editMode;
        m_pageDetailsWidget->ui->lineEdit_Page_Name->setReadOnly(false);
        m_pageDetailsWidget->ui->pushButton_Page_Icon_Browse->setEnabled(!m_currentDetailPageId.isEmpty());
        m_pageEditChanged = false;
        refreshDetailActionButtons();
    }

    void Apps::saveAppsDetails() {
        if (m_currentDetailAppName.isEmpty()) {
            return;
        }

        const int idx = m_settings->getConfiguration()->findApp(m_currentDetailAppName);
        if (idx == -1) {
            return;
        }

        auto appJsonObject = m_settings->getConfiguration()->getRoot()["apps"].at(idx);
        QString newName = m_appDetailsWidget->ui->lineEdit_Apps_Name->text().trimmed();
        if (newName.isEmpty()) {
            newName = m_currentDetailAppName;
        } else if (newName != m_currentDetailAppName && m_settings->getConfiguration()->findApp(newName) != -1) {
            drawer::warning(this, tr("Applications"), tr("An application named \"%1\" already exists.").arg(newName));
            m_appDetailsWidget->ui->lineEdit_Apps_Name->setText(m_currentDetailAppName);
            return;
        }

        appJsonObject["name"] = newName.toStdString();
        appJsonObject["description"] = m_appDetailsWidget->ui->lineEdit_Apps_Description->text().trimmed().toStdString();
        appJsonObject["displayName"] = m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->text().trimmed().toStdString();
        appJsonObject["signalk"]["appIcon"] = m_appDetailsWidget->appIconPath().toStdString();
        appJsonObject["fairwind"]["zoomPercent"] = m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent->value();
        m_settings->getConfiguration()->getRoot()["apps"].at(idx) = appJsonObject;

        if (newName != m_currentDetailAppName) {
            renameAppInLauncherNodes(m_currentDetailAppName, newName);
            m_currentDetailAppName = newName;
        }

        markSettingsDirty();
        rebuildAvailableAppsList();
        rebuildPageEditor();
        if (m_appDetailsSaveTimer) {
            m_appDetailsSaveTimer->stop();
        }
        m_appsEditChanged = false;
    }

    void Apps::savePageDetails() {
        if (m_currentDetailPageId.isEmpty()) {
            return;
        }

        auto *node = selectedNode();
        if (!node || nodeId(*node) != m_currentDetailPageId) {
            if (auto *nodes = launcherLayoutNodes()) {
                node = findNodeById(*nodes, m_currentDetailPageId);
            }
        }
        if (!node || !isPageNode(*node)) {
            return;
        }

        (*node)[kNodeNameKey] = m_pageDetailsWidget->pageName().toStdString();
        (*node)[kNodeIconKey] = m_pageDetailsWidget->pageIconPath().toStdString();
        m_pageEditChanged = false;
        if (m_pageDetailsSaveTimer) {
            m_pageDetailsSaveTimer->stop();
        }
        markSettingsDirty();
        rebuildPageTree();
        rebuildPageEditor();
        selectTreeItemById(m_currentDetailPageId);
    }

    QString Apps::uniqueAppName(const QString &baseName) const {
        QString candidate = baseName;
        int suffix = 1;
        while (m_settings->getConfiguration()->findApp(candidate) != -1) {
            candidate = QStringLiteral("%1_%2").arg(baseName).arg(suffix++);
        }
        return candidate;
    }

    void Apps::refreshAvailableAppActionButtons() const {
        const bool hasSelection = m_availableAppsList && m_availableAppsList->currentItem();
        ui->toolButton_Remove->setEnabled(hasSelection);
        ui->toolButton_EditApp->setEnabled(hasSelection);
        ui->toolButton_AddSelectedAppToPage->setEnabled(hasSelection && !selectedPageId().isEmpty());
        ui->toolButton_AddAllApps->setEnabled(!selectedPageId().isEmpty() && m_availableAppsList && m_availableAppsList->count() > 0);
    }

    void Apps::refreshPageTreeActionButtons() const {
        auto *item = m_pageTree ? m_pageTree->currentItem() : nullptr;
        const bool hasSelection = item != nullptr;
        const bool hasPageSelection = !selectedPageId().isEmpty();
        const bool hasCurrentPageApps = selectedNode() && pageHasRemovableApps(*selectedNode());
        const bool hasAnyPageApps = launcherLayoutNodes() && anyPageHasRemovableApps(*launcherLayoutNodes());
        ui->toolButton_RemoveNode->setEnabled(hasSelection);
        ui->toolButton_MoveNodeLeft->setEnabled(hasSelection);
        ui->toolButton_MoveNodeRight->setEnabled(hasSelection);
        ui->toolButton_MoveNodeUp->setEnabled(hasSelection);
        ui->toolButton_MoveNodeDown->setEnabled(hasSelection);
        ui->toolButton_PageDetails->setEnabled(hasSelection);
        ui->toolButton_AddSelectedAppToPage->setEnabled(hasSelection && m_availableAppsList && m_availableAppsList->currentItem());
        ui->toolButton_ClearPageSlot->setEnabled(m_pageGrid && m_pageGrid->hasSelectedSlot() && !selectedPageId().isEmpty());
        ui->toolButton_SelectedSlotDetails->setEnabled(!selectedGridEntry().isEmpty());
        ui->toolButton_AddAllApps->setEnabled(hasPageSelection && m_availableAppsList && m_availableAppsList->count() > 0);
        ui->toolButton_ClearCurrentPageApps->setEnabled(hasPageSelection && hasCurrentPageApps);
        ui->toolButton_ClearAllPagesApps->setEnabled(hasAnyPageApps);
    }

    void Apps::refreshDetailActionButtons() const {
        const bool hasDetail = !m_currentDetailAppName.isEmpty();
        m_appDetailsWidget->ui->lineEdit_Apps_Name->setEnabled(hasDetail);
        m_appDetailsWidget->ui->lineEdit_Apps_Description->setEnabled(hasDetail);
        m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->setEnabled(hasDetail);
        m_appDetailsWidget->ui->pushButton_Apps_Name_Browse->setEnabled(hasDetail);
        m_appDetailsWidget->ui->pushButton_Apps_AppIcon_Browse->setEnabled(hasDetail);
        const bool hasPageDetail = !m_currentDetailPageId.isEmpty();
        m_pageDetailsWidget->ui->lineEdit_Page_Name->setEnabled(hasPageDetail);
        m_pageDetailsWidget->ui->pushButton_Page_Icon_Browse->setEnabled(hasPageDetail);
        m_appDetailsWidget->ui->toolButton_BackToLayout->setEnabled(true);
        m_pageDetailsWidget->ui->toolButton_BackFromPageDetails->setEnabled(true);
    }

    void Apps::rebuildAvailableAppsList() {
        if (!m_availableAppsList) {
            return;
        }

        const QString currentAppName = m_availableAppsList->currentItem() ? m_availableAppsList->currentItem()->data(Qt::UserRole).toString()
                                                                          : m_currentDetailAppName;
        QSignalBlocker blocker(m_availableAppsList);
        m_availableAppsList->clear();

        const auto &root = m_settings->getConfiguration()->getRoot();
        if (!root.contains("apps") || !root["apps"].is_array()) {
            return;
        }

        QList<nlohmann::json> jsonApps;
        for (const auto &jsonApp : root["apps"]) {
            if (jsonApp.is_object()) {
                jsonApps.append(jsonApp);
            }
        }

        std::sort(jsonApps.begin(), jsonApps.end(), [](const auto &left, const auto &right) {
            AppItem leftItem(left);
            AppItem rightItem(right);
            if (leftItem.getOrder() != rightItem.getOrder()) {
                return leftItem.getOrder() < rightItem.getOrder();
            }
            return QString::compare(leftItem.getDisplayName(), rightItem.getDisplayName(), Qt::CaseInsensitive) < 0;
        });

        for (const auto &jsonApp : jsonApps) {
            AppItem appItem(jsonApp);
            auto *item = new QListWidgetItem(QIcon(appItem.getIcon()), appItem.getDisplayName());
            item->setData(Qt::UserRole, appItem.getName());
            item->setToolTip(appItem.getDescription());
            item->setFlags((item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled) & ~Qt::ItemIsUserCheckable);
            m_availableAppsList->addItem(item);
            if (appItem.getName() == currentAppName) {
                m_availableAppsList->setCurrentItem(item);
            }
        }

        if (!currentAppName.isEmpty() && !m_availableAppsList->currentItem()) {
            for (int i = 0; i < m_availableAppsList->count(); ++i) {
                auto *item = m_availableAppsList->item(i);
                if (item && item->data(Qt::UserRole).toString() == currentAppName) {
                    m_availableAppsList->setCurrentItem(item);
                    break;
                }
            }
        }
    }

    QTreeWidgetItem *Apps::addTreeNode(const nlohmann::json &node, QTreeWidgetItem *parentItem) {
        const QString type = nodeType(node);
        const QString title = defaultNodeTitle(node);
        auto *item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_pageTree);
        item->setText(0, title);
        item->setData(0, kTreeIdRole, nodeId(node));
        item->setData(0, kTreeIdRole + 1, type);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setIcon(0, QIcon(pageIconForPath(pageIconPath(node))));

        if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
            for (const auto &child : node[kNodeChildrenKey]) {
                if (child.is_object()) {
                    addTreeNode(child, item);
                }
            }
        }

        return item;
    }

    void Apps::rebuildPageTree() {
        if (!m_pageTree) {
            return;
        }

        ensureLauncherLayout();
        normalizeLauncherLayout();
        const QString preservedSelection = m_selectedPageNodeId;
        QSignalBlocker blocker(m_pageTree);
        m_pageTree->clear();

        if (const auto *nodes = launcherLayoutNodes()) {
            for (const auto &node : *nodes) {
                if (node.is_object()) {
                    addTreeNode(node, nullptr);
                }
            }
        }

        m_pageTree->expandAll();
        if (!preservedSelection.isEmpty()) {
            selectTreeItemById(preservedSelection);
        }
        if (!m_pageTree->currentItem()) {
            if (auto *firstPage = firstPageItem(m_pageTree)) {
                m_pageTree->setCurrentItem(firstPage);
            } else if (m_pageTree->topLevelItemCount() > 0) {
                m_pageTree->setCurrentItem(m_pageTree->topLevelItem(0));
            }
        }
        if (m_pageTree->currentItem()) {
            m_selectedPageNodeId = m_pageTree->currentItem()->data(0, kTreeIdRole).toString();
        }
    }

    void Apps::rebuildPageEditor() {
        const QString pageId = selectedPageId();
        const int rows = std::max(1, m_settings->getConfiguration()->getLauncherRows());
        const int columns = std::max(1, m_settings->getConfiguration()->getLauncherColumns());
        QStringList emptyItems;
        emptyItems.reserve(rows * columns);
        for (int i = 0; i < rows * columns; ++i) {
            emptyItems.append(QString());
        }
        m_pageGrid->setGridSize(rows, columns);

        if (pageId.isEmpty()) {
            ui->label_PageTitle->setText(tr("Select a page"));
            ui->label_PageIcon->setPixmap(QPixmap());
            m_pageGrid->setPageItems(emptyItems);
            refreshPageTreeActionButtons();
            return;
        }

        if (const auto *node = selectedNode()) {
            ui->label_PageTitle->setText(defaultNodeTitle(*node));
            QPixmap pixmap = pageIconPixmap(*node);
            if (!pixmap.isNull()) {
                pixmap = pixmap.scaled(ui->label_PageIcon->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
            }
            ui->label_PageIcon->setPixmap(pixmap);
            m_pageGrid->setPageItems(pageItemsFromNode(*node));
        } else {
            ui->label_PageTitle->setText(tr("Select a page"));
            ui->label_PageIcon->setPixmap(QPixmap());
            m_pageGrid->setPageItems(emptyItems);
        }

        refreshPageTreeActionButtons();
    }

    void Apps::showDetailsForApp(const QString &appName, const bool startEditing) {
        Q_UNUSED(startEditing);
        if (m_appsEditChanged && !m_currentDetailAppName.isEmpty() && m_currentDetailAppName != appName) {
            saveAppsDetails();
        }
        m_currentDetailPageId.clear();
        setPageEditMode(false);
        setAppsEditMode(false);
        m_currentDetailAppName = appName;
        const int idx = m_settings->getConfiguration()->findApp(appName);
        if (idx == -1) {
            return;
        }

        AppItem appItem(m_settings->getConfiguration()->getRoot()["apps"].at(idx));
        const QSignalBlocker nameBlocker(m_appDetailsWidget->ui->lineEdit_Apps_Name);
        const QSignalBlocker descriptionBlocker(m_appDetailsWidget->ui->lineEdit_Apps_Description);
        const QSignalBlocker displayNameBlocker(m_appDetailsWidget->ui->lineEdit_Apps_DisplayName);
        const QSignalBlocker zoomBlocker(m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent);
        m_appDetailsWidget->ui->label_Apps_Version_Text->setText(appItem.getVersion());
        m_appDetailsWidget->ui->label_Apps_Url_Text->setText(appItem.getUrl());
        m_appDetailsWidget->ui->label_Apps_Copyright_Text->setText(appItem.getCopyright());
        m_appDetailsWidget->ui->label_Apps_License_Text->setText(appItem.getLicense());
        m_appDetailsWidget->ui->label_Apps_Author_Text->setText(appItem.getAuthor());
        m_appDetailsWidget->ui->label_Apps_Vendor_Text->setText(appItem.getVendor());
        m_appDetailsWidget->ui->lineEdit_Apps_Name->setText(appItem.getName());
        m_appDetailsWidget->ui->lineEdit_Apps_Description->setText(appItem.getDescription());
        m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->setText(appItem.getDisplayName());
        m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent->setValue(appItem.getZoomPercent());
        m_appDetailsWidget->setAppIconPath(appItem.getAppIcon());

        QPixmap pixmap = appItem.getIcon();
        if (!pixmap.isNull()) {
            pixmap = pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_appDetailsWidget->ui->label_Apps_Icon->setPixmap(pixmap);

        ui->stackedWidget_RightPane->setCurrentWidget(ui->page_Details);
        m_appDetailsWidget->ui->lineEdit_Apps_Name->setEnabled(true);
        m_appDetailsWidget->ui->lineEdit_Apps_Name->setReadOnly(false);
        m_appDetailsWidget->ui->lineEdit_Apps_Description->setEnabled(true);
        m_appDetailsWidget->ui->lineEdit_Apps_Description->setReadOnly(false);
        m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->setEnabled(true);
        m_appDetailsWidget->ui->lineEdit_Apps_DisplayName->setReadOnly(false);
        m_appDetailsWidget->ui->spinBox_Apps_ZoomPercent->setEnabled(true);
        m_appDetailsWidget->ui->pushButton_Apps_Name_Browse->setEnabled(true);
        m_appDetailsWidget->ui->pushButton_Apps_AppIcon_Browse->setEnabled(true);
        m_appDetailsWidget->hideIconPicker();
        refreshDetailActionButtons();
    }

    void Apps::showDetailsForPage(const QString &pageId, const bool startEditing) {
        Q_UNUSED(startEditing);
        if (m_pageEditChanged && !m_currentDetailPageId.isEmpty() && m_currentDetailPageId != pageId) {
            savePageDetails();
        }

        m_currentDetailAppName.clear();
        setAppsEditMode(false);
        setPageEditMode(false);
        m_currentDetailPageId = pageId;

        const auto *node = selectedNode();
        if ((!node || nodeId(*node) != pageId) && launcherLayoutNodes()) {
            node = findNodeById(*launcherLayoutNodes(), pageId);
        }
        if (!node || !isPageNode(*node)) {
            return;
        }

        const QSignalBlocker blocker(m_pageDetailsWidget->ui->lineEdit_Page_Name);
        m_pageDetailsWidget->setPageName(nodeName(*node));
        const QString iconPath = pageIconPath(*node);
        m_pageDetailsWidget->setPageIconPath(iconPath);
        m_pageDetailsWidget->hideIconPicker();
        ui->stackedWidget_RightPane->setCurrentWidget(ui->page_PageDetails);
        m_pageDetailsWidget->ui->lineEdit_Page_Name->setEnabled(true);
        m_pageDetailsWidget->ui->lineEdit_Page_Name->setReadOnly(false);
        m_pageDetailsWidget->ui->pushButton_Page_Icon_Browse->setEnabled(true);
        refreshDetailActionButtons();
    }

    void Apps::showLayoutEditor() {
        if (m_appsEditChanged && !m_currentDetailAppName.isEmpty()) {
            saveAppsDetails();
        }
        if (m_pageEditChanged && !m_currentDetailPageId.isEmpty()) {
            savePageDetails();
        }
        setAppsEditMode(false);
        setPageEditMode(false);
        m_currentDetailAppName.clear();
        m_currentDetailPageId.clear();
        if (m_appDetailsSaveTimer) {
            m_appDetailsSaveTimer->stop();
        }
        if (m_pageDetailsSaveTimer) {
            m_pageDetailsSaveTimer->stop();
        }
        ui->stackedWidget_RightPane->setCurrentWidget(ui->page_Layout);
        rebuildPageEditor();
    }

    void Apps::removeAppFromLauncherNodes(const QString &appName) {
        std::function<void(nlohmann::json &)> removeFromNodes = [&](nlohmann::json &nodes) {
            if (!nodes.is_array()) {
                return;
            }
            for (auto &node : nodes) {
                if (!node.is_object()) {
                    continue;
                }
                if (isPageNode(node)) {
                    ensureJsonArray(node, kNodeItemsKey);
                    nlohmann::json filteredItems = nlohmann::json::array();
                    for (const auto &item : node[kNodeItemsKey]) {
                        if (item.is_string() && QString::fromStdString(item.get<std::string>()) == appName) {
                            continue;
                        }
                        filteredItems.push_back(item);
                    }
                    node[kNodeItemsKey] = filteredItems;
                } else if (isFolderNode(node)) {
                    ensureJsonArray(node, kNodeChildrenKey);
                    removeFromNodes(node[kNodeChildrenKey]);
                }
            }
        };

        if (auto *nodes = launcherLayoutNodes()) {
            removeFromNodes(*nodes);
        }
    }

    void Apps::renameAppInLauncherNodes(const QString &oldName, const QString &newName) {
        std::function<void(nlohmann::json &)> renameInNodes = [&](nlohmann::json &nodes) {
            if (!nodes.is_array()) {
                return;
            }
            for (auto &node : nodes) {
                if (!node.is_object()) {
                    continue;
                }
                if (isPageNode(node)) {
                    ensureJsonArray(node, kNodeItemsKey);
                    for (auto &item : node[kNodeItemsKey]) {
                        if (item.is_string() && QString::fromStdString(item.get<std::string>()) == oldName) {
                            item = newName.toStdString();
                        }
                    }
                } else if (isFolderNode(node)) {
                    ensureJsonArray(node, kNodeChildrenKey);
                    renameInNodes(node[kNodeChildrenKey]);
                }
            }
        };

        if (auto *nodes = launcherLayoutNodes()) {
            renameInNodes(*nodes);
        }
    }

    bool Apps::pageHasRemovableApps(const nlohmann::json &node) const {
        if (!isPageNode(node)) {
            return false;
        }

        for (const QString &item : pageItemsFromNode(node)) {
            if (!item.trimmed().isEmpty() && !isParentReference(item) && !isFolderReference(item)) {
                return true;
            }
        }

        return false;
    }

    bool Apps::anyPageHasRemovableApps(const nlohmann::json &nodes) const {
        if (!nodes.is_array()) {
            return false;
        }

        for (const auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }

            if (isPageNode(node) && pageHasRemovableApps(node)) {
                return true;
            }

            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array() &&
                anyPageHasRemovableApps(node[kNodeChildrenKey])) {
                return true;
            }
        }

        return false;
    }

    void Apps::clearRemovableAppsFromNode(nlohmann::json &node) const {
        if (!isPageNode(node)) {
            return;
        }

        QStringList items = pageItemsFromNode(node);
        for (QString &item : items) {
            if (!item.trimmed().isEmpty() && !isParentReference(item) && !isFolderReference(item)) {
                item.clear();
            }
        }
        setPageItemsForNode(node, items);
        normalizeParentReferenceForNode(node, parentPageIdForNodeId(nodeId(node)));
    }

    void Apps::clearRemovableAppsFromNodes(nlohmann::json &nodes) const {
        if (!nodes.is_array()) {
            return;
        }

        for (auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }

            if (isPageNode(node)) {
                clearRemovableAppsFromNode(node);
            }

            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                clearRemovableAppsFromNodes(node[kNodeChildrenKey]);
            }
        }
    }

    void Apps::markSettingsDirty() {
        if (m_settings) {
            m_settings->markDirty(FairWindSK::RuntimeApps, 0);
        }
    }

    bool Apps::promotePageNode(const QString &pageId, const bool selectPromotedPage) {
        if (pageId.isEmpty()) {
            return false;
        }

        auto *nodes = launcherLayoutNodes();
        if (!nodes) {
            return false;
        }

        nlohmann::json *parentChildren = nullptr;
        int index = -1;
        if (!findNodeParent(*nodes, pageId, &parentChildren, &index) || !parentChildren || index < 0) {
            return false;
        }

        std::function<bool(nlohmann::json &, nlohmann::json *, nlohmann::json **, int *)> findChildrenOwner =
            [&](nlohmann::json &candidateNodes, nlohmann::json *targetChildren, nlohmann::json **ownerChildren, int *ownerIndex) -> bool {
                if (!candidateNodes.is_array()) {
                    return false;
                }
                for (size_t i = 0; i < candidateNodes.size(); ++i) {
                    auto &candidate = candidateNodes[i];
                    if (!candidate.is_object()) {
                        continue;
                    }
                    if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array() &&
                        &candidate[kNodeChildrenKey] == targetChildren) {
                        *ownerChildren = &candidateNodes;
                        *ownerIndex = int(i);
                        return true;
                    }
                    if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array()) {
                        if (findChildrenOwner(candidate[kNodeChildrenKey], targetChildren, ownerChildren, ownerIndex)) {
                            return true;
                        }
                    }
                }
                return false;
            };

        nlohmann::json *grandParentChildren = nullptr;
        int parentIndex = -1;
        if (!findChildrenOwner(*nodes, parentChildren, &grandParentChildren, &parentIndex) ||
            !grandParentChildren || parentIndex < 0) {
            return false;
        }

        auto &parentPage = (*grandParentChildren)[parentIndex];
        if (isPageNode(parentPage)) {
            removeFolderReferenceFromNode(parentPage, pageId);
        }

        nlohmann::json nodeCopy = (*parentChildren)[index];
        parentChildren->erase(parentChildren->begin() + index);
        normalizeParentReferenceForNode(nodeCopy, QString());

        nlohmann::json *greatGrandParentChildren = nullptr;
        int grandParentIndex = -1;
        if (grandParentChildren == nodes) {
            greatGrandParentChildren = grandParentChildren;
            grandParentIndex = parentIndex;
        } else if (!findChildrenOwner(*nodes, grandParentChildren, &greatGrandParentChildren, &grandParentIndex) ||
                   !greatGrandParentChildren || grandParentIndex < 0) {
            return false;
        }

        greatGrandParentChildren->insert(greatGrandParentChildren->begin() + grandParentIndex + 1, nodeCopy);

        if (selectPromotedPage) {
            selectTreeItemById(pageId);
        }
        return true;
    }

    void Apps::ensureLauncherLayout() {
        auto &root = m_settings->getConfiguration()->getRoot();
        ensureJsonObject(root, kLauncherLayoutKey);
        auto &launcherLayout = root[kLauncherLayoutKey];
        ensureJsonArray(launcherLayout, kLauncherNodesKey);
        if (!launcherLayout.contains(kLauncherNextNodeIdKey) || !launcherLayout[kLauncherNextNodeIdKey].is_number_integer()) {
            launcherLayout[kLauncherNextNodeIdKey] = 1;
        }

        if (!launcherLayout[kLauncherNodesKey].empty()) {
            return;
        }

        const int itemsPerPage = launcherItemsPerPage();
        const auto activeAppNames = collectActiveAppNames(*m_settings->getConfiguration());
        if (activeAppNames.isEmpty()) {
            launcherLayout[kLauncherNodesKey].push_back(makePageNode(tr("Page 1")));
            return;
        }

        int pageNumber = 1;
        for (int i = 0; i < activeAppNames.size(); i += itemsPerPage) {
            auto pageNode = makePageNode(tr("Page %1").arg(pageNumber++));
            ensureJsonArray(pageNode, kNodeItemsKey);
            const int pageEnd = std::min(i + itemsPerPage, int(activeAppNames.size()));
            for (int itemIndex = i; itemIndex < pageEnd; ++itemIndex) {
                pageNode[kNodeItemsKey].push_back(activeAppNames.at(itemIndex).toStdString());
            }
            launcherLayout[kLauncherNodesKey].push_back(pageNode);
        }
    }

    bool Apps::synchronizeAvailableApps(const bool showErrors) {
        auto *fairWind = FairWindSK::getInstance();
        if (!fairWind) {
            return false;
        }

        auto localLauncherLayout = m_settings->getConfiguration()->getRoot().contains(kLauncherLayoutKey)
                                       ? m_settings->getConfiguration()->getRoot()[kLauncherLayoutKey]
                                       : nlohmann::json::object();
        const QString preservedSelection = m_selectedPageNodeId;
        const QString preservedDetail = m_currentDetailAppName;

        if (!fairWind->loadApps()) {
            if (showErrors) {
                drawer::warning(this, tr("Applications"), tr("Unable to synchronize applications from the server."));
            }
            return false;
        }

        auto &localRoot = m_settings->getConfiguration()->getRoot();
        localRoot["apps"] = fairWind->getConfiguration()->getRoot()["apps"];
        if (!localLauncherLayout.is_null() && !localLauncherLayout.empty()) {
            localRoot[kLauncherLayoutKey] = localLauncherLayout;
        }

        m_selectedPageNodeId = preservedSelection;
        m_currentDetailAppName = preservedDetail;
        ensureLauncherLayout();
        normalizeLauncherLayout();
        return true;
    }

    void Apps::appendOverflowPages(nlohmann::json &nodes, const int insertIndex, const QStringList &overflowItems, const QString &baseName) {
        if (!nodes.is_array() || overflowItems.isEmpty()) {
            return;
        }

        const int itemsPerPage = launcherItemsPerPage();
        int offset = 0;
        int pageNumber = 2;
        int currentInsertIndex = std::clamp(insertIndex, 0, int(nodes.size()));
        while (offset < overflowItems.size()) {
            auto pageNode = makePageNode(tr("%1 %2").arg(baseName, QString::number(pageNumber++)), QLatin1String(kDefaultPageIconPath));
            QStringList pageItems;
            pageItems.reserve(itemsPerPage);
            for (int i = 0; i < itemsPerPage; ++i) {
                if (offset < overflowItems.size()) {
                    pageItems.append(overflowItems.at(offset++));
                } else {
                    pageItems.append(QString());
                }
            }
            setPageItemsForNode(pageNode, pageItems);
            nodes.insert(nodes.begin() + currentInsertIndex, pageNode);
            ++currentInsertIndex;
        }
    }

    void Apps::normalizeLauncherNodes(nlohmann::json &nodes) {
        if (!nodes.is_array()) {
            return;
        }

        const int itemsPerPage = launcherItemsPerPage();
        for (size_t index = 0; index < nodes.size(); ++index) {
            auto &node = nodes[index];
            if (!node.is_object()) {
                continue;
            }

            if (isPageNode(node)) {
                QStringList items = pageItemsFromNode(node);
                const QString parentPageId = parentPageIdForNodeId(nodeId(node));
                items.removeAll(QString());
                if (!parentPageId.isEmpty()) {
                    items.removeAll(parentReferenceToken());
                    items.prepend(parentReferenceToken());
                } else {
                    items.removeAll(parentReferenceToken());
                }

                QStringList currentItems;
                QStringList overflowItems;
                currentItems.reserve(itemsPerPage);
                overflowItems.reserve(std::max(0, int(items.size()) - itemsPerPage));
                for (int itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
                    if (itemIndex < itemsPerPage) {
                        currentItems.append(items.at(itemIndex));
                    } else {
                        overflowItems.append(items.at(itemIndex));
                    }
                }
                while (currentItems.size() < itemsPerPage) {
                    currentItems.append(QString());
                }

                setPageItemsForNode(node, currentItems);
                normalizeParentReferenceForNode(node, parentPageId);

                if (!overflowItems.isEmpty()) {
                    appendOverflowPages(nodes, int(index) + 1, overflowItems, defaultNodeTitle(node));
                }
            }

            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                ensureJsonArray(node, kNodeChildrenKey);
                normalizeLauncherNodes(node[kNodeChildrenKey]);
            }
        }
    }

    void Apps::normalizeLauncherLayout() {
        auto *nodes = launcherLayoutNodes();
        if (!nodes) {
            return;
        }
        normalizeLauncherNodes(*nodes);
    }

    void Apps::collectPageIds(const nlohmann::json &nodes, QStringList &pageIds) const {
        if (!nodes.is_array()) {
            return;
        }
        for (const auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }
            if (isPageNode(node)) {
                pageIds.append(nodeId(node));
            }
            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                collectPageIds(node[kNodeChildrenKey], pageIds);
            }
        }
    }

    void Apps::fillPageChainWithApps(const QString &startPageId, const QStringList &appNames) {
        if (startPageId.isEmpty()) {
            return;
        }

        auto *nodes = launcherLayoutNodes();
        if (!nodes) {
            return;
        }

        nlohmann::json *parentChildren = nullptr;
        int pageIndex = -1;
        if (!findNodeParent(*nodes, startPageId, &parentChildren, &pageIndex) || !parentChildren || pageIndex < 0) {
            return;
        }

        const int itemsPerPage = launcherItemsPerPage();
        QStringList remainingApps = appNames;
        int currentIndex = pageIndex;
        QString currentPageId = startPageId;

        while (!remainingApps.isEmpty()) {
            auto *pageNode = findNodeById(*parentChildren, currentPageId);
            if (!pageNode || !isPageNode(*pageNode)) {
                break;
            }

            QStringList pageItems = pageItemsFromNode(*pageNode);
            const QString parentPageId = parentPageIdForNodeId(nodeId(*pageNode));
            int slotIndex = 0;
            if (!parentPageId.isEmpty() && !pageItems.isEmpty() && pageItems.first() == parentReferenceToken()) {
                slotIndex = 1;
            }

            for (; slotIndex < itemsPerPage && !remainingApps.isEmpty(); ++slotIndex) {
                pageItems[slotIndex] = remainingApps.takeFirst();
            }

            while (slotIndex < itemsPerPage) {
                pageItems[slotIndex++] = QString();
            }

            setPageItemsForNode(*pageNode, pageItems);
            normalizeParentReferenceForNode(*pageNode, parentPageId);

            if (remainingApps.isEmpty()) {
                break;
            }

            auto nextPageNode = makePageNode(tr("%1 %2").arg(defaultNodeTitle(*pageNode), QString::number(currentIndex - pageIndex + 2)),
                                             QLatin1String(kDefaultPageIconPath));
            const QString nextPageId = nodeId(nextPageNode);
            parentChildren->insert(parentChildren->begin() + currentIndex + 1, nextPageNode);
            ++currentIndex;
            currentPageId = nextPageId;
        }
    }

    int Apps::launcherItemsPerPage() const {
        const auto *configuration = m_settings ? m_settings->getConfiguration() : FairWindSK::getInstance()->getConfiguration();
        return std::max(1, configuration->getLauncherRows() * configuration->getLauncherColumns());
    }

    QString Apps::defaultNodeTitle(const nlohmann::json &node) const {
        const QString title = nodeName(node);
        if (!title.trimmed().isEmpty()) {
            return title;
        }
        return isFolderNode(node) ? tr("Folder") : tr("Page");
    }

    QString Apps::selectedPageId() const {
        const auto *node = selectedNode();
        if (node && isPageNode(*node)) {
            return nodeId(*node);
        }
        return {};
    }

    bool Apps::isFolderReference(const QString &value) const {
        return value.startsWith(QLatin1String(kFolderReferencePrefix));
    }

    bool Apps::isParentReference(const QString &value) const {
        return isParentReferenceValue(value);
    }

    QString Apps::folderReferenceForPageId(const QString &pageId) const {
        return QStringLiteral("%1%2").arg(QLatin1String(kFolderReferencePrefix), pageId);
    }

    QString Apps::pageIdFromFolderReference(const QString &value) const {
        return isFolderReference(value) ? value.mid(QLatin1String(kFolderReferencePrefix).size()) : QString();
    }

    QString Apps::parentReferenceToken() const {
        return QLatin1String(kParentReferenceToken);
    }

    int Apps::firstAvailableSlot(const QStringList &items) const {
        for (int i = 0; i < items.size(); ++i) {
            if (items.at(i).trimmed().isEmpty()) {
                return i;
            }
        }
        return -1;
    }

    QString Apps::requestedPageName(const QString &title, const QString &initialText) const {
        bool ok = false;
        const QString text = drawer::getText(const_cast<Apps *>(this),
                                             title,
                                             tr("Page name"),
                                             initialText,
                                             &ok);
        if (!ok) {
            return {};
        }
        return text.trimmed();
    }

    QString Apps::requestedPageIcon(const QString &pageName) const {
        Q_UNUSED(pageName);
        const QString iconPath = drawer::getIconPath(const_cast<Apps *>(this),
                                                     tr("Page icon"),
                                                     QLatin1String(kDefaultPageIconPath));
        if (iconPath.isEmpty()) {
            return QLatin1String(kDefaultPageIconPath);
        }
        return iconPath;
    }

    void Apps::removeFolderReferenceFromNode(nlohmann::json &node, const QString &pageId) const {
        if (!isPageNode(node)) {
            return;
        }
        QStringList items = pageItemsFromNode(node);
        const QString reference = folderReferenceForPageId(pageId);
        for (QString &item : items) {
            if (item == reference) {
                item.clear();
            }
        }
        setPageItemsForNode(node, items);
    }

    void Apps::normalizeParentReferenceForNode(nlohmann::json &node, const QString &parentPageId) const {
        if (!isPageNode(node)) {
            return;
        }

        QStringList items = pageItemsFromNode(node);
        items.removeAll(parentReferenceToken());

        if (!parentPageId.isEmpty()) {
            items.prepend(parentReferenceToken());
        }

        while (items.size() > launcherItemsPerPage()) {
            items.removeLast();
        }
        while (items.size() < launcherItemsPerPage()) {
            items.append(QString());
        }

        setPageItemsForNode(node, items);
    }

    nlohmann::json Apps::makePageNode(const QString &name, const QString &icon) const {
        nlohmann::json node = nlohmann::json::object();
        node[kNodeIdKey] = nextNodeId(QStringLiteral("page")).toStdString();
        node[kNodeTypeKey] = kNodeTypePage;
        node[kNodeNameKey] = name.toStdString();
        node[kNodeIconKey] = (icon.isEmpty() ? QLatin1String(kDefaultPageIconPath) : icon).toStdString();
        node[kNodeItemsKey] = nlohmann::json::array();
        return node;
    }

    nlohmann::json Apps::makeFolderNode(const QString &name) const {
        nlohmann::json node = nlohmann::json::object();
        node[kNodeIdKey] = nextNodeId(QStringLiteral("folder")).toStdString();
        node[kNodeTypeKey] = kNodeTypeFolder;
        node[kNodeNameKey] = name.toStdString();
        node[kNodeChildrenKey] = nlohmann::json::array();
        return node;
    }

    nlohmann::json *Apps::launcherLayoutNodes() {
        auto &root = m_settings->getConfiguration()->getRoot();
        ensureJsonObject(root, kLauncherLayoutKey);
        ensureJsonArray(root[kLauncherLayoutKey], kLauncherNodesKey);
        return &root[kLauncherLayoutKey][kLauncherNodesKey];
    }

    const nlohmann::json *Apps::launcherLayoutNodes() const {
        const auto &root = m_settings->getConfiguration()->getRoot();
        if (!root.contains(kLauncherLayoutKey) || !root[kLauncherLayoutKey].is_object()) {
            return nullptr;
        }
        const auto &launcherLayout = root[kLauncherLayoutKey];
        if (!launcherLayout.contains(kLauncherNodesKey) || !launcherLayout[kLauncherNodesKey].is_array()) {
            return nullptr;
        }
        return &launcherLayout[kLauncherNodesKey];
    }

    nlohmann::json *Apps::findNodeById(nlohmann::json &nodes, const QString &id) const {
        if (!nodes.is_array()) {
            return nullptr;
        }
        for (auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }
            if (nodeId(node) == id) {
                return &node;
            }
            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                ensureJsonArray(node, kNodeChildrenKey);
                if (auto *child = findNodeById(node[kNodeChildrenKey], id)) {
                    return child;
                }
            }
        }
        return nullptr;
    }

    const nlohmann::json *Apps::findNodeById(const nlohmann::json &nodes, const QString &id) const {
        if (!nodes.is_array()) {
            return nullptr;
        }
        for (const auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }
            if (nodeId(node) == id) {
                return &node;
            }
            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                if (const auto *child = findNodeById(node[kNodeChildrenKey], id)) {
                    return child;
                }
            }
        }
        return nullptr;
    }

    bool Apps::findNodeParent(nlohmann::json &nodes, const QString &id, nlohmann::json **parentChildren, int *index) const {
        if (!nodes.is_array()) {
            return false;
        }
        for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
            auto &node = nodes[nodeIndex];
            if (!node.is_object()) {
                continue;
            }
            if (nodeId(node) == id) {
                if (parentChildren) {
                    *parentChildren = &nodes;
                }
                if (index) {
                    *index = int(nodeIndex);
                }
                return true;
            }
            if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                ensureJsonArray(node, kNodeChildrenKey);
                if (findNodeParent(node[kNodeChildrenKey], id, parentChildren, index)) {
                    return true;
                }
            }
        }
        return false;
    }

    QString Apps::parentPageIdForNodeId(const QString &pageId) const {
        std::function<QString(const nlohmann::json &)> findParentId = [&](const nlohmann::json &nodes) -> QString {
            if (!nodes.is_array()) {
                return {};
            }

            for (const auto &node : nodes) {
                if (!node.is_object()) {
                    continue;
                }

                if (node.contains(kNodeChildrenKey) && node[kNodeChildrenKey].is_array()) {
                    for (const auto &child : node[kNodeChildrenKey]) {
                        if (child.is_object() && nodeId(child) == pageId && isPageNode(node)) {
                            return nodeId(node);
                        }
                    }

                    const QString nestedParentId = findParentId(node[kNodeChildrenKey]);
                    if (!nestedParentId.isEmpty()) {
                        return nestedParentId;
                    }
                }
            }

            return {};
        };

        const auto *nodes = launcherLayoutNodes();
        return nodes ? findParentId(*nodes) : QString();
    }

    QStringList Apps::pageItemsFromNode(const nlohmann::json &node) const {
        QStringList items;
        items.reserve(launcherItemsPerPage());
        for (int i = 0; i < launcherItemsPerPage(); ++i) {
            items.append(QString());
        }
        if (!isPageNode(node) || !node.contains(kNodeItemsKey) || !node[kNodeItemsKey].is_array()) {
            return items;
        }

        int index = 0;
        for (const auto &item : node[kNodeItemsKey]) {
            if (index >= items.size()) {
                break;
            }
            if (item.is_string()) {
                items[index] = QString::fromStdString(item.get<std::string>());
            }
            ++index;
        }
        return items;
    }

    void Apps::setPageItemsForNode(nlohmann::json &node, const QStringList &items) const {
        node[kNodeItemsKey] = nlohmann::json::array();
        for (const auto &item : items) {
            if (item.isEmpty()) {
                node[kNodeItemsKey].push_back("");
            } else {
                node[kNodeItemsKey].push_back(item.toStdString());
            }
        }
    }

    QString Apps::pageIconPath(const nlohmann::json &node) const {
        return nodeIconPath(node);
    }

    QPixmap Apps::pageIconPixmap(const nlohmann::json &node) const {
        return pageIconForPath(pageIconPath(node));
    }

    QPair<QString, QPixmap> Apps::resolveAppPresentation(const QString &appName) const {
        if (isParentReference(appName)) {
            const QString parentPageId = parentPageIdForNodeId(selectedPageId());
            if (const auto *nodes = launcherLayoutNodes()) {
                if (const auto *parentNode = findNodeById(*nodes, parentPageId)) {
                    const QString parentTitle = defaultNodeTitle(*parentNode).trimmed();
                    return qMakePair(parentTitle.isEmpty() ? tr("Parent page") : parentTitle, pageIconPixmap(*parentNode));
                }
            }
            return qMakePair(tr("Parent page"), QPixmap(QLatin1String(kParentNavigationIconPath)));
        }

        if (isFolderReference(appName)) {
            const QString pageId = pageIdFromFolderReference(appName);
            if (const auto *nodes = launcherLayoutNodes()) {
                if (const auto *node = findNodeById(*nodes, pageId)) {
                    return qMakePair(defaultNodeTitle(*node), pageIconPixmap(*node));
                }
            }
            return qMakePair(tr("Folder"), QPixmap(QLatin1String(kDefaultPageIconPath)));
        }

        const int idx = m_settings->getConfiguration()->findApp(appName);
        if (idx == -1) {
            return qMakePair(appName, QPixmap::fromImage(QImage(":/resources/images/icons/webapp-256x256.png")));
        }

        AppItem appItem(m_settings->getConfiguration()->getRoot()["apps"].at(idx));
        return qMakePair(appItem.getDisplayName(), appItem.getIcon());
    }

    QString Apps::selectedGridEntry() const {
        if (!m_pageGrid || !m_pageGrid->currentItem()) {
            return {};
        }
        return m_pageGrid->currentItem()->data(Qt::UserRole).toString();
    }

    nlohmann::json *Apps::selectedNode() {
        if (m_selectedPageNodeId.isEmpty()) {
            return nullptr;
        }
        auto *nodes = launcherLayoutNodes();
        return nodes ? findNodeById(*nodes, m_selectedPageNodeId) : nullptr;
    }

    const nlohmann::json *Apps::selectedNode() const {
        if (m_selectedPageNodeId.isEmpty()) {
            return nullptr;
        }
        const auto *nodes = launcherLayoutNodes();
        return nodes ? findNodeById(*nodes, m_selectedPageNodeId) : nullptr;
    }

    void Apps::selectTreeItemById(const QString &id) {
        if (!m_pageTree || id.isEmpty()) {
            return;
        }

        QList<QTreeWidgetItem *> pending;
        for (int i = 0; i < m_pageTree->topLevelItemCount(); ++i) {
            pending.append(m_pageTree->topLevelItem(i));
        }

        while (!pending.isEmpty()) {
            auto *item = pending.takeFirst();
            if (!item) {
                continue;
            }
            if (item->data(0, kTreeIdRole).toString() == id) {
                m_pageTree->setCurrentItem(item);
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                pending.append(item->child(i));
            }
        }
    }

    QString Apps::nextNodeId(const QString &prefix) const {
        auto &root = m_settings->getConfiguration()->getRoot();
        ensureJsonObject(root, kLauncherLayoutKey);
        auto &launcherLayout = root[kLauncherLayoutKey];
        if (!launcherLayout.contains(kLauncherNextNodeIdKey) || !launcherLayout[kLauncherNextNodeIdKey].is_number_integer()) {
            launcherLayout[kLauncherNextNodeIdKey] = 1;
        }
        const int nextId = launcherLayout[kLauncherNextNodeIdKey].get<int>();
        launcherLayout[kLauncherNextNodeIdKey] = nextId + 1;
        return QStringLiteral("%1-%2").arg(prefix).arg(nextId);
    }

    void Apps::onAvailableAppSelectionChanged() {
        refreshAvailableAppActionButtons();
    }

    void Apps::onAvailableAppDoubleClicked(QListWidgetItem *listWidgetItem) {
        if (!listWidgetItem) {
            return;
        }
        showDetailsForApp(listWidgetItem->data(Qt::UserRole).toString());
    }

    void Apps::onAppsDetailsFieldsTextChanged(const QString &text) {
        Q_UNUSED(text);
        if (!m_currentDetailAppName.isEmpty()) {
            m_appsEditChanged = true;
            if (m_appDetailsSaveTimer) {
                m_appDetailsSaveTimer->start();
            }
        }
    }

    void Apps::onAppsAppIconBrowse() {
        if (m_currentDetailAppName.isEmpty()) {
            return;
        }

        const QString iconPath = drawer::getIconPath(this,
                                                     tr("Application icon"),
                                                     m_appDetailsWidget ? m_appDetailsWidget->appIconPath() : QString());
        if (iconPath.isEmpty() || !m_appDetailsWidget) {
            return;
        }

        m_appDetailsWidget->setAppIconPath(iconPath);
        m_appsEditChanged = true;
        saveAppsDetails();
        showDetailsForApp(m_currentDetailAppName, false);
    }

    void Apps::onAppsNameBrowse() {
        if (m_currentDetailAppName.isEmpty()) {
            return;
        }

        const QString selectedPath = drawer::getOpenFilePath(this,
                                                             tr("Select file"),
                                                             QDir::homePath(),
                                                             tr("All files (*.*)"));
        if (selectedPath.isEmpty()) {
            return;
        }

        m_appDetailsWidget->ui->lineEdit_Apps_Name->setText(QFileInfo(selectedPath).completeBaseName());
    }

    void Apps::onAddAppClicked() {
        AppItem appItem;
        appItem.setName(uniqueAppName(QStringLiteral("new_app")));
        appItem.setDisplayName(tr("New application"));
        appItem.setDescription(tr("Describe this application"));
        appItem.setOrder(m_availableAppsList->count() + 1);
        appItem.setActive(false);

        m_settings->getConfiguration()->getRoot()["apps"].push_back(appItem.asJson());
        markSettingsDirty();
        rebuildAvailableAppsList();

        for (int i = 0; i < m_availableAppsList->count(); ++i) {
            auto *item = m_availableAppsList->item(i);
            if (item && item->data(Qt::UserRole).toString() == appItem.getName()) {
                m_availableAppsList->setCurrentItem(item);
                break;
            }
        }

        showDetailsForApp(appItem.getName(), true);
    }

    void Apps::onAddAllAppsClicked() {
        ensureLauncherLayout();
        normalizeLauncherLayout();

        const QString pageId = selectedPageId();
        if (pageId.isEmpty()) {
            drawer::warning(this, tr("Applications"), tr("Select a target page first."));
            return;
        }

        QStringList appNames;
        const auto &root = m_settings->getConfiguration()->getRoot();
        if (root.contains("apps") && root["apps"].is_array()) {
            QList<nlohmann::json> jsonApps;
            for (const auto &jsonApp : root["apps"]) {
                if (jsonApp.is_object()) {
                    jsonApps.append(jsonApp);
                }
            }

            std::sort(jsonApps.begin(), jsonApps.end(), [](const auto &left, const auto &right) {
                AppItem leftItem(left);
                AppItem rightItem(right);
                if (leftItem.getOrder() != rightItem.getOrder()) {
                    return leftItem.getOrder() < rightItem.getOrder();
                }
                return QString::compare(leftItem.getDisplayName(), rightItem.getDisplayName(), Qt::CaseInsensitive) < 0;
            });

            for (const auto &jsonApp : jsonApps) {
                AppItem appItem(jsonApp);
                appNames.append(appItem.getName());
            }
        }

        fillPageChainWithApps(pageId, appNames);
        markSettingsDirty();
        rebuildPageTree();
        selectTreeItemById(pageId);
        rebuildPageEditor();
    }

    void Apps::onRemoveAppClicked() {
        auto *item = m_availableAppsList->currentItem();
        if (!item) {
            return;
        }

        const QString appName = item->data(Qt::UserRole).toString();
        if (drawer::question(this,
                             tr("Remove application"),
                             tr("Remove \"%1\" from the available applications list?").arg(item->text()),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        const int idx = m_settings->getConfiguration()->findApp(appName);
        if (idx != -1) {
            m_settings->getConfiguration()->getRoot()["apps"].erase(idx);
        }
        removeAppFromLauncherNodes(appName);
        markSettingsDirty();
        m_currentDetailAppName.clear();
        rebuildAvailableAppsList();
        rebuildPageEditor();
        showLayoutEditor();
    }

    void Apps::onAddPageClicked() {
        ensureLauncherLayout();
        auto pageNode = makePageNode(tr("Page"), QLatin1String(kDefaultPageIconPath));
        nlohmann::json *parentChildren = launcherLayoutNodes();
        int index = parentChildren ? int(parentChildren->size()) : 0;

        if (auto *currentNode = selectedNode()) {
            if (isFolderNode(*currentNode)) {
                ensureJsonArray(*currentNode, kNodeChildrenKey);
                parentChildren = &(*currentNode)[kNodeChildrenKey];
                index = int(parentChildren->size());
            } else if (findNodeParent(*launcherLayoutNodes(), nodeId(*currentNode), &parentChildren, &index)) {
                ++index;
            }
        }

        if (!parentChildren) {
            return;
        }

        parentChildren->insert(parentChildren->begin() + index, pageNode);
        m_selectedPageNodeId = nodeId(pageNode);
        markSettingsDirty();
        rebuildPageTree();
        rebuildPageEditor();
        selectTreeItemById(m_selectedPageNodeId);
        showDetailsForPage(m_selectedPageNodeId, true);
    }

    void Apps::onAddSelectedAppToPageClicked() {
        const QString pageId = selectedPageId();
        if (pageId.isEmpty()) {
            drawer::warning(this, tr("Applications"), tr("Select a target page first."));
            return;
        }

        auto *currentItem = m_availableAppsList ? m_availableAppsList->currentItem() : nullptr;
        if (!currentItem) {
            drawer::warning(this, tr("Applications"), tr("Select an application first."));
            return;
        }

        const QString appName = currentItem->data(Qt::UserRole).toString().trimmed();
        if (appName.isEmpty()) {
            return;
        }

        ensureLauncherLayout();
        normalizeLauncherLayout();

        if (auto *pageNode = findNodeById(*launcherLayoutNodes(), pageId); pageNode && isPageNode(*pageNode)) {
            QStringList items = pageItemsFromNode(*pageNode);
            const int slotIndex = firstAvailableSlot(items);
            if (slotIndex != -1) {
                items[slotIndex] = appName;
                setPageItemsForNode(*pageNode, items);
                normalizeParentReferenceForNode(*pageNode, parentPageIdForNodeId(pageId));
            } else {
                fillPageChainWithApps(pageId, QStringList{appName});
            }
        }

        markSettingsDirty();
        rebuildPageTree();
        selectTreeItemById(pageId);
        rebuildPageEditor();
        refreshPageTreeActionButtons();
        refreshAvailableAppActionButtons();
    }

    void Apps::onClearCurrentPageAppsClicked() {
        const QString pageId = selectedPageId();
        if (pageId.isEmpty()) {
            drawer::warning(this, tr("Applications"), tr("Select a target page first."));
            return;
        }

        auto *nodes = launcherLayoutNodes();
        auto *pageNode = nodes ? findNodeById(*nodes, pageId) : nullptr;
        if (!pageNode || !isPageNode(*pageNode) || !pageHasRemovableApps(*pageNode)) {
            return;
        }

        if (drawer::question(this,
                             tr("Remove applications from current page"),
                             tr("Remove all applications from the current page?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        clearRemovableAppsFromNode(*pageNode);
        markSettingsDirty();
        rebuildPageTree();
        selectTreeItemById(pageId);
        rebuildPageEditor();
        refreshPageTreeActionButtons();
    }

    void Apps::onClearAllPagesAppsClicked() {
        auto *nodes = launcherLayoutNodes();
        if (!nodes || !anyPageHasRemovableApps(*nodes)) {
            return;
        }

        if (drawer::question(this,
                             tr("Remove applications from all pages"),
                             tr("Remove all applications from every launcher page?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        const QString preservedPageId = selectedPageId();
        clearRemovableAppsFromNodes(*nodes);
        markSettingsDirty();
        rebuildPageTree();
        if (!preservedPageId.isEmpty()) {
            selectTreeItemById(preservedPageId);
        }
        rebuildPageEditor();
        refreshPageTreeActionButtons();
    }

    void Apps::onRemoveNodeClicked() {
        auto *currentNode = selectedNode();
        if (!currentNode) {
            return;
        }

        if (drawer::question(this,
                             tr("Remove launcher node"),
                             tr("Remove the selected page or folder from the launcher layout?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        nlohmann::json *parentChildren = nullptr;
        int index = -1;
        if (findNodeParent(*launcherLayoutNodes(), nodeId(*currentNode), &parentChildren, &index) && parentChildren && index >= 0) {
            std::function<bool(nlohmann::json &, nlohmann::json *, nlohmann::json **, int *)> findChildrenOwner =
                [&](nlohmann::json &nodes, nlohmann::json *targetChildren, nlohmann::json **ownerChildren, int *ownerIndex) -> bool {
                    if (!nodes.is_array()) {
                        return false;
                    }
                    for (size_t i = 0; i < nodes.size(); ++i) {
                        auto &candidate = nodes[i];
                        if (!candidate.is_object()) {
                            continue;
                        }
                        if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array() &&
                            &candidate[kNodeChildrenKey] == parentChildren) {
                            *ownerChildren = &nodes;
                            *ownerIndex = int(i);
                            return true;
                        }
                        if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array()) {
                            if (findChildrenOwner(candidate[kNodeChildrenKey], targetChildren, ownerChildren, ownerIndex)) {
                                return true;
                            }
                        }
                    }
                    return false;
                };
            nlohmann::json *ownerChildren = nullptr;
            int ownerIndex = -1;
            if (findChildrenOwner(*launcherLayoutNodes(), parentChildren, &ownerChildren, &ownerIndex) &&
                ownerChildren && ownerIndex >= 0) {
                removeFolderReferenceFromNode((*ownerChildren)[ownerIndex], nodeId(*currentNode));
            }
            parentChildren->erase(parentChildren->begin() + index);
        }

        if (launcherLayoutNodes()->empty()) {
            launcherLayoutNodes()->push_back(makePageNode(tr("Page 1")));
        }

        m_selectedPageNodeId.clear();
        markSettingsDirty();
        rebuildPageTree();
        rebuildPageEditor();
    }

    void Apps::onMoveNodeLeftClicked() {
        auto *currentNode = selectedNode();
        if (!currentNode) {
            return;
        }

        const QString currentNodeId = nodeId(*currentNode);
        if (!promotePageNode(currentNodeId, false)) {
            return;
        }
        markSettingsDirty();
        rebuildPageTree();
        rebuildPageEditor();
        selectTreeItemById(currentNodeId);
    }

    void Apps::onMoveNodeRightClicked() {
        auto *currentNode = selectedNode();
        if (!currentNode) {
            return;
        }

        const QString currentNodeId = nodeId(*currentNode);
        nlohmann::json *parentChildren = nullptr;
        int index = -1;
        if (!findNodeParent(*launcherLayoutNodes(), currentNodeId, &parentChildren, &index) ||
            !parentChildren || index <= 0 || index >= int(parentChildren->size())) {
            return;
        }

        std::function<bool(nlohmann::json &, nlohmann::json *, nlohmann::json **, int *)> findChildrenOwner =
            [&](nlohmann::json &nodes, nlohmann::json *targetChildren, nlohmann::json **ownerChildren, int *ownerIndex) -> bool {
                if (!nodes.is_array()) {
                    return false;
                }
                for (size_t i = 0; i < nodes.size(); ++i) {
                    auto &candidate = nodes[i];
                    if (!candidate.is_object()) {
                        continue;
                    }
                    if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array() &&
                        &candidate[kNodeChildrenKey] == targetChildren) {
                        *ownerChildren = &nodes;
                        *ownerIndex = int(i);
                        return true;
                    }
                    if (candidate.contains(kNodeChildrenKey) && candidate[kNodeChildrenKey].is_array()) {
                        if (findChildrenOwner(candidate[kNodeChildrenKey], targetChildren, ownerChildren, ownerIndex)) {
                            return true;
                        }
                    }
                }
                return false;
            };

        nlohmann::json *currentOwnerChildren = nullptr;
        int currentOwnerIndex = -1;
        if (findChildrenOwner(*launcherLayoutNodes(), parentChildren, &currentOwnerChildren, &currentOwnerIndex) &&
            currentOwnerChildren && currentOwnerIndex >= 0) {
            removeFolderReferenceFromNode((*currentOwnerChildren)[currentOwnerIndex], currentNodeId);
        }

        auto &previousSibling = (*parentChildren)[index - 1];
        if (!isPageNode(previousSibling)) {
            drawer::warning(this, tr("Pages"), tr("A page can only be nested under another page."));
            return;
        }
        const QString parentPageId = nodeId(previousSibling);

        QStringList parentItems = pageItemsFromNode(previousSibling);
        const int availableIndex = firstAvailableSlot(parentItems);
        if (availableIndex < 0) {
            drawer::warning(this, tr("Pages"), tr("No available slot exists in the parent page for the folder icon."));
            return;
        }

        parentItems[availableIndex] = folderReferenceForPageId(currentNodeId);
        setPageItemsForNode(previousSibling, parentItems);
        ensureJsonArray(previousSibling, kNodeChildrenKey);

        nlohmann::json nodeCopy = (*parentChildren)[index];
        parentChildren->erase(parentChildren->begin() + index);
        normalizeParentReferenceForNode(nodeCopy, parentPageId);
        previousSibling[kNodeChildrenKey].push_back(nodeCopy);

        markSettingsDirty();
        rebuildPageTree();
        rebuildPageEditor();
        selectTreeItemById(parentPageId);
        showLayoutEditor();
    }

    void Apps::onMoveNodeUpClicked() {
        auto *currentNode = selectedNode();
        if (!currentNode) {
            return;
        }

        const QString currentNodeId = nodeId(*currentNode);
        nlohmann::json *parentChildren = nullptr;
        int index = -1;
        if (!findNodeParent(*launcherLayoutNodes(), currentNodeId, &parentChildren, &index) || !parentChildren || index < 0) {
            return;
        }
        if (index <= 0) {
            return;
        }
        std::iter_swap(parentChildren->begin() + index, parentChildren->begin() + index - 1);
        markSettingsDirty();
        rebuildPageTree();
        selectTreeItemById(currentNodeId);
    }

    void Apps::onMoveNodeDownClicked() {
        auto *currentNode = selectedNode();
        if (!currentNode) {
            return;
        }

        const QString currentNodeId = nodeId(*currentNode);
        nlohmann::json *parentChildren = nullptr;
        int index = -1;
        if (!findNodeParent(*launcherLayoutNodes(), currentNodeId, &parentChildren, &index) ||
            !parentChildren || index < 0 || index >= int(parentChildren->size()) - 1) {
            return;
        }
        std::iter_swap(parentChildren->begin() + index, parentChildren->begin() + index + 1);
        markSettingsDirty();
        rebuildPageTree();
        selectTreeItemById(currentNodeId);
    }

    void Apps::onPageTreeSelectionChanged() {
        auto *item = m_pageTree->currentItem();
        m_selectedPageNodeId = item ? item->data(0, kTreeIdRole).toString() : QString();
        if (ui->stackedWidget_RightPane->currentWidget() == ui->page_PageDetails && !m_selectedPageNodeId.isEmpty()) {
            showDetailsForPage(m_selectedPageNodeId, false);
        } else {
            rebuildPageEditor();
        }
    }

    void Apps::onPageTreeItemDoubleClicked(QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        if (!item) {
            return;
        }
        const QString id = item->data(0, kTreeIdRole).toString();
        if (!id.isEmpty()) {
            showDetailsForPage(id, false);
        }
    }

    void Apps::onPageTreeItemChanged(QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        if (!item) {
            return;
        }

        const QString id = item->data(0, kTreeIdRole).toString();
        if (id.isEmpty()) {
            return;
        }

        if (auto *node = findNodeById(*launcherLayoutNodes(), id)) {
            (*node)[kNodeNameKey] = item->text(0).toStdString();
            markSettingsDirty();
            if (ui->stackedWidget_RightPane->currentWidget() == ui->page_PageDetails && id == m_currentDetailPageId) {
                showDetailsForPage(id, m_pageEditMode);
            } else {
                rebuildPageEditor();
            }
        }
    }

    void Apps::onPageGridItemsChanged(const QStringList &items) {
        auto *node = selectedNode();
        if (!node || !isPageNode(*node)) {
            return;
        }
        const QString currentPageId = nodeId(*node);
        const QStringList previousItems = pageItemsFromNode(*node);
        setPageItemsForNode(*node, items);
        normalizeParentReferenceForNode(*node, parentPageIdForNodeId(nodeId(*node)));

        QStringList removedFolderPageIds;
        for (const QString &previousItem : previousItems) {
            if (!isFolderReference(previousItem)) {
                continue;
            }
            if (!items.contains(previousItem)) {
                removedFolderPageIds.append(pageIdFromFolderReference(previousItem));
            }
        }

        bool changedHierarchy = false;
        for (const QString &removedPageId : removedFolderPageIds) {
            changedHierarchy = promotePageNode(removedPageId, false) || changedHierarchy;
        }

        markSettingsDirty();
        if (changedHierarchy) {
            rebuildPageTree();
            selectTreeItemById(currentPageId);
            rebuildPageEditor();
        }
        refreshPageTreeActionButtons();
    }

    void Apps::onPageGridAppDoubleClicked(const QString &appName) {
        if (isParentReference(appName)) {
            const QString parentPageId = parentPageIdForNodeId(selectedPageId());
            if (!parentPageId.isEmpty()) {
                selectTreeItemById(parentPageId);
                showDetailsForPage(parentPageId, false);
            }
            return;
        }
        if (isFolderReference(appName)) {
            const QString pageId = pageIdFromFolderReference(appName);
            selectTreeItemById(pageId);
            showDetailsForPage(pageId, false);
            return;
        }
        showDetailsForApp(appName);
    }

    void Apps::onClearPageSlotClicked() {
        if (m_pageGrid) {
            m_pageGrid->clearSelectedSlot();
        }
        refreshPageTreeActionButtons();
    }

    void Apps::onBackToLayoutClicked() {
        showLayoutEditor();
    }

    void Apps::onShowSelectedPageDetails() {
        if (!m_selectedPageNodeId.isEmpty()) {
            showDetailsForPage(m_selectedPageNodeId, false);
        }
    }

    void Apps::onShowSelectedGridItemDetails() {
        const QString value = selectedGridEntry();
        if (value.isEmpty()) {
            return;
        }
        if (isParentReference(value)) {
            const QString parentPageId = parentPageIdForNodeId(selectedPageId());
            if (!parentPageId.isEmpty()) {
                selectTreeItemById(parentPageId);
                showDetailsForPage(parentPageId, false);
            }
            return;
        }
        if (isFolderReference(value)) {
            const QString pageId = pageIdFromFolderReference(value);
            selectTreeItemById(pageId);
            showDetailsForPage(pageId, false);
            return;
        }
        showDetailsForApp(value, false);
    }

    void Apps::onPageDetailsFieldsTextChanged(const QString &text) {
        Q_UNUSED(text);
        if (!m_currentDetailPageId.isEmpty()) {
            m_pageEditChanged = true;
            if (m_pageDetailsSaveTimer) {
                m_pageDetailsSaveTimer->start();
            }
        }
    }
} // fairwindsk::ui::settings
