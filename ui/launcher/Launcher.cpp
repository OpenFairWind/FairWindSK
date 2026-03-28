//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <algorithm>
#include <functional>
#include <QEnterEvent>
#include <QFrame>
#include <QGridLayout>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStringList>
#include <QTimer>

#include "Launcher.hpp"
#include "AppItem.hpp"

namespace fairwindsk::ui::launcher {
    namespace {
        using OrderedApp = QPair<AppItem *, QString>;
        enum class TileKind {
            Empty,
            App,
            Folder,
            Parent
        };

        struct LauncherTileEntry {
            TileKind kind = TileKind::Empty;
            AppItem *app = nullptr;
            QString id;
            QString title;
            QString description;
            QPixmap pixmap;
        };

        constexpr auto kLauncherLayoutKey = "launcherLayout";
        constexpr auto kLauncherNodesKey = "nodes";
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

        QList<OrderedApp> collectOrderedApps() {
            QList<OrderedApp> orderedApps;
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
            for (const auto &hash : fairWindSK->getAppsHashes()) {
                auto *app = fairWindSK->getAppItemByHash(hash);
                if (app && app->getActive()) {
                    orderedApps.append(OrderedApp(app, hash));
                }
            }

            std::sort(orderedApps.begin(), orderedApps.end(), [](const auto &left, const auto &right) {
                if (left.first->getOrder() != right.first->getOrder()) {
                    return left.first->getOrder() < right.first->getOrder();
                }
                const int displayNameCompare = QString::compare(left.first->getDisplayName(),
                                                                right.first->getDisplayName(),
                                                                Qt::CaseInsensitive);
                if (displayNameCompare != 0) {
                    return displayNameCompare < 0;
                }
                return QString::compare(left.second, right.second, Qt::CaseInsensitive) < 0;
            });

            return orderedApps;
        }

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
                QPixmap pixmap(localPath);
                if (!pixmap.isNull()) {
                    return pixmap;
                }
            }

            QPixmap pixmap(trimmed);
            if (!pixmap.isNull()) {
                return pixmap;
            }

