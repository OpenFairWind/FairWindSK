#include "DataWidgets.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "ui/DrawerDialogHost.hpp"
#include "FairWindSK.hpp"
#include "BarSettingsUi.hpp"
#include "ui/IconUtils.hpp"
#include "ui/layout/BarLayout.hpp"
#include "ui/widgets/TouchIconBrowser.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr int kControlHeight = 52;
        constexpr int kIconPreviewSize = 58;

        void setControlHeight(QWidget *widget) {
            if (widget) {
                widget->setMinimumHeight(kControlHeight);
            }
        }
    }

    DataWidgets::DataWidgets(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
        buildUi();
        populateFromConfiguration();
    }

    void DataWidgets::refreshFromConfiguration() {
        populateFromConfiguration(currentWidgetId());
    }

    void DataWidgets::buildUi() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(8);

        auto *toolbarLayout = new QHBoxLayout();
        toolbarLayout->setContentsMargins(0, 0, 0, 0);
        toolbarLayout->setSpacing(8);
        m_addButton = new QPushButton(tr("Add Widget"), this);
        m_removeButton = new QPushButton(tr("Remove Widget"), this);
        toolbarLayout->addWidget(m_addButton);
        toolbarLayout->addWidget(m_removeButton);
        toolbarLayout->addStretch(1);
        rootLayout->addLayout(toolbarLayout);

        auto *splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setChildrenCollapsible(false);
        rootLayout->addWidget(splitter, 1);

        m_listWidget = new QListWidget(splitter);
        m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setIconSize(QSize(40, 40));
        m_listWidget->setMinimumWidth(250);
        m_listWidget->setAlternatingRowColors(false);

        auto *editorScrollArea = new QScrollArea(splitter);
        editorScrollArea->setWidgetResizable(true);
        editorScrollArea->setFrameShape(QFrame::NoFrame);
        editorScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editorScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto *editorWidget = new QWidget(editorScrollArea);
        auto *editorLayout = new QVBoxLayout(editorWidget);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(10);

        auto *identityLayout = new QHBoxLayout();
        identityLayout->setContentsMargins(0, 0, 0, 0);
        identityLayout->setSpacing(10);
        m_iconPreview = new QLabel(editorWidget);
        m_iconPreview->setFixedSize(QSize(kIconPreviewSize, kIconPreviewSize));
        m_iconPreview->setAlignment(Qt::AlignCenter);
        identityLayout->addWidget(m_iconPreview, 0, Qt::AlignTop);

        auto *identityFieldsLayout = new QFormLayout();
        identityFieldsLayout->setContentsMargins(0, 0, 0, 0);
        identityFieldsLayout->setSpacing(8);
        m_idValueLabel = new QLabel(editorWidget);
        m_idValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_nameEdit = new QLineEdit(editorWidget);
        m_nameEdit->setPlaceholderText(tr("Widget name"));
        identityFieldsLayout->addRow(tr("ID"), m_idValueLabel);
        identityFieldsLayout->addRow(tr("Name"), m_nameEdit);
        identityLayout->addLayout(identityFieldsLayout, 1);
        editorLayout->addLayout(identityLayout);

        auto *formLayout = new QFormLayout();
        formLayout->setContentsMargins(0, 0, 0, 0);
        formLayout->setSpacing(8);

        auto *iconPathLayout = new QHBoxLayout();
        iconPathLayout->setContentsMargins(0, 0, 0, 0);
        iconPathLayout->setSpacing(8);
        m_iconEdit = new QLineEdit(editorWidget);
        m_iconEdit->setPlaceholderText(tr("Icon resource path"));
        m_chooseIconButton = new QToolButton(editorWidget);
        m_chooseIconButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/edit-google.svg")));
        m_chooseIconButton->setToolTip(tr("Choose icon"));
        m_chooseIconButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_chooseIconButton->setMinimumSize(QSize(kControlHeight, kControlHeight));
        iconPathLayout->addWidget(m_iconEdit, 1);
        iconPathLayout->addWidget(m_chooseIconButton, 0);
        formLayout->addRow(tr("Icon"), iconPathLayout);

        m_typeComboBox = new QComboBox(editorWidget);
        for (const auto kind : {widgets::DataWidgetKind::Numeric,
                                widgets::DataWidgetKind::Gauge,
                                widgets::DataWidgetKind::Position,
                                widgets::DataWidgetKind::DateTime,
                                widgets::DataWidgetKind::Waypoint}) {
            m_typeComboBox->addItem(widgets::dataWidgetKindLabel(kind), widgets::dataWidgetKindId(kind));
        }
        formLayout->addRow(tr("Type"), m_typeComboBox);

        m_signalKPathEdit = new QLineEdit(editorWidget);
        m_signalKPathEdit->setPlaceholderText(tr("Signal K path"));
        formLayout->addRow(tr("Signal K Path"), m_signalKPathEdit);

        m_sourceUnitEdit = new QLineEdit(editorWidget);
        m_sourceUnitEdit->setPlaceholderText(tr("Source unit, for example rad or ms-1"));
        formLayout->addRow(tr("Source Unit"), m_sourceUnitEdit);

        m_defaultUnitEdit = new QLineEdit(editorWidget);
        m_defaultUnitEdit->setPlaceholderText(tr("Default display unit, for example deg or kn"));
        formLayout->addRow(tr("Default Unit"), m_defaultUnitEdit);

        m_updatePolicyComboBox = new QComboBox(editorWidget);
        m_updatePolicyComboBox->addItem(tr("Ideal"), QStringLiteral("ideal"));
        m_updatePolicyComboBox->addItem(tr("Fixed"), QStringLiteral("fixed"));
        m_updatePolicyComboBox->addItem(tr("Instant"), QStringLiteral("instant"));
        formLayout->addRow(tr("Update Policy"), m_updatePolicyComboBox);

        m_periodSpinBox = new QSpinBox(editorWidget);
        m_periodSpinBox->setRange(100, 600000);
        m_periodSpinBox->setSingleStep(100);
        m_periodSpinBox->setSuffix(tr(" ms"));
        formLayout->addRow(tr("Period"), m_periodSpinBox);

        m_minPeriodSpinBox = new QSpinBox(editorWidget);
        m_minPeriodSpinBox->setRange(0, 600000);
        m_minPeriodSpinBox->setSingleStep(100);
        m_minPeriodSpinBox->setSuffix(tr(" ms"));
        formLayout->addRow(tr("Minimum Period"), m_minPeriodSpinBox);

        m_minimumSpinBox = new QDoubleSpinBox(editorWidget);
        m_minimumSpinBox->setRange(-1000000000.0, 1000000000.0);
        m_minimumSpinBox->setDecimals(3);
        formLayout->addRow(tr("Gauge Minimum"), m_minimumSpinBox);

        m_maximumSpinBox = new QDoubleSpinBox(editorWidget);
        m_maximumSpinBox->setRange(-1000000000.0, 1000000000.0);
        m_maximumSpinBox->setDecimals(3);
        formLayout->addRow(tr("Gauge Maximum"), m_maximumSpinBox);

        m_dateTimeFormatEdit = new QLineEdit(editorWidget);
        m_dateTimeFormatEdit->setPlaceholderText(tr("Date/time format"));
        formLayout->addRow(tr("Date Format"), m_dateTimeFormatEdit);

        editorLayout->addLayout(formLayout);
        editorLayout->addStretch(1);
        editorScrollArea->setWidget(editorWidget);

        splitter->addWidget(m_listWidget);
        splitter->addWidget(editorScrollArea);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes({280, 720});

        const QList<QWidget *> controls = {
            m_nameEdit,
            m_iconEdit,
            m_typeComboBox,
            m_signalKPathEdit,
            m_sourceUnitEdit,
            m_defaultUnitEdit,
            m_updatePolicyComboBox,
            m_periodSpinBox,
            m_minPeriodSpinBox,
            m_minimumSpinBox,
            m_maximumSpinBox,
            m_dateTimeFormatEdit
        };
        for (auto *widget : controls) {
            setControlHeight(widget);
        }

        connect(m_listWidget, &QListWidget::currentItemChanged, this, &DataWidgets::onSelectionChanged);
        connect(m_addButton, &QPushButton::clicked, this, &DataWidgets::onAddWidget);
        connect(m_removeButton, &QPushButton::clicked, this, &DataWidgets::onRemoveWidget);
        connect(m_chooseIconButton, &QToolButton::clicked, this, &DataWidgets::onChooseIcon);

        connect(m_nameEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);
        connect(m_iconEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);
        connect(m_typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataWidgets::onEditorChanged);
        connect(m_signalKPathEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);
        connect(m_sourceUnitEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);
        connect(m_defaultUnitEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);
        connect(m_updatePolicyComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DataWidgets::onEditorChanged);
        connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataWidgets::onEditorChanged);
        connect(m_minPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataWidgets::onEditorChanged);
        connect(m_minimumSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DataWidgets::onEditorChanged);
        connect(m_maximumSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DataWidgets::onEditorChanged);
        connect(m_dateTimeFormatEdit, &QLineEdit::editingFinished, this, &DataWidgets::onEditorChanged);

        applyChrome();
        setEditorEnabled(false);
    }

    void DataWidgets::applyChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration() : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        if (m_listWidget) {
            m_listWidget->setStyleSheet(barsettings::listWidgetChrome(chrome, false));
        }
        if (m_iconPreview) {
            m_iconPreview->setStyleSheet(QStringLiteral(
                "QLabel {"
                " border: 1px solid %1;"
                " border-radius: 8px;"
                " background: %2;"
                " color: %3;"
                " }").arg(chrome.border.name(),
                           fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 24).name(QColor::HexArgb),
                           chrome.text.name()));
        }
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_addButton, chrome, true, kControlHeight);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_removeButton, chrome, false, kControlHeight);
        fairwindsk::ui::applyBottomBarToolButtonChrome(
            m_chooseIconButton,
            chrome,
            fairwindsk::ui::BottomBarButtonChrome::Accent,
            QSize(24, 24),
            kControlHeight);
    }

    void DataWidgets::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event && (event->type() == QEvent::PaletteChange ||
                      event->type() == QEvent::ApplicationPaletteChange ||
                      event->type() == QEvent::StyleChange)) {
            applyChrome();
            updateIconPreview();
        }
    }

    void DataWidgets::populateFromConfiguration(const QString &selectId) {
        if (!m_settings || !m_listWidget) {
            return;
        }

        m_populating = true;
        const QSignalBlocker blocker(m_listWidget);
        m_listWidget->clear();

        const auto definitions = widgets::dataWidgetDefinitions(m_settings->getConfiguration()->getRoot());
        for (const auto &definition : definitions) {
            auto *item = new QListWidgetItem(QIcon(definition.icon), definition.name, m_listWidget);
            item->setData(Qt::UserRole, definition.id);
            item->setToolTip(definition.signalKPath.trimmed().isEmpty()
                                 ? definition.name
                                 : QStringLiteral("%1\n%2").arg(definition.name, definition.signalKPath));
            item->setSizeHint(QSize(220, 58));
            m_listWidget->addItem(item);
        }

        m_populating = false;

        QListWidgetItem *itemToSelect = itemForId(selectId);
        if (!itemToSelect && m_listWidget->count() > 0) {
            itemToSelect = m_listWidget->item(0);
        }

        if (itemToSelect) {
            m_listWidget->setCurrentItem(itemToSelect);
        } else {
            setEditorEnabled(false);
            setEditorFromDefinition({});
        }
        applyChrome();
    }

    void DataWidgets::setEditorEnabled(const bool enabled) {
        const QList<QWidget *> controls = {
            m_idValueLabel,
            m_nameEdit,
            m_iconEdit,
            m_chooseIconButton,
            m_typeComboBox,
            m_signalKPathEdit,
            m_sourceUnitEdit,
            m_defaultUnitEdit,
            m_updatePolicyComboBox,
            m_periodSpinBox,
            m_minPeriodSpinBox,
            m_minimumSpinBox,
            m_maximumSpinBox,
            m_dateTimeFormatEdit
        };
        for (auto *widget : controls) {
            if (widget) {
                widget->setEnabled(enabled);
            }
        }
        if (m_removeButton) {
            m_removeButton->setEnabled(enabled);
        }
    }

    void DataWidgets::setEditorFromDefinition(const widgets::DataWidgetDefinition &definition) {
        m_populating = true;

        const QSignalBlocker nameBlocker(m_nameEdit);
        const QSignalBlocker iconBlocker(m_iconEdit);
        const QSignalBlocker typeBlocker(m_typeComboBox);
        const QSignalBlocker pathBlocker(m_signalKPathEdit);
        const QSignalBlocker sourceUnitBlocker(m_sourceUnitEdit);
        const QSignalBlocker defaultUnitBlocker(m_defaultUnitEdit);
        const QSignalBlocker policyBlocker(m_updatePolicyComboBox);
        const QSignalBlocker periodBlocker(m_periodSpinBox);
        const QSignalBlocker minPeriodBlocker(m_minPeriodSpinBox);
        const QSignalBlocker minimumBlocker(m_minimumSpinBox);
        const QSignalBlocker maximumBlocker(m_maximumSpinBox);
        const QSignalBlocker dateBlocker(m_dateTimeFormatEdit);

        m_idValueLabel->setText(definition.id);
        m_nameEdit->setText(definition.name);
        m_iconEdit->setText(definition.icon);
        const int typeIndex = m_typeComboBox->findData(widgets::dataWidgetKindId(definition.kind));
        m_typeComboBox->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
        m_signalKPathEdit->setText(definition.signalKPath);
        m_sourceUnitEdit->setText(definition.sourceUnit);
        m_defaultUnitEdit->setText(definition.defaultUnit);
        const int policyIndex = m_updatePolicyComboBox->findData(definition.updatePolicy);
        m_updatePolicyComboBox->setCurrentIndex(policyIndex >= 0 ? policyIndex : 0);
        m_periodSpinBox->setValue(definition.period);
        m_minPeriodSpinBox->setValue(definition.minPeriod);
        m_minimumSpinBox->setValue(definition.minimum);
        m_maximumSpinBox->setValue(definition.maximum);
        m_dateTimeFormatEdit->setText(definition.dateTimeFormat);

        m_populating = false;
        setEditorEnabled(!definition.id.isEmpty());
        updateIconPreview();
    }

    widgets::DataWidgetDefinition DataWidgets::editorDefinition() const {
        widgets::DataWidgetDefinition definition = definitionForId(currentWidgetId());
        if (definition.id.trimmed().isEmpty()) {
            return definition;
        }

        definition.name = m_nameEdit->text().trimmed();
        if (definition.name.isEmpty()) {
            definition.name = definition.id;
        }
        definition.icon = widgets::TouchIconBrowser::normalizedIconStoragePath(m_iconEdit->text().trimmed());
        definition.kind = widgets::dataWidgetKindFromId(m_typeComboBox->currentData().toString());
        definition.signalKPath = m_signalKPathEdit->text().trimmed();
        definition.sourceUnit = m_sourceUnitEdit->text().trimmed();
        definition.defaultUnit = m_defaultUnitEdit->text().trimmed();
        definition.updatePolicy = m_updatePolicyComboBox->currentData().toString();
        definition.period = m_periodSpinBox->value();
        definition.minPeriod = m_minPeriodSpinBox->value();
        definition.minimum = m_minimumSpinBox->value();
        definition.maximum = m_maximumSpinBox->value();
        if (definition.maximum <= definition.minimum) {
            definition.maximum = definition.minimum + 1.0;
        }
        definition.dateTimeFormat = m_dateTimeFormatEdit->text().trimmed();
        return definition;
    }

    widgets::DataWidgetDefinition DataWidgets::definitionForId(const QString &id) const {
        if (!m_settings || id.trimmed().isEmpty()) {
            return {};
        }
        return widgets::dataWidgetDefinition(m_settings->getConfiguration()->getRoot(), id);
    }

    QString DataWidgets::currentWidgetId() const {
        const auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        return item ? item->data(Qt::UserRole).toString() : QString();
    }

    QListWidgetItem *DataWidgets::itemForId(const QString &id) const {
        if (!m_listWidget || id.trimmed().isEmpty()) {
            return nullptr;
        }
        for (int row = 0; row < m_listWidget->count(); ++row) {
            auto *item = m_listWidget->item(row);
            if (item && item->data(Qt::UserRole).toString() == id) {
                return item;
            }
        }
        return nullptr;
    }

    void DataWidgets::persistEditor() {
        if (m_populating || !m_settings) {
            return;
        }

        auto definition = editorDefinition();
        if (definition.id.trimmed().isEmpty()) {
            return;
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        widgets::upsertDataWidgetDefinition(root, definition);

        if (auto *item = itemForId(definition.id)) {
            item->setText(definition.name);
            item->setIcon(QIcon(definition.icon));
            item->setToolTip(definition.signalKPath.trimmed().isEmpty()
                                 ? definition.name
                                 : QStringLiteral("%1\n%2").arg(definition.name, definition.signalKPath));
        }
        updateIconPreview();
        m_settings->markDirty(FairWindSK::RuntimeUi | FairWindSK::RuntimeSignalKPaths, 0);
    }

    void DataWidgets::updateIconPreview() {
        if (!m_iconPreview) {
            return;
        }

        const QString iconPath = m_iconEdit ? m_iconEdit->text().trimmed() : QString();
        const QPixmap pixmap = widgets::TouchIconBrowser::iconPixmapForPath(iconPath, kIconPreviewSize - 14);
        if (pixmap.isNull()) {
            m_iconPreview->setText(tr("Icon"));
            m_iconPreview->setPixmap(QPixmap());
        } else {
            m_iconPreview->setText(QString());
            m_iconPreview->setPixmap(pixmap);
        }
    }

    void DataWidgets::onSelectionChanged() {
        if (m_populating) {
            return;
        }

        setEditorFromDefinition(definitionForId(currentWidgetId()));
    }

    void DataWidgets::onAddWidget() {
        if (!m_settings) {
            return;
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        widgets::DataWidgetDefinition definition;
        definition.name = tr("New Data Widget");
        definition.id = widgets::uniqueDataWidgetId(root, definition.name);
        definition.icon = QStringLiteral(":/resources/svg/OpenBridge/lcd-sog.svg");
        definition.kind = widgets::DataWidgetKind::Numeric;
        definition.updatePolicy = QStringLiteral("ideal");
        definition.period = 1000;
        definition.minPeriod = 200;
        definition.minimum = 0.0;
        definition.maximum = 100.0;
        widgets::upsertDataWidgetDefinition(root, definition);
        m_settings->markDirty(FairWindSK::RuntimeUi | FairWindSK::RuntimeSignalKPaths, 0);
        populateFromConfiguration(definition.id);
    }

    void DataWidgets::onRemoveWidget() {
        if (!m_settings) {
            return;
        }

        const QString id = currentWidgetId();
        if (id.trimmed().isEmpty()) {
            return;
        }

        const auto definition = definitionForId(id);
        const auto response = drawer::question(
            this,
            tr("Remove Data Widget"),
            tr("Remove %1 from data widgets and bar layouts?").arg(definition.name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (response != QMessageBox::Yes) {
            return;
        }

        auto &root = m_settings->getConfiguration()->getRoot();
        widgets::removeDataWidgetDefinition(root, id);
        layout::removeWidgetFromBar(root, layout::BarId::Top, id);
        layout::removeWidgetFromBar(root, layout::BarId::Bottom, id);
        m_settings->markDirty(FairWindSK::RuntimeUi | FairWindSK::RuntimeSignalKPaths, 0);
        populateFromConfiguration();
    }

    void DataWidgets::onChooseIcon() {
        if (!m_iconEdit || !m_iconEdit->isEnabled()) {
            return;
        }

        const QString iconPath = drawer::getIconPath(this, tr("Choose Data Widget Icon"), m_iconEdit->text().trimmed());
        if (iconPath.trimmed().isEmpty()) {
            return;
        }

        m_iconEdit->setText(iconPath);
        persistEditor();
    }

    void DataWidgets::onEditorChanged() {
        persistEditor();
    }
}
