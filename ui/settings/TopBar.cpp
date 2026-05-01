#include "TopBar.hpp"

#include <algorithm>
#include <functional>
#include <utility>
#include <QAbstractItemModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollerProperties>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "BarSettingsUi.hpp"
#include "ui/IconUtils.hpp"
#include "ui/layout/BarLayout.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        using fairwindsk::ui::layout::BarId;
        using fairwindsk::ui::layout::EntryKind;
        using fairwindsk::ui::layout::LayoutEntry;

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
                setDragDropOverwriteMode(false);
                setDefaultDropAction(Qt::MoveAction);
                setDragEnabled(true);
                setAcceptDrops(true);
                setDropIndicatorShown(true);
                setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setTextElideMode(Qt::ElideRight);
            }

            void setEditedCallback(std::function<void()> callback) {
                m_editedCallback = std::move(callback);
            }

        protected:
            void mousePressEvent(QMouseEvent *event) override {
                m_draggedRow = event ? row(itemAt(event->position().toPoint())) : -1;
                QListWidget::mousePressEvent(event);
            }

            int insertRowForPosition(const QPoint &position) const {
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

            void dragEnterEvent(QDragEnterEvent *event) override {
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

            void dragMoveEvent(QDragMoveEvent *event) override {
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

            void dropEvent(QDropEvent *event) override {
                if (isInternalDrop(event)) {
                    const int oldRow = m_draggedRow >= 0 ? m_draggedRow : currentRow();
                    if (oldRow < 0 || oldRow >= count()) {
                        m_draggedRow = -1;
                        event->ignore();
                        return;
                    }

                    int insertRow = insertRowForPosition(event->position().toPoint());
                    if (oldRow < insertRow) {
                        --insertRow;
                    }
                    insertRow = std::clamp(insertRow, 0, count() - 1);

                    if (oldRow != insertRow) {
                        const QSignalBlocker blocker(model());
                        QListWidgetItem *movedItem = takeItem(oldRow);
                        insertItem(insertRow, movedItem);
                        setCurrentItem(movedItem);
                    }

                    m_draggedRow = -1;
                    event->setDropAction(Qt::MoveAction);
                    event->accept();
                    notifyEdited();
                    return;
                }

                if (!event || !event->mimeData()->hasFormat(WidgetPalette::mimeType())) {
                    QListWidget::dropEvent(event);
                    return;
                }

                LayoutEntry entry = WidgetPalette::decodeEntry(event->mimeData()->data(WidgetPalette::mimeType()));
                if (entry.kind != EntryKind::Widget && entry.instanceId.isEmpty()) {
                    entry.instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                }

                int insertRow = insertRowForPosition(event->position().toPoint());

                if (entry.kind == EntryKind::Widget) {
                    for (int row = 0; row < count(); ++row) {
                        auto *existingItem = item(row);
                        if (!existingItem) {
                            continue;
                        }
                        if (existingItem->data(TopBar::RoleWidgetId).toString() != entry.widgetId) {
                            continue;
                        }

                        const QSignalBlocker blocker(model());
                        QListWidgetItem *movedItem = takeItem(row);
                        if (row < insertRow) {
                            --insertRow;
                        }
                        insertItem(std::clamp(insertRow, 0, count()), movedItem);
                        setCurrentItem(movedItem);
                        m_draggedRow = -1;
                        event->setDropAction(Qt::MoveAction);
                        event->accept();
                        notifyEdited();
                        return;
                    }
                }

                auto *newItem = new QListWidgetItem();
                newItem->setFlags(Qt::ItemIsEnabled |
                                  Qt::ItemIsSelectable |
                                  Qt::ItemIsDragEnabled |
                                  Qt::ItemIsDropEnabled);
                newItem->setIcon(WidgetPalette::entryIcon(entry));
                newItem->setText(layout::entryLabel(entry));
                newItem->setTextAlignment(Qt::AlignCenter);
                newItem->setData(TopBar::RoleKind, static_cast<int>(entry.kind));
                newItem->setData(TopBar::RoleWidgetId, entry.widgetId);
                newItem->setData(TopBar::RoleInstanceId, entry.instanceId);
                newItem->setData(TopBar::RoleExpandHorizontally, entry.expandHorizontally);
                newItem->setData(TopBar::RoleExpandVertically, entry.expandVertically);
                const QSignalBlocker blocker(model());
                insertItem(std::clamp(insertRow, 0, count()), newItem);
                setCurrentItem(newItem);
                event->setDropAction(Qt::CopyAction);
                event->accept();
                notifyEdited();
            }

        private:
            bool isInternalDrop(const QDropEvent *event) const {
                return event &&
                       !event->mimeData()->hasFormat(WidgetPalette::mimeType()) &&
                       (event->source() == this ||
                        event->source() == viewport() ||
                        (m_draggedRow >= 0 && event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist"))));
            }

            bool isInternalDrop(const QDragMoveEvent *event) const {
                return event &&
                       !event->mimeData()->hasFormat(WidgetPalette::mimeType()) &&
                       (event->source() == this ||
                        event->source() == viewport() ||
                        (m_draggedRow >= 0 && event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist"))));
            }

            void notifyEdited() const {
                if (m_editedCallback) {
                    m_editedCallback();
                }
            }

            int m_draggedRow = -1;
            std::function<void()> m_editedCallback;
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
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(8);

        m_previewFrame = new QFrame(this);
        m_previewFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_previewFrame->setMinimumHeight(78);
        m_previewFrame->setMaximumHeight(78);
        auto *previewFrameLayout = new QHBoxLayout(m_previewFrame);
        previewFrameLayout->setContentsMargins(0, 0, 0, 0);
        previewFrameLayout->setSpacing(6);

        m_leftShellButton = new QToolButton(m_previewFrame);
        m_rightShellButton = new QToolButton(m_previewFrame);
        for (auto *button : {m_leftShellButton, m_rightShellButton}) {
            button->setAutoRaise(true);
            button->setIconSize(QSize(28, 28));
            button->setMinimumSize(QSize(60, 60));
            button->setMaximumSize(QSize(60, 60));
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setEnabled(true);
            button->setFocusPolicy(Qt::NoFocus);
        }
        m_leftShellButton->setToolTip(tr("Fixed FairWindSK shell button"));
        m_rightShellButton->setToolTip(tr("Fixed current-application shell button"));

        auto *previewWidget = new PreviewListWidget(m_previewFrame);
        previewWidget->setEditedCallback([this]() { onPreviewEdited(); });
        m_previewWidget = previewWidget;
        m_previewWidget->setSpacing(4);
        m_previewWidget->setIconSize(QSize(30, 30));
        m_previewWidget->setMinimumHeight(74);
        m_previewWidget->setMaximumHeight(74);
        m_previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_previewWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::TouchGesture);
        m_previewWidget->setToolTip(tr("Tap a widget to edit it. Drag within the preview to reorder the top bar."));
        previewFrameLayout->addWidget(m_leftShellButton, 0, Qt::AlignVCenter);
        previewFrameLayout->addWidget(m_previewWidget, 1);
        previewFrameLayout->addWidget(m_rightShellButton, 0, Qt::AlignVCenter);
        rootLayout->addWidget(m_previewFrame);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);

        m_minimumWidthButton = new QToolButton(this);
        m_maximumWidthButton = new QToolButton(this);
        m_minimumHeightButton = new QToolButton(this);
        m_expandHeightButton = new QToolButton(this);
        m_moveLeftButton = new QToolButton(this);
        m_moveRightButton = new QToolButton(this);
        m_removeSelectedButton = new QToolButton(this);
        m_resetDefaultsButton = new QToolButton(this);

        barsettings::configureActionButton(m_removeSelectedButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg"),
                                           tr("Remove Selected"),
                                           kControlHeight);
        barsettings::configureActionButton(m_resetDefaultsButton,
                                           QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg"),
                                           tr("Reset Defaults"),
                                           kControlHeight);
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
                                         tr("Keep the selected widget at the normal top-bar height"),
                                         tr("Minimize vertically"),
                                         kControlHeight);
        barsettings::configureSizeButton(m_expandHeightButton,
                                         QStringLiteral(":/resources/svg/OpenBridge/layout-vertical-maximize.svg"),
                                         tr("Let the selected widget use the full top-bar height"),
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
            entry.expandHorizontally = layout::defaultExpandHorizontally(BarId::Top, definition.id);
            paletteEntries.append(entry);
        }
        m_paletteWidget->setEntries(paletteEntries);

        connect(m_paletteWidget, &WidgetPalette::entryActivated, this, &TopBar::onPaletteEntryActivated);
        connect(m_previewWidget, &QListWidget::itemSelectionChanged, this, &TopBar::onPreviewSelectionChanged);
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsInserted, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsRemoved, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() { onPreviewEdited(); });
        connect(m_minimumWidthButton, &QToolButton::clicked, this, &TopBar::onMinimumWidthSelected);
        connect(m_maximumWidthButton, &QToolButton::clicked, this, &TopBar::onMaximumWidthSelected);
        connect(m_minimumHeightButton, &QToolButton::clicked, this, &TopBar::onMinimumHeightSelected);
        connect(m_expandHeightButton, &QToolButton::clicked, this, &TopBar::onMaximumHeightSelected);
        connect(m_moveLeftButton, &QToolButton::clicked, this, &TopBar::onMoveSelectedLeft);
        connect(m_moveRightButton, &QToolButton::clicked, this, &TopBar::onMoveSelectedRight);
        connect(m_removeSelectedButton, &QToolButton::clicked, this, &TopBar::onRemoveSelected);
        connect(m_resetDefaultsButton, &QToolButton::clicked, this, &TopBar::onResetDefaults);

        m_previewWidget->installEventFilter(this);
        m_previewWidget->viewport()->installEventFilter(this);

        applyChrome();
        updateInspector();
    }

    void TopBar::applyChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration() : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 18.0);
        barsettings::applySecondaryLabelStyle(m_previewLabel, chrome);
        barsettings::applySecondaryLabelStyle(m_paletteLabel, chrome);
        barsettings::applyHintLabelStyle(m_hintLabel, chrome);
        if (m_previewWidget) {
            m_previewWidget->setStyleSheet(barsettings::listWidgetChrome(chrome, false));
        }
        if (m_previewFrame) {
            m_previewFrame->setStyleSheet(barsettings::previewFrameChrome(chrome));
        }
        if (m_paletteWidget) {
            m_paletteWidget->setStyleSheet(barsettings::paletteChrome(chrome));
        }

        if (m_leftShellButton) {
            m_leftShellButton->setIcon(QIcon(QStringLiteral(":/resources/images/mainwindow/fairwind_icon.png")));
            fairwindsk::ui::applyBottomBarToolButtonChrome(m_leftShellButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(28, 28), 60);
        }
        if (m_rightShellButton) {
            m_rightShellButton->setIcon(QIcon(QStringLiteral(":/resources/images/icons/apps_icon.png")));
            fairwindsk::ui::applyBottomBarToolButtonChrome(m_rightShellButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(28, 28), 60);
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

    bool TopBar::eventFilter(QObject *watched, QEvent *event) {
        if ((watched == m_previewWidget || (m_previewWidget && watched == m_previewWidget->viewport())) &&
            event &&
            (event->type() == QEvent::Resize ||
             event->type() == QEvent::Show)) {
            refreshPreviewItems();
        }

        return QWidget::eventFilter(watched, event);
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

    void TopBar::appendPaletteEntryToPreview(const LayoutEntry &entry) {
        if (!m_previewWidget) {
            return;
        }

        if (entry.kind == EntryKind::Widget) {
            if (auto *existingItem = findPreviewWidgetItem(entry.widgetId)) {
                m_previewWidget->setCurrentItem(existingItem);
                refreshPreviewItem(existingItem);
                return;
            }
        }

        auto *item = createPreviewItem(entry);
        m_previewWidget->addItem(item);
        m_previewWidget->setCurrentItem(item);
        refreshPreviewItems();
    }

    void TopBar::refreshPreviewItem(QListWidgetItem *item) const {
        if (!item) {
            return;
        }

        const auto entry = entryForPreviewItem(item);
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setText(barsettings::previewEntryText(entry));
        item->setSizeHint(itemSizeHint(entry));
    }

    QString TopBar::itemSummary(const LayoutEntry &entry) const {
        QStringList summary;
        summary.append(layout::horizontalSizeLabel(entry));
        if (entry.expandVertically) {
            summary.append(tr("Height"));
        }

        if (entry.kind == EntryKind::Stretch) {
            summary.clear();
            summary.append(tr("Elastic"));
        }

        return summary.join(QStringLiteral(" + "));
    }

    int TopBar::minimumItemWidth(const LayoutEntry &entry) const {
        if (entry.kind == EntryKind::Separator) {
            return 48;
        }
        if (entry.kind == EntryKind::Stretch) {
            return 60;
        }

        return 72;
    }

    QSize TopBar::itemSizeHint(const LayoutEntry &entry) const {
        int width = minimumItemWidth(entry);
        if (m_previewWidget && m_previewWidget->count() > 0) {
            int totalMinimumWidth = 0;
            int expandingEntries = 0;
            for (int row = 0; row < m_previewWidget->count(); ++row) {
                const auto itemEntry = entryForPreviewItem(m_previewWidget->item(row));
                totalMinimumWidth += minimumItemWidth(itemEntry);
                if (itemEntry.expandHorizontally || itemEntry.kind == EntryKind::Stretch) {
                    ++expandingEntries;
                }
            }

            const int spacingWidth = m_previewWidget->spacing() * std::max(0, m_previewWidget->count() - 1);
            const int chromeWidth = 16;
            const int viewportWidth = m_previewWidget->viewport()
                                          ? m_previewWidget->viewport()->width()
                                          : m_previewWidget->width();
            const int spareWidth = viewportWidth - totalMinimumWidth - spacingWidth - chromeWidth;
            if ((entry.expandHorizontally || entry.kind == EntryKind::Stretch) && expandingEntries > 0 && spareWidth > 0) {
                width += spareWidth / expandingEntries;
            }
        }

        int height = entry.expandVertically ? 72 : 68;
        if (entry.kind == EntryKind::Separator && entry.expandVertically) {
            height = 72;
        }
        return QSize(width, height);
    }

    QListWidgetItem *TopBar::createPreviewItem(const LayoutEntry &entry) const {
        auto *item = new QListWidgetItem();
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setData(RoleExpandHorizontally, entry.expandHorizontally);
        item->setData(RoleExpandVertically, entry.expandVertically);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled |
                       Qt::ItemIsDropEnabled);
        item->setText(barsettings::previewEntryText(entry));
        item->setSizeHint(itemSizeHint(entry));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    }

    void TopBar::refreshPreviewItems() const {
        if (!m_previewWidget) {
            return;
        }

        for (int row = 0; row < m_previewWidget->count(); ++row) {
            refreshPreviewItem(m_previewWidget->item(row));
        }
        m_previewWidget->doItemsLayout();
        m_previewWidget->viewport()->update();
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

        applyChrome();
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
        refreshPreviewItems();

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
            entry.expandHorizontally = layout::defaultExpandHorizontally(BarId::Top, definition.id);
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
        const int currentRow = m_previewWidget ? m_previewWidget->currentRow() : -1;
        const int itemCount = m_previewWidget ? m_previewWidget->count() : 0;

        if (m_minimumWidthButton) {
            const QSignalBlocker blocker(m_minimumWidthButton);
            m_minimumWidthButton->setEnabled(hasSelection && entry.kind == EntryKind::Widget);
            m_minimumWidthButton->setChecked(hasSelection && entry.kind == EntryKind::Widget && !entry.expandHorizontally);
        }
        if (m_maximumWidthButton) {
            const QSignalBlocker blocker(m_maximumWidthButton);
            m_maximumWidthButton->setEnabled(hasSelection && entry.kind == EntryKind::Widget);
            m_maximumWidthButton->setChecked(hasSelection && entry.kind == EntryKind::Widget && entry.expandHorizontally);
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

    void TopBar::updatePaletteState() {
        if (!m_paletteWidget) {
            return;
        }
        QList<LayoutEntry> activeEntries;
        for (int row = 0; row < m_previewWidget->count(); ++row) {
            if (auto *item = m_previewWidget->item(row)) {
                activeEntries.append(entryForPreviewItem(item));
            }
        }
        m_paletteWidget->setActiveEntries(activeEntries);
    }

    void TopBar::onPaletteEntryActivated(const LayoutEntry &entry) {
        appendPaletteEntryToPreview(entry);
    }

    void TopBar::onPreviewSelectionChanged() {
        updateInspector();
    }

    void TopBar::onMinimumWidthSelected() {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandHorizontally, false);
        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::onMaximumWidthSelected() {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandHorizontally, true);
        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::onMinimumHeightSelected() {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandVertically, false);
        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::onMaximumHeightSelected() {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandVertically, true);
        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::moveCurrentPreviewItem(const int offset) {
        if (!m_previewWidget || offset == 0) {
            return;
        }

        const int currentRow = m_previewWidget->currentRow();
        if (currentRow < 0 || m_previewWidget->count() <= 1) {
            return;
        }

        const int targetRow = std::clamp(currentRow + offset, 0, m_previewWidget->count() - 1);
        if (currentRow == targetRow) {
            return;
        }

        QListWidgetItem *item = nullptr;
        {
            const QSignalBlocker blocker(m_previewWidget->model());
            item = m_previewWidget->takeItem(currentRow);
            m_previewWidget->insertItem(targetRow, item);
            m_previewWidget->setCurrentItem(item);
        }

        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }

    void TopBar::onMoveSelectedLeft() {
        moveCurrentPreviewItem(-1);
    }

    void TopBar::onMoveSelectedRight() {
        moveCurrentPreviewItem(1);
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
        refreshPreviewItems();
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

        refreshPreviewItems();
        persistToConfiguration();
        updateInspector();
    }
}