            return QPixmap(QLatin1String(kDefaultPageIconPath));
        }

        QString defaultNodeTitle(const nlohmann::json &node) {
            const QString title = nodeName(node).trimmed();
            if (!title.isEmpty()) {
                return title;
            }
            return QObject::tr("Page");
        }

        bool isPageNode(const nlohmann::json &node) {
            return nodeType(node) == QLatin1String(kNodeTypePage);
        }

        bool isFolderReference(const QString &value) {
            return value.startsWith(QLatin1String(kFolderReferencePrefix));
        }

        bool isParentReference(const QString &value) {
            return value == QLatin1String(kParentReferenceToken);
        }

        QString pageIdFromFolderReference(const QString &value) {
            return isFolderReference(value) ? value.mid(QLatin1String(kFolderReferencePrefix).size()) : QString();
        }

        QString parentPageIdForNodeId(const nlohmann::json &nodes, const QString &pageId) {
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

                    const QString nestedParentId = parentPageIdForNodeId(node[kNodeChildrenKey], pageId);
                    if (!nestedParentId.isEmpty()) {
                        return nestedParentId;
                    }
                }
            }

            return {};
        }

        const nlohmann::json *findNodeById(const nlohmann::json &nodes, const QString &id) {
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

        QStringList pageItemsFromNode(const nlohmann::json &node, const int itemsPerPage) {
            QStringList items;
            items.reserve(itemsPerPage);
            int itemIndex = 0;
            if (node.contains(kNodeItemsKey) && node[kNodeItemsKey].is_array()) {
                for (const auto &item : node[kNodeItemsKey]) {
                    if (itemIndex >= itemsPerPage) {
                        break;
                    }
                    items.append(item.is_string() ? QString::fromStdString(item.get<std::string>()) : QString());
                    ++itemIndex;
                }
            }
            while (itemIndex < itemsPerPage) {
                items.append(QString());
                ++itemIndex;
            }
            return items;
        }

        const nlohmann::json *resolveScopeNodes(const nlohmann::json &allNodes,
                                                const QString &rootPageId,
                                                nlohmann::json &scratchNodes) {
            if (rootPageId.isEmpty()) {
                return &allNodes;
            }

            if (const auto *rootNode = findNodeById(allNodes, rootPageId)) {
                scratchNodes = nlohmann::json::array();
                scratchNodes.push_back(*rootNode);
                return &scratchNodes;
            }

            return &allNodes;
        }

        QList<LauncherTileEntry> collectLauncherEntries(const QString &rootPageId,
                                                        const int itemsPerPage,
                                                        bool *usedFallbackRoot = nullptr) {
            QList<LauncherTileEntry> entries;
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
            auto *configuration = fairWindSK->getConfiguration();
            auto &root = configuration->getRoot();

            if (root.contains(kLauncherLayoutKey) && root[kLauncherLayoutKey].is_object()) {
                const auto &layout = root[kLauncherLayoutKey];
                if (layout.contains(kLauncherNodesKey) && layout[kLauncherNodesKey].is_array()) {
                    const auto &allNodes = layout[kLauncherNodesKey];
                    nlohmann::json scopedNodesScratch;
                    const nlohmann::json *scopedNodes = resolveScopeNodes(allNodes, rootPageId, scopedNodesScratch);
                    if (usedFallbackRoot) {
                        *usedFallbackRoot = (!rootPageId.isEmpty() && scopedNodes == &allNodes);
                    }

                    if (scopedNodes && scopedNodes->is_array()) {
                        for (const auto &node : *scopedNodes) {
                            if (!node.is_object() || !isPageNode(node)) {
                                continue;
                            }

                            const QStringList pageEntries = pageItemsFromNode(node, itemsPerPage);
                            for (const QString &slot : pageEntries) {
                                LauncherTileEntry entry;
                                if (slot.trimmed().isEmpty()) {
                                    entries.append(entry);
                                    continue;
                                }

                                if (isParentReference(slot)) {
                                    entry.kind = TileKind::Parent;
                                    entry.id = parentPageIdForNodeId(allNodes, nodeId(node));
                                    entry.title = QObject::tr("Parent page");
                                    entry.description = QObject::tr("Return to the parent page");
                                    entry.pixmap = QPixmap(QLatin1String(kParentNavigationIconPath));
                                    entries.append(entry);
                                    continue;
                                }

                                if (isFolderReference(slot)) {
                                    const QString childPageId = pageIdFromFolderReference(slot);
                                    entry.kind = TileKind::Folder;
                                    entry.id = childPageId;
                                    if (const auto *childNode = findNodeById(allNodes, childPageId)) {
                                        entry.title = defaultNodeTitle(*childNode);
                                        entry.description = QObject::tr("Open folder");
                                        entry.pixmap = pageIconForPath(nodeIconPath(*childNode));
                                    } else {
                                        entry.title = QObject::tr("Folder");
                                        entry.description = QObject::tr("Open folder");
                                        entry.pixmap = QPixmap(QLatin1String(kDefaultPageIconPath));
                                    }
                                    entries.append(entry);
                                    continue;
                                }

                                auto *app = fairWindSK->getAppItemByHash(slot);
                                if (app && app->getActive()) {
                                    entry.kind = TileKind::App;
                                    entry.app = app;
                                    entry.id = slot;
                                    entry.title = app->getDisplayName();
                                    entry.description = app->getDescription();
                                    entry.pixmap = app->getIcon();
                                }
                                entries.append(entry);
                            }
                        }
                    }
                }
            }

            if (entries.isEmpty()) {
                const auto orderedApps = collectOrderedApps();
                for (const auto &item : orderedApps) {
                    LauncherTileEntry entry;
                    if (item.first && item.first->getActive()) {
                        entry.kind = TileKind::App;
                        entry.app = item.first;
                        entry.id = item.second;
                        entry.title = item.first->getDisplayName();
                        entry.description = item.first->getDescription();
                        entry.pixmap = item.first->getIcon();
                    }
                    entries.append(entry);
                }
            }

            return entries;
        }

        class AppTile final : public QFrame {
        public:
            explicit AppTile(QWidget *parent = nullptr) : QFrame(parent) {
                setCursor(Qt::PointingHandCursor);
                setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            }

            void setAppData(const QString &hash,
                            const QString &title,
                            const QString &description,
                            const QPixmap &pixmap) {
                m_hash = hash;
                m_title = title;
                m_pixmap = pixmap;
                setToolTip(description);
                update();
            }

            void setBasePointSize(const qreal pointSize) {
                m_basePointSize = pointSize;
            }

            void setActivateHandler(std::function<void(const QString &)> handler) {
                m_onActivated = std::move(handler);
            }

            qreal basePointSize() const {
                return m_basePointSize;
            }

            QString appHash() const {
                return m_hash;
            }

        protected:
            void enterEvent(QEnterEvent *event) override {
                QFrame::enterEvent(event);
                m_hovered = true;
                update();
            }

            void leaveEvent(QEvent *event) override {
                QFrame::leaveEvent(event);
                m_hovered = false;
                update();
            }

            void mouseReleaseEvent(QMouseEvent *event) override {
                QFrame::mouseReleaseEvent(event);
                if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()) && m_onActivated) {
                    m_onActivated(m_hash);
                }
            }

            void paintEvent(QPaintEvent *event) override {
                Q_UNUSED(event);

                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                const QRectF tileRect = rect().adjusted(1, 1, -1, -1);
                const qreal radius = 2.0;

                QPainterPath clipPath;
                clipPath.addRoundedRect(tileRect, radius, radius);
                painter.setClipPath(clipPath);

                painter.fillRect(tileRect, QColor(16, 22, 32));
                if (!m_pixmap.isNull()) {
                    const QPixmap scaled = m_pixmap.scaled(tileRect.size().toSize(),
                                                           Qt::KeepAspectRatioByExpanding,
                                                           Qt::SmoothTransformation);
                    const QRect sourceRect((scaled.width() - int(tileRect.width())) / 2,
                                           (scaled.height() - int(tileRect.height())) / 2,
                                           int(tileRect.width()),
                                           int(tileRect.height()));
                    painter.drawPixmap(tileRect.toRect(), scaled, sourceRect);
                }

                QLinearGradient overlay(tileRect.topLeft(), QPointF(tileRect.left(), tileRect.bottom()));
                overlay.setColorAt(0.0, QColor(0, 0, 0, 0));
                overlay.setColorAt(0.55, QColor(0, 0, 0, 10));
                overlay.setColorAt(1.0, QColor(0, 0, 0, 170));
                painter.fillRect(tileRect, overlay);

                painter.setClipping(false);
                painter.setPen(QPen(m_hovered ? QColor(255, 255, 255) : QColor(230, 231, 235), m_hovered ? 2.0 : 1.0));
                painter.drawRoundedRect(tileRect, radius, radius);

                QFont titleFont = font();
                if (m_basePointSize > 0.0) {
                    titleFont.setPointSizeF(m_basePointSize);
                }
                painter.setFont(titleFont);
                painter.setPen(QColor(248, 250, 252));
                painter.drawText(tileRect.adjusted(10, 10, -10, -10).toRect(),
                                 Qt::AlignLeft | Qt::AlignBottom | Qt::TextWordWrap,
                                 m_title);
            }

        private:
            QString m_hash;
            QString m_title;
            QPixmap m_pixmap;
            qreal m_basePointSize = 0.0;
            bool m_hovered = false;
            std::function<void(const QString &)> m_onActivated;
        };

        const QString kNavigationButtonStyle = QStringLiteral(
            "QToolButton {"
            " background: transparent;"
            " color: #f9fafb;"
            " border: none;"
            " padding: 8px;"
            " }"
            "QToolButton:hover { background: rgba(255, 255, 255, 0.08); border-radius: 8px; }"
            "QToolButton:pressed { background: rgba(255, 255, 255, 0.14); border-radius: 8px; }");
        const QString kLauncherFrameStyle = QStringLiteral(
            "QScrollArea {"
            " background: transparent;"
            " border: 4px solid rgba(245, 245, 245, 0.95);"
            " border-radius: 3px;"
            " }");
    }

    Launcher::Launcher(QWidget *parent) : QWidget(parent), ui(new Ui::Launcher) {
        ui->setupUi(this);

        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(6, 6, 6, 6);
            m_layout->setHorizontalSpacing(4);
            m_layout->setVerticalSpacing(4);
            ui->toolButton_Left->setStyleSheet(kNavigationButtonStyle);
            ui->toolButton_Right->setStyleSheet(kNavigationButtonStyle);
            ui->toolButton_Left->setAutoRaise(true);
            ui->toolButton_Right->setAutoRaise(true);
            ui->scrollArea->setFrameShape(QFrame::NoFrame);
            ui->scrollArea->setStyleSheet(kLauncherFrameStyle);

            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->setWidgetResizable(false);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);
            ui->scrollArea->viewport()->installEventFilter(this);

            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                m_targetPage = currentPage();
                updateScrollButtons();
            });
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() { updateScrollButtons(); });
            refreshFromConfiguration();
        }
    }

    Launcher::~Launcher() {
        delete ui;
        ui = nullptr;
    }

    void Launcher::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        resize();
    }

    void Launcher::showEvent(QShowEvent *event) {
        QWidget::showEvent(event);
        resize();
    }

    void Launcher::refreshFromConfiguration(const bool forceRebuild) {
        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        const int newColumns = std::max(1, configuration->getLauncherColumns());
        const int newRows = std::max(1, configuration->getLauncherRows());
        const QString newLayoutSignature = buildLayoutSignature();
        const bool geometryChanged = m_cols != newColumns || m_rows != newRows;
        const bool contentChanged = m_layoutSignature != newLayoutSignature;

        m_targetPage = qBound(0, currentPage(), std::max(0, m_pageCount - 1));
        m_cols = newColumns;
        m_rows = newRows;

        if (forceRebuild || geometryChanged || contentChanged || m_tiles.isEmpty()) {
            rebuildTiles();
            m_layoutSignature = newLayoutSignature;
        }

        if (isVisible()) {
            QTimer::singleShot(0, this, [this]() { resize(); });
        }
        updateScrollButtons();
    }

    bool Launcher::eventFilter(QObject *watched, QEvent *event) {
        if (watched == ui->scrollArea->viewport() && event && event->type() == QEvent::Resize && isVisible()) {
            resize();
        }

        return QWidget::eventFilter(watched, event);
    }

    void Launcher::resize() {
        if (!isVisible()) {
            return;
        }

        auto *layout = qobject_cast<QGridLayout *>(ui->scrollAreaWidgetContents->layout());
        if (!layout) {
            return;
        }

        const auto viewportSize = ui->scrollArea->viewport()->size();
        if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
            return;
        }

        const int availableHeight = qMax(220, viewportSize.height() - layout->contentsMargins().top() - layout->contentsMargins().bottom());
        const int availableWidth = qMax(320, viewportSize.width() - layout->contentsMargins().left() - layout->contentsMargins().right());
        const int rowHeight = qMax(110, (availableHeight - ((m_rows - 1) * layout->verticalSpacing())) / qMax(1, m_rows));
        const int columnWidth = qMax(140, (availableWidth - ((m_cols - 1) * layout->horizontalSpacing())) / qMax(1, m_cols));
        const int totalColumns = std::max(1, m_pageCount * m_cols);
        const int contentWidth = std::max(viewportSize.width(),
                                          layout->contentsMargins().left() +
                                          layout->contentsMargins().right() +
                                          (totalColumns * columnWidth) +
                                          (std::max(0, totalColumns - 1) * layout->horizontalSpacing()));
        const int contentHeight = std::max(viewportSize.height(),
                                           layout->contentsMargins().top() +
                                           layout->contentsMargins().bottom() +
                                           (m_rows * rowHeight) +
                                           (std::max(0, m_rows - 1) * layout->verticalSpacing()));

        for (int col = 0; col < totalColumns; ++col) {
            layout->setColumnMinimumWidth(col, columnWidth);
        }

        for (int row = 0; row < m_rows; ++row) {
            layout->setRowMinimumHeight(row, rowHeight);
        }

        for (auto *tileWidget : m_tiles) {
            auto *tile = dynamic_cast<AppTile *>(tileWidget);
            if (!tile) {
                continue;
            }

            QFont font = tile->font();
            const qreal basePointSize = tile->basePointSize();
            if (basePointSize > 0.0) {
                font.setPointSizeF(std::max<qreal>(10.0, basePointSize));
                tile->setFont(font);
            }
            tile->setFixedSize(columnWidth, rowHeight);
        }

        ui->scrollAreaWidgetContents->resize(contentWidth, contentHeight);
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const int targetValue = qBound(scrollBar->minimum(),
                                       m_targetPage * pageWidth(),
                                       scrollBar->maximum());
        if (scrollBar->value() != targetValue) {
            ui->scrollArea->horizontalScrollBar()->setValue(targetValue);
        }
        updateScrollButtons();
    }

    void Launcher::onScrollLeft() {
        if (currentPage() == 0 && !m_navigationStack.isEmpty()) {
            m_currentRootPageId = m_navigationStack.takeLast();
            m_targetPage = 0;
            refreshFromConfiguration(true);
            return;
        }
        m_targetPage = std::max(0, currentPage() - 1);
        ui->scrollArea->horizontalScrollBar()->setValue(m_targetPage * pageWidth());
    }

    void Launcher::onScrollRight() {
        m_targetPage = std::min(m_pageCount - 1, currentPage() + 1);
        ui->scrollArea->horizontalScrollBar()->setValue(m_targetPage * pageWidth());
    }

    void Launcher::updateScrollButtons() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const bool canScroll = scrollBar->maximum() > scrollBar->minimum();
        const bool canGoBack = !m_navigationStack.isEmpty() && currentPage() == 0;
        ui->toolButton_Left->setEnabled(canGoBack || (canScroll && scrollBar->value() > scrollBar->minimum()));
        ui->toolButton_Right->setEnabled(canScroll && scrollBar->value() < scrollBar->maximum());
    }

    int Launcher::pageWidth() const {
        return qMax(1, ui->scrollArea->viewport()->width());
    }

    int Launcher::currentPage() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        return qBound(0, (scrollBar->value() + (pageWidth() / 2)) / pageWidth(), std::max(0, m_pageCount - 1));
    }

    QString Launcher::buildLayoutSignature() const {
        QStringList parts;
        const int itemsPerPage = std::max(1, m_rows * m_cols);
        bool usedFallbackRoot = false;
        const auto entries = collectLauncherEntries(m_currentRootPageId, itemsPerPage, &usedFallbackRoot);
        parts.reserve(entries.size() + 2);
        parts.append(m_currentRootPageId);
        parts.append(usedFallbackRoot ? QStringLiteral("root-fallback") : QStringLiteral("root-ok"));

        for (const auto &entry : entries) {
            if (entry.kind == TileKind::Empty) {
                parts.append(QStringLiteral("_"));
                continue;
            }
            if (entry.kind == TileKind::Folder) {
                parts.append(QStringLiteral("folder|%1|%2").arg(entry.id, entry.title));
                continue;
            }
            if (entry.kind == TileKind::Parent) {
                parts.append(QStringLiteral("parent|%1|%2").arg(entry.id, entry.title));
                continue;
            }
            auto *appItem = entry.app;
            parts.append(QStringLiteral("%1|%2|%3|%4|%5")
                             .arg(entry.id,
                                  QString::number(appItem ? appItem->getOrder() : 0),
                                  entry.title,
                                  entry.description,
                                  appItem ? appItem->getAppIcon() : QString()));
        }

        return parts.join(QStringLiteral("||"));
    }

    void Launcher::rebuildTiles() {
        if (!m_layout) {
            return;
        }

        const int preservedPage = qBound(0, m_targetPage, std::max(0, m_pageCount - 1));
        while (m_layout->count() > 0) {
            auto *item = m_layout->takeAt(0);
            if (auto *widget = item->widget()) {
                delete widget;
            }
            delete item;
        }
        m_tiles.clear();

        const int itemsPerPage = std::max(1, m_rows * m_cols);
        bool usedFallbackRoot = false;
        const auto entries = collectLauncherEntries(m_currentRootPageId, itemsPerPage, &usedFallbackRoot);
        if (usedFallbackRoot) {
            m_currentRootPageId.clear();
            m_navigationStack.clear();
        }
        m_pageCount = std::max(1, int((entries.size() + itemsPerPage - 1) / itemsPerPage));
        m_targetPage = qBound(0, preservedPage, std::max(0, m_pageCount - 1));
        int index = 0;
        for (const auto &entry : entries) {
            const int page = index / itemsPerPage;
            const int indexInPage = index % itemsPerPage;
            const int row = indexInPage / m_cols;
            const int col = (page * m_cols) + (indexInPage % m_cols);

            if (entry.kind == TileKind::Empty) {
                ++index;
                continue;
            }

            auto *tile = new AppTile(ui->scrollAreaWidgetContents);
            tile->setBasePointSize(tile->font().pointSizeF() + 1.0);
            tile->setAppData(entry.id, entry.title, entry.description, entry.pixmap);
            const TileKind kind = entry.kind;
            tile->setActivateHandler([this, kind](const QString &hash) {
                if (hash.isEmpty()) {
                    if (kind == TileKind::Parent && !m_navigationStack.isEmpty()) {
                        m_currentRootPageId = m_navigationStack.takeLast();
                        m_targetPage = 0;
                        refreshFromConfiguration(true);
                    }
                    return;
                }

                if (kind == TileKind::Folder) {
                    m_navigationStack.append(m_currentRootPageId);
                    m_currentRootPageId = hash;
                    m_targetPage = 0;
                    refreshFromConfiguration(true);
                    return;
                }

                if (kind == TileKind::Parent) {
                    if (!m_navigationStack.isEmpty()) {
                        const QString previousRoot = m_navigationStack.takeLast();
                        m_currentRootPageId = previousRoot.isEmpty() ? hash : previousRoot;
                    } else {
                        m_currentRootPageId = hash;
                    }
                    m_targetPage = 0;
                    refreshFromConfiguration(true);
                    return;
                }

                emit foregroundAppChanged(hash);
            });

            m_layout->addWidget(tile, row, col);
            m_tiles.append(tile);
            ++index;
        }
    }

} // fairwindsk::ui::launcher
