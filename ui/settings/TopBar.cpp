#include "TopBar.hpp"

#include <algorithm>
#include <QAbstractItemModel>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
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
#include "ui/IconUtils.hpp"
#include "ui/layout/BarLayout.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        using fairwindsk::ui::layout::BarId;
        using fairwindsk::ui::layout::EntryKind;
        using fairwindsk::ui::layout::LayoutEntry;

        constexpr auto kTopBarPaletteMimeType = "application/x-fairwindsk-topbar-entry";

        QString listWidgetChrome(const fairwindsk::ui::ComfortChromeColors &colors,
                                 const bool dashedItems = false) {
            return QStringLiteral(
                "QListWidget {"
                " border: 1px solid %1;"
                " border-radius: 12px;"
                " padding: 8px;"
                " background: %2;"
                " }"
                "QListWidget::item {"
                " margin: 4px;"
                " padding: 10px 12px;"
                " border: 1px %3 %4;"
                " border-radius: 10px;"
                " background: %5;"
                " color: %6;"
                " }"
                "QListWidget::item:selected {"
                " border-color: %7;"
                " background: %8;"
                " color: %9;"
                " font-weight: 600;"
                " }")
                .arg(colors.border.name(),
                     colors.window.name(),
                     dashedItems ? QStringLiteral("dashed") : QStringLiteral("solid"),
                     colors.border.name(),
                     fairwindsk::ui::comfortAlpha(colors.buttonBackground, dashedItems ? 22 : 32).name(QColor::HexArgb),
                     colors.text.name(),
                     colors.accentTop.name(),
                     fairwindsk::ui::comfortAlpha(colors.accentTop, 46).name(QColor::HexArgb),
                     colors.text.name());
        }

        void applySecondaryLabelStyle(QLabel *label, const fairwindsk::ui::ComfortChromeColors &colors) {
            if (!label) {
                return;
            }

            QFont font = label->font();
            font.setBold(true);
            font.setPointSizeF(std::max(font.pointSizeF(), 13.0));
            label->setFont(font);

            QPalette palette = label->palette();
            palette.setColor(QPalette::WindowText, colors.text);
            palette.setColor(QPalette::Text, colors.text);
            label->setPalette(palette);
        }

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
            tr("Drag items from the bottom palette onto the preview bar, then drag within the preview to reorder. The FairWindSK button stays fixed on the left and the current-application button stays fixed on the right. Use Width and Height to decide which middle items should grow to fill available space."),
            this);
        m_hintLabel->setWordWrap(true);
        rootLayout->addWidget(m_hintLabel);

        m_previewLabel = new QLabel(tr("Preview"), this);
        rootLayout->addWidget(m_previewLabel);

        m_previewWidget = new PreviewListWidget(this);
        m_previewWidget->setMinimumHeight(124);
        m_previewWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::LeftMouseButtonGesture);
        m_previewWidget->setToolTip(tr("Tap a widget to edit it. Drag within the preview to reorder the top bar."));
        rootLayout->addWidget(m_previewWidget);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);

        m_expandWidthButton = new QToolButton(this);
        m_expandHeightButton = new QToolButton(this);
        m_removeSelectedButton = new QToolButton(this);
        m_resetDefaultsButton = new QToolButton(this);

        auto configureActionButton = [kControlHeight](QToolButton *button,
                                                      const QString &iconPath,
                                                      const QString &toolTip,
                                                      const bool checkable = false) {
            if (!button) {
                return;
            }

            button->setCheckable(checkable);
            button->setAutoRaise(true);
            button->setIcon(QIcon(iconPath));
            button->setIconSize(QSize(24, 24));
            button->setToolTip(toolTip);
            button->setStatusTip(toolTip);
            button->setAccessibleName(toolTip);
            button->setMinimumSize(QSize(kControlHeight, kControlHeight));
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        };

        configureActionButton(m_expandWidthButton,
                              QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg"),
                              tr("Extend Width"),
                              true);
        configureActionButton(m_expandHeightButton,
                              QStringLiteral(":/resources/svg/OpenBridge/arrow-down-google.svg"),
                              tr("Extend Height"),
                              true);
        configureActionButton(m_removeSelectedButton,
                              QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg"),
                              tr("Remove Selected"));
        configureActionButton(m_resetDefaultsButton,
                              QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg"),
                              tr("Reset Defaults"));

        m_expandWidthButton->setCheckable(true);
        m_expandHeightButton->setCheckable(true);
        controlsLayout->addWidget(m_expandWidthButton);
        controlsLayout->addWidget(m_expandHeightButton);
        controlsLayout->addWidget(m_removeSelectedButton);
        controlsLayout->addWidget(m_resetDefaultsButton);
        controlsLayout->addStretch(1);
        rootLayout->addLayout(controlsLayout);

        m_paletteLabel = new QLabel(tr("Widget Palette"), this);
        rootLayout->addWidget(m_paletteLabel);

        m_paletteWidget = new PaletteListWidget(this);
        m_paletteWidget->setMinimumHeight(124);
        m_paletteWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_paletteWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_paletteWidget->viewport(), QScroller::LeftMouseButtonGesture);
        m_paletteWidget->setToolTip(tr("Tap or drag a palette item to place it on the preview bar."));
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

        connect(m_paletteWidget, &QListWidget::itemClicked, this, &TopBar::onPaletteItemClicked);
        connect(m_previewWidget, &QListWidget::itemSelectionChanged, this, &TopBar::onPreviewSelectionChanged);
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsInserted, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsRemoved, this, [this]() { onPreviewEdited(); });
        connect(m_previewWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() { onPreviewEdited(); });
        connect(m_expandWidthButton, &QToolButton::toggled, this, &TopBar::onExpandWidthToggled);
        connect(m_expandHeightButton, &QToolButton::toggled, this, &TopBar::onExpandHeightToggled);
        connect(m_removeSelectedButton, &QToolButton::clicked, this, &TopBar::onRemoveSelected);
        connect(m_resetDefaultsButton, &QToolButton::clicked, this, &TopBar::onResetDefaults);

        applyChrome();
        updateInspector();
    }

    void TopBar::applyChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration() : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 18.0);
        applySecondaryLabelStyle(m_previewLabel, chrome);
        applySecondaryLabelStyle(m_paletteLabel, chrome);

        if (m_hintLabel) {
            QPalette hintPalette = m_hintLabel->palette();
            hintPalette.setColor(QPalette::WindowText, chrome.text);
            hintPalette.setColor(QPalette::Text, chrome.text);
            m_hintLabel->setPalette(hintPalette);
        }
        if (m_previewWidget) {
            m_previewWidget->setStyleSheet(listWidgetChrome(chrome, false));
        }
        if (m_paletteWidget) {
            m_paletteWidget->setStyleSheet(listWidgetChrome(chrome, true));
        }

        fairwindsk::ui::applyBottomBarToolButtonChrome(m_expandWidthButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_expandHeightButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_removeSelectedButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_resetDefaultsButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
    }

    fairwindsk::ui::layout::LayoutEntry TopBar::entryFromPaletteItem(const QListWidgetItem *item) const {
        LayoutEntry entry;
        if (!item) {
            return entry;
        }

        entry.kind = static_cast<EntryKind>(item->data(RolePaletteKind).toInt());
        entry.widgetId = item->data(RolePaletteWidgetId).toString();
        entry.instanceId = entry.kind == EntryKind::Widget
                               ? entry.widgetId
                               : QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.enabled = true;
        entry.expandHorizontally = entry.kind == EntryKind::Stretch;
        return entry;
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
    }

    void TopBar::refreshPreviewItem(QListWidgetItem *item) const {
        if (!item) {
            return;
        }

        const auto entry = entryForPreviewItem(item);
        item->setText(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), itemSummary(entry)));
        item->setSizeHint(itemSizeHint(entry));
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
        QString detail = tr("Tap or drag to add");
        if (entry.kind == EntryKind::Stretch) {
            detail = tr("Elastic spacer");
        } else if (entry.kind == EntryKind::Separator) {
            detail = tr("Visual divider");
        }

        auto *item = new QListWidgetItem(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), detail));
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
            if (kind == EntryKind::Separator) {
                item->setToolTip(tr("Tap or drag to add a separator to the preview bar."));
                continue;
            }
            if (kind == EntryKind::Stretch) {
                item->setToolTip(tr("Tap or drag to add an elastic extender to the preview bar."));
                continue;
            }
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

    void TopBar::onPaletteItemClicked(QListWidgetItem *item) {
        if (!item) {
            return;
        }

        appendPaletteEntryToPreview(entryFromPaletteItem(item));
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
        refreshPreviewItem(item);
        persistToConfiguration();
    }

    void TopBar::onExpandHeightToggled(const bool checked) {
        auto *item = m_previewWidget ? m_previewWidget->currentItem() : nullptr;
        if (!item) {
            return;
        }

        item->setData(RoleExpandVertically, checked);
        refreshPreviewItem(item);
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
            refreshPreviewItem(item);
        }

        persistToConfiguration();
        updateInspector();
    }
}
