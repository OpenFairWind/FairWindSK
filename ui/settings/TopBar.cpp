#include "TopBar.hpp"

#include <algorithm>
#include <QAbstractItemModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
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

        QString previewFrameChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
            return QStringLiteral(
                "QFrame {"
                " border: 1px solid %1;"
                " border-radius: 14px;"
                " background: %2;"
                " }")
                .arg(colors.border.name(),
                     fairwindsk::ui::comfortAlpha(colors.buttonBackground, 18).name(QColor::HexArgb));
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

        m_previewFrame = new QFrame(this);
        auto *previewFrameLayout = new QHBoxLayout(m_previewFrame);
        previewFrameLayout->setContentsMargins(8, 8, 8, 8);
        previewFrameLayout->setSpacing(8);

        m_leftShellButton = new QToolButton(m_previewFrame);
        m_rightShellButton = new QToolButton(m_previewFrame);
        for (auto *button : {m_leftShellButton, m_rightShellButton}) {
            button->setAutoRaise(true);
            button->setIconSize(QSize(28, 28));
            button->setMinimumSize(QSize(56, 56));
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setEnabled(true);
            button->setFocusPolicy(Qt::NoFocus);
        }
        m_leftShellButton->setToolTip(tr("Fixed FairWindSK shell button"));
        m_rightShellButton->setToolTip(tr("Fixed current-application shell button"));

        m_previewWidget = new PreviewListWidget(m_previewFrame);
        m_previewWidget->setMinimumHeight(124);
        m_previewWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_previewWidget->viewport(), QScroller::LeftMouseButtonGesture);
        m_previewWidget->setToolTip(tr("Tap a widget to edit it. Drag within the preview to reorder the top bar."));
        previewFrameLayout->addWidget(m_leftShellButton, 0, Qt::AlignVCenter);
        previewFrameLayout->addWidget(m_previewWidget, 1);
        previewFrameLayout->addWidget(m_rightShellButton, 0, Qt::AlignVCenter);
        rootLayout->addWidget(m_previewFrame);

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
            paletteEntries.append(entry);
        }
        m_paletteWidget->setEntries(paletteEntries);

        connect(m_paletteWidget, &WidgetPalette::entryActivated, this, &TopBar::onPaletteEntryActivated);
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
        if (m_previewFrame) {
            m_previewFrame->setStyleSheet(previewFrameChrome(chrome));
        }
        if (m_paletteWidget) {
            m_paletteWidget->setStyleSheet(QStringLiteral(
                "QScrollArea { border: 1px solid %1; border-radius: 12px; background: %2; }"
                "QWidget { background: transparent; }")
                                               .arg(chrome.border.name(),
                                                    chrome.window.name()));
        }

        if (m_leftShellButton) {
            m_leftShellButton->setIcon(QIcon(QStringLiteral(":/resources/images/mainwindow/fairwind_icon.png")));
            fairwindsk::ui::applyBottomBarToolButtonChrome(m_leftShellButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(28, 28), 56);
        }
        if (m_rightShellButton) {
            m_rightShellButton->setIcon(QIcon(QStringLiteral(":/resources/images/icons/apps_icon.png")));
            fairwindsk::ui::applyBottomBarToolButtonChrome(m_rightShellButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(28, 28), 56);
        }

        fairwindsk::ui::applyBottomBarToolButtonChrome(m_expandWidthButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_expandHeightButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_removeSelectedButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_resetDefaultsButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
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
        item->setIcon(WidgetPalette::entryIcon(entry));
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
        item->setText(QStringLiteral("%1\n%2")
                          .arg(layout::entryLabel(entry), itemSummary(entry)));
        item->setSizeHint(itemSizeHint(entry));
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
