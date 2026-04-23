#include "BarLayoutSettings.hpp"

#include <QAbstractItemModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QLabel>
#include <QListWidgetItem>
#include <QMimeData>
#include <QScroller>
#include <QScrollerProperties>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::settings {
    using fairwindsk::ui::layout::BarId;
    using fairwindsk::ui::layout::EntryKind;
    using fairwindsk::ui::layout::LayoutEntry;

    namespace {
        QString previewListChrome(const fairwindsk::ui::ComfortChromeColors &colors) {
            return QStringLiteral(
                "QListWidget { border: 1px solid %1; border-radius: 12px; background: %2; padding: 8px; }"
                "QListWidget::item { margin: 4px; padding: 10px; border: 1px solid %3; border-radius: 10px; color: %4; }"
                "QListWidget::item:selected { border-color: %5; background: %6; font-weight: 600; }")
                .arg(colors.border.name(),
                     fairwindsk::ui::comfortAlpha(colors.buttonBackground, 18).name(QColor::HexArgb),
                     colors.border.name(),
                     colors.text.name(),
                     colors.accentTop.name(),
                     fairwindsk::ui::comfortAlpha(colors.accentTop, 42).name(QColor::HexArgb));
        }

        class PaletteAwareListWidget final : public QListWidget {
        public:
            explicit PaletteAwareListWidget(QWidget *parent = nullptr)
                : QListWidget(parent) {
                setViewMode(QListView::IconMode);
                setFlow(QListView::LeftToRight);
                setWrapping(false);
                setResizeMode(QListView::Adjust);
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
                newItem->setFlags(Qt::ItemIsEnabled |
                                  Qt::ItemIsSelectable |
                                  Qt::ItemIsDragEnabled);
                newItem->setIcon(WidgetPalette::entryIcon(entry));
                newItem->setSizeHint(QSize(108, 84));
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
        rootLayout->setContentsMargins(12, 12, 12, 12);
        rootLayout->setSpacing(12);

        m_titleLabel = new QLabel(layout::barLabel(m_barId), this);
        rootLayout->addWidget(m_titleLabel);

        m_hintLabel = new QLabel(
            tr("Tap or drag palette items to place them on this bar. Drag rows to reorder them. Add separators and elastic extenders to build clear MFD-style groups."),
            this);
        m_hintLabel->setWordWrap(true);
        rootLayout->addWidget(m_hintLabel);

        m_previewLabel = new QLabel(tr("Preview"), this);
        rootLayout->addWidget(m_previewLabel);

        m_previewFrame = new QFrame(this);
        auto *previewLayout = new QVBoxLayout(m_previewFrame);
        previewLayout->setContentsMargins(8, 8, 8, 8);
        previewLayout->setSpacing(8);

        auto *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(8);
        previewLayout->addLayout(controlsLayout);

        auto configureActionButton = [kControlHeight](QToolButton *button,
                                                      const QString &iconPath,
                                                      const QString &toolTip) {
            if (!button) {
                return;
            }

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

        m_removeSelectedButton = new QToolButton(this);
        m_resetDefaultsButton = new QToolButton(this);
        configureActionButton(m_removeSelectedButton,
                              QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg"),
                              tr("Remove Selected"));
        configureActionButton(m_resetDefaultsButton,
                              QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg"),
                              tr("Reset Defaults"));
        controlsLayout->addWidget(m_removeSelectedButton);
        controlsLayout->addWidget(m_resetDefaultsButton);
        controlsLayout->addStretch(1);

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
        m_listWidget->setSpacing(8);
        m_listWidget->setUniformItemSizes(false);
        m_listWidget->setMinimumHeight(128);
        m_listWidget->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(m_listWidget->viewport(), QScroller::LeftMouseButtonGesture);
        auto scrollerProperties = QScroller::scroller(m_listWidget->viewport())->scrollerProperties();
        scrollerProperties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        scrollerProperties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
        scrollerProperties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.55);
        QScroller::scroller(m_listWidget->viewport())->setScrollerProperties(scrollerProperties);
        m_listWidget->setToolTip(tr("Drag inside the preview to reorder the bar."));
        previewLayout->addWidget(m_listWidget, 1);
        rootLayout->addWidget(m_previewFrame);

        auto *paletteLabel = new QLabel(tr("Widget Palette"), this);
        rootLayout->addWidget(paletteLabel);

        m_paletteWidget = new WidgetPalette(this);
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

        connect(m_listWidget, &QListWidget::itemChanged, this, &BarLayoutSettings::onItemChanged);
        connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &BarLayoutSettings::onSelectionChanged);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
            if (!m_populating) {
                persistToConfiguration();
            }
        });
        connect(m_paletteWidget, &WidgetPalette::entryActivated, this, &BarLayoutSettings::onPaletteEntryActivated);
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
        if (m_previewFrame) {
            m_previewFrame->setStyleSheet(QStringLiteral(
                "QFrame { border: 1px solid %1; border-radius: 14px; background: %2; }")
                                              .arg(chrome.border.name(),
                                                   fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 18).name(QColor::HexArgb)));
        }
        if (m_listWidget) {
            m_listWidget->setStyleSheet(previewListChrome(chrome));
        }

        fairwindsk::ui::applyBottomBarToolButtonChrome(m_removeSelectedButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
        fairwindsk::ui::applyBottomBarToolButtonChrome(m_resetDefaultsButton, chrome, fairwindsk::ui::BottomBarButtonChrome::Flat, QSize(24, 24), 52);
    }

    QListWidgetItem *BarLayoutSettings::createItem(const LayoutEntry &entry) {
        constexpr int kRowHeight = 84;

        auto *item = new QListWidgetItem(layout::entryLabel(entry));
        item->setIcon(WidgetPalette::entryIcon(entry));
        item->setData(RoleKind, static_cast<int>(entry.kind));
        item->setData(RoleWidgetId, entry.widgetId);
        item->setData(RoleInstanceId, entry.instanceId);
        item->setFlags(Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable |
                       Qt::ItemIsDragEnabled);
        item->setSizeHint(QSize(108, kRowHeight));
        refreshPreviewItem(item);
        return item;
    }

    void BarLayoutSettings::refreshPreviewItem(QListWidgetItem *item) const {
        if (!item) {
            return;
        }

        const auto entry = entryForItem(item);
        const QString stateText = entry.kind == EntryKind::Stretch
                                      ? tr("Elastic")
                                      : entry.kind == EntryKind::Separator
                                            ? tr("Divider")
                                            : tr("Active");
        item->setText(QStringLiteral("%1\n%2").arg(layout::entryLabel(entry), stateText));
        item->setIcon(WidgetPalette::entryIcon(entry));
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
        m_listWidget->addItem(createItem(entry));
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
        persistToConfiguration();
        updateActions();
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
}
