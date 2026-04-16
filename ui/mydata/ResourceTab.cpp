//
// Created by Codex on 21/03/26.
//

#include "ResourceTab.hpp"

#include <QBrush>
#include <QCheckBox>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSizePolicy>
#include <QUuid>

#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"
#include "JsonObjectEditorWidget.hpp"
#include "FairWindSK.hpp"
#include "signalk/Client.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui_ResourceTab.h"

namespace {
    QJsonObject featureObject(const QJsonObject &resource) {
        return resource["feature"].toObject();
    }

    QJsonObject geometryObject(const QJsonObject &resource) {
        return featureObject(resource)["geometry"].toObject();
    }

    QJsonObject featurePropertiesObject(const QJsonObject &resource) {
        return featureObject(resource)["properties"].toObject();
    }

    QJsonArray coordinateArray(const QJsonObject &resource) {
        return geometryObject(resource)["coordinates"].toArray();
    }
}

namespace fairwindsk::ui::mydata {

    ResourceTab::ResourceTab(const ResourceKind kind, QWidget *parent)
        : QWidget(parent),
          ui(new ::Ui::ResourceTab),
          m_kind(kind),
          m_model(new ResourceModel(kind, this)),
          m_nameEdit(new QLineEdit(this)),
          m_descriptionEdit(new QLineEdit(this)),
          m_typeEdit(new QLineEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
          m_coordinateRowWidget(new QWidget(this)),
          m_coordinateDisplayEdit(new QLineEdit(this)),
          m_coordinateEditButton(new QToolButton(this)),
          m_coordinatesEdit(new QPlainTextEdit(this)),
          m_geometryEdit(new QPlainTextEdit(this)),
          m_propertiesEditor(new JsonObjectEditorWidget(this)),
          m_previewWidget(new GeoJsonPreviewWidget(this)),
          m_hrefEdit(new QLineEdit(this)),
          m_mimeTypeEdit(new QLineEdit(this)),
          m_notePositionCheckBox(new QCheckBox(tr("Include position"), this)),
          m_identifierEdit(new QLineEdit(this)),
          m_chartFormatEdit(new QLineEdit(this)),
          m_chartUrlEdit(new QLineEdit(this)),
          m_tilemapUrlEdit(new QLineEdit(this)),
          m_chartRegionEdit(new QLineEdit(this)),
          m_chartScaleSpinBox(new QDoubleSpinBox(this)),
          m_chartLayersEdit(new QLineEdit(this)),
          m_chartBoundsEdit(new QPlainTextEdit(this)) {
        ui->setupUi(this);

        m_stackedWidget = ui->stackedWidget;
        m_listPage = ui->pageList;
        m_detailsPage = ui->pageDetails;
        m_searchEdit = ui->lineEditSearch;
        m_tableWidget = ui->tableWidget;
        m_addButton = ui->toolButtonAdd;
        m_backButton = ui->toolButtonBack;
        m_newButton = ui->toolButtonNew;
        m_editButton = ui->toolButtonEdit;
        m_saveButton = ui->toolButtonSave;
        m_cancelButton = ui->toolButtonCancel;
        m_deleteButton = ui->toolButtonDelete;
        m_importButton = ui->toolButtonImport;
        m_exportButton = ui->toolButtonExport;
        m_refreshButton = ui->toolButtonRefresh;
        m_titleLabel = ui->labelTitle;
        m_idValueLabel = new QLabel(this);
        m_timestampValueLabel = new QLabel(this);

        m_searchEdit->setPlaceholderText(tr("Search %1").arg(resourceKindToTitle(kind).toLower()));
        m_searchEdit->setMaximumHeight(28);
        m_searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(m_searchEdit, &QLineEdit::textChanged, this, &ResourceTab::onSearchTextChanged);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &ResourceTab::onRefreshClicked);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &ResourceTab::onImportClicked);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &ResourceTab::onExportClicked);

        m_addButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_addButton->setToolTip(tr("Add"));
        connect(m_addButton, &QToolButton::clicked, this, &ResourceTab::onAddClicked);

        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setColumnCount(m_model->columnCount() + 1);
        styleTable();
        auto *resourceHeader = m_tableWidget->horizontalHeader();
        resourceHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        resourceHeader->setSectionResizeMode(1, QHeaderView::Stretch);
        resourceHeader->setSectionResizeMode(m_model->columnCount(), QHeaderView::Fixed);
        resourceHeader->setStretchLastSection(false);
        m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &ResourceTab::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::cellActivated, this, &ResourceTab::onTableDoubleClicked);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &ResourceTab::onBackClicked);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New %1").arg(resourceKindToSingularTitle(kind).toLower()));
        connect(m_newButton, &QToolButton::clicked, this, &ResourceTab::onAddClicked);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit"));
        connect(m_editButton, &QToolButton::clicked, this, &ResourceTab::onEditClicked);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &ResourceTab::onSaveClicked);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &ResourceTab::onCancelClicked);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete"));
        connect(m_deleteButton, &QToolButton::clicked, this, &ResourceTab::onDeleteClicked);

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette());
        auto *formLayout = new QFormLayout(ui->widgetFormHost);
        ui->splitterDetails->setStretchFactor(0, 2);
        ui->splitterDetails->setStretchFactor(1, 1);

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        m_chartScaleSpinBox->setRange(0.0, 1000000000.0);
        m_chartScaleSpinBox->setDecimals(0);

        m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));
        m_coordinatesEdit->setPlaceholderText(tr("41.9028, 12.4964\n41.9031, 12.4970"));
        m_geometryEdit->setPlaceholderText("{\n  \"type\": \"Polygon\",\n  \"coordinates\": [[[12.4, 41.9], [12.5, 41.9], [12.5, 42.0], [12.4, 41.9]]]\n}");
        m_chartLayersEdit->setPlaceholderText("base,depth");
        m_chartBoundsEdit->setPlaceholderText("[[12.0, 41.0], [13.0, 42.0]]");
        m_coordinateDisplayEdit->setReadOnly(true);
        m_coordinateDisplayEdit->setPlaceholderText(tr("No position"));
        m_coordinateEditButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_coordinateEditButton->setToolTip(tr("Edit coordinates"));

        retintToolButtons();

        if (m_kind == ResourceKind::Route) {
            m_detailTabs = new QTabWidget(ui->widgetPreviewHost);
            m_detailTabs->setDocumentMode(true);
            m_detailTabs->addTab(m_previewWidget, tr("Preview"));
            m_propertiesTreeTab = new QWidget(m_detailTabs);
            m_propertiesJsonTab = new QWidget(m_detailTabs);
            m_detailTabs->addTab(m_propertiesTreeTab, tr("Properties Tree"));
            m_detailTabs->addTab(m_propertiesJsonTab, tr("Properties JSON"));
            ui->verticalLayoutPreviewHost->addWidget(m_detailTabs);
            m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));
            m_propertiesEditor->setTabBarVisible(false);
            connect(m_detailTabs, &QTabWidget::currentChanged, this, [this](int) {
                syncDetailTabs();
            });
        } else {
            ui->verticalLayoutPreviewHost->addWidget(m_previewWidget);
        }

        auto *coordinateRowLayout = new QHBoxLayout(m_coordinateRowWidget);
        coordinateRowLayout->setContentsMargins(0, 0, 0, 0);
        coordinateRowLayout->setSpacing(6);
        coordinateRowLayout->addWidget(m_coordinateDisplayEdit, 1);
        coordinateRowLayout->addWidget(m_coordinateEditButton);
        connect(m_coordinateEditButton, &QToolButton::clicked, this, &ResourceTab::onCoordinateEditClicked);
        connect(m_notePositionCheckBox, &QCheckBox::toggled, this, [this](const bool checked) {
            if (m_kind == ResourceKind::Note) {
                m_coordinateDisplayEdit->setEnabled(checked);
                m_coordinateEditButton->setEnabled(checked && (m_isEditing || m_isCreating));
                if (!checked) {
                    m_coordinateDisplayEdit->clear();
                } else {
                    updateCoordinateDisplay();
                }
            }
        });

        if (m_kind != ResourceKind::Route) {
            formLayout->addRow(tr("Id"), m_idValueLabel);
        }
        formLayout->addRow(m_kind == ResourceKind::Note ? tr("Title") : tr("Name"), m_nameEdit);
        formLayout->addRow(tr("Description"), m_descriptionEdit);

        switch (m_kind) {
            case ResourceKind::Route:
                formLayout->addRow(tr("Type"), m_typeEdit);
                formLayout->addRow(tr("Coordinates"), m_coordinatesEdit);
                break;
            case ResourceKind::Region:
                formLayout->addRow(tr("Geometry (JSON)"), m_geometryEdit);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
            case ResourceKind::Note:
                formLayout->addRow(tr("Href"), m_hrefEdit);
                formLayout->addRow(tr("MIME Type"), m_mimeTypeEdit);
                formLayout->addRow(QString(), m_notePositionCheckBox);
                formLayout->addRow(tr("Position"), m_coordinateRowWidget);
                formLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
            case ResourceKind::Chart:
                formLayout->addRow(tr("Identifier"), m_identifierEdit);
                formLayout->addRow(tr("Format"), m_chartFormatEdit);
                formLayout->addRow(tr("Chart URL"), m_chartUrlEdit);
                formLayout->addRow(tr("Tilemap URL"), m_tilemapUrlEdit);
                formLayout->addRow(tr("Region"), m_chartRegionEdit);
                formLayout->addRow(tr("Scale"), m_chartScaleSpinBox);
                formLayout->addRow(tr("Layers"), m_chartLayersEdit);
                formLayout->addRow(tr("Bounds (JSON)"), m_chartBoundsEdit);
                break;
            case ResourceKind::Waypoint:
                formLayout->addRow(tr("Type"), m_typeEdit);
                formLayout->addRow(tr("Position"), m_coordinateRowWidget);
                formLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
        }

        if (m_kind != ResourceKind::Route) {
            formLayout->addRow(tr("Timestamp"), m_timestampValueLabel);
        }

        showListPage();

        connect(m_model, &QAbstractItemModel::modelReset, this, &ResourceTab::rebuildTable);
        connectEditorPreviewInputs();
        rebuildTable();
    }

    ResourceTab::~ResourceTab() {
        delete ui;
    }

    void ResourceTab::styleTable() {
        m_tableWidget->verticalHeader()->setVisible(false);
    }

    QString ResourceTab::currentResourceIdFromSelection() const {
        const auto selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.isEmpty()) {
            return {};
        }

        const int row = selectedItems.first()->row();
        if (row < 0 || row >= m_visibleResourceIds.size()) {
            return {};
        }
        return m_visibleResourceIds.at(row);
    }

    bool ResourceTab::canNavigateResource() const {
        return m_kind == ResourceKind::Waypoint;
    }

    void ResourceTab::rebuildTable() {
        const int actionsColumn = m_model->columnCount();
        const QString filter = m_searchEdit->text().trimmed();
        const QStringList headers = [&]() {
            QStringList values;
            for (int column = 0; column < m_model->columnCount(); ++column) {
                values.append(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString());
            }
            values.append(tr("Actions"));
            return values;
        }();

        m_tableWidget->clearContents();
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
        m_tableWidget->setRowCount(0);
        m_visibleResourceIds.clear();

        for (int row = 0; row < m_model->rowCount(); ++row) {
            const auto resource = m_model->resourceAtRow(row);
            const QString id = m_model->resourceIdAtRow(row);
            QString haystack = resourceDisplayName(resource) + " " + resourceDescription(resource);
            for (int column = 0; column < m_model->columnCount(); ++column) {
                haystack += " " + m_model->data(m_model->index(row, column), Qt::DisplayRole).toString();
            }
            if (!filter.isEmpty() && !haystack.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            const int visibleRow = m_tableWidget->rowCount();
            m_tableWidget->insertRow(visibleRow);
            m_visibleResourceIds.append(id);
            const QColor baseColor = m_tableWidget->palette().color(QPalette::Base);
            const QColor textColor = fairwindsk::ui::bestContrastingColor(
                baseColor,
                {m_tableWidget->palette().color(QPalette::Text),
                 m_tableWidget->palette().color(QPalette::WindowText),
                 m_tableWidget->palette().color(QPalette::ButtonText),
                 QColor(Qt::black),
                 QColor(Qt::white)});

            for (int column = 0; column < m_model->columnCount(); ++column) {
                auto *item = new QTableWidgetItem(m_model->data(m_model->index(row, column), Qt::DisplayRole).toString());
                item->setForeground(QBrush(textColor));
                item->setBackground(QBrush(baseColor));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                const auto alignment = m_model->data(m_model->index(row, column), Qt::TextAlignmentRole);
                if (alignment.isValid()) {
                    item->setTextAlignment(static_cast<Qt::Alignment>(alignment.toInt()));
                }
                m_tableWidget->setItem(visibleRow, column, item);
            }

            auto *actionsWidget = new QWidget();
            auto *actionsLayout = new QHBoxLayout(actionsWidget);
            actionsLayout->setContentsMargins(4, 0, 4, 0);
            actionsLayout->setSpacing(2);

            auto *navigateButton = new QToolButton(actionsWidget);
            navigateButton->setAutoRaise(true);
            navigateButton->setIcon(QIcon(":/resources/svg/OpenBridge/navigation-route.svg"));
            fairwindsk::ui::applyTintedButtonIcon(
                navigateButton,
                fairwindsk::ui::bestContrastingColor(
                    m_tableWidget->palette().color(QPalette::Base),
                    {m_tableWidget->palette().color(QPalette::Text),
                     m_tableWidget->palette().color(QPalette::ButtonText),
                     m_tableWidget->palette().color(QPalette::WindowText)}),
                QSize(22, 22));
            navigateButton->setToolTip(canNavigateResource()
                                               ? tr("Navigate to resource")
                                               : tr("Navigate is not available for this resource"));
            navigateButton->setEnabled(canNavigateResource());
            navigateButton->setProperty("resourceId", id);
            connect(navigateButton, &QToolButton::clicked, this, &ResourceTab::onNavigateRowClicked);
            actionsLayout->addWidget(navigateButton);

            auto *editButton = new QToolButton(actionsWidget);
            editButton->setAutoRaise(true);
            editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
            fairwindsk::ui::applyTintedButtonIcon(
                editButton,
                fairwindsk::ui::bestContrastingColor(
                    m_tableWidget->palette().color(QPalette::Base),
                    {m_tableWidget->palette().color(QPalette::Text),
                     m_tableWidget->palette().color(QPalette::ButtonText),
                     m_tableWidget->palette().color(QPalette::WindowText)}),
                QSize(22, 22));
            editButton->setToolTip(tr("Edit resource"));
            editButton->setProperty("resourceId", id);
            connect(editButton, &QToolButton::clicked, this, &ResourceTab::onEditRowClicked);
            actionsLayout->addWidget(editButton);

            auto *removeButton = new QToolButton(actionsWidget);
            removeButton->setAutoRaise(true);
            removeButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
            fairwindsk::ui::applyTintedButtonIcon(
                removeButton,
                fairwindsk::ui::bestContrastingColor(
                    m_tableWidget->palette().color(QPalette::Base),
                    {m_tableWidget->palette().color(QPalette::Text),
                     m_tableWidget->palette().color(QPalette::ButtonText),
                     m_tableWidget->palette().color(QPalette::WindowText)}),
                QSize(22, 22));
            removeButton->setToolTip(tr("Remove resource"));
            removeButton->setProperty("resourceId", id);
            connect(removeButton, &QToolButton::clicked, this, &ResourceTab::onRemoveRowClicked);
            actionsLayout->addWidget(removeButton);

            actionsLayout->addStretch(1);
            m_tableWidget->setCellWidget(visibleRow, actionsColumn, actionsWidget);
        }

        m_tableWidget->setColumnWidth(actionsColumn, 152);
    }

    void ResourceTab::selectResource(const QString &id) {
        for (int row = 0; row < m_visibleResourceIds.size(); ++row) {
            if (m_visibleResourceIds.at(row) == id) {
                m_tableWidget->selectRow(row);
                m_tableWidget->scrollToItem(m_tableWidget->item(row, 0));
                return;
            }
        }
    }

    void ResourceTab::showError(const QString &message) const {
        drawer::warning(const_cast<ResourceTab *>(this), resourceKindToTitle(m_kind), message);
    }

    void ResourceTab::showListPage() {
        m_stackedWidget->setCurrentWidget(m_listPage);
        m_isEditing = false;
        m_isCreating = false;
    }

    void ResourceTab::syncDetailTabs() {
        if (!m_detailTabs || !m_propertiesTreeTab || !m_propertiesJsonTab) {
            return;
        }

        if (!m_propertiesTreeTab->layout()) {
            auto *layout = new QVBoxLayout(m_propertiesTreeTab);
            layout->setContentsMargins(0, 0, 0, 0);
        }
        if (!m_propertiesJsonTab->layout()) {
            auto *layout = new QVBoxLayout(m_propertiesJsonTab);
            layout->setContentsMargins(0, 0, 0, 0);
        }

        const int currentIndex = m_detailTabs->currentIndex();
        if (currentIndex == 1) {
            m_propertiesEditor->setCurrentView(0);
            m_propertiesEditor->setParent(m_propertiesTreeTab);
            static_cast<QVBoxLayout *>(m_propertiesTreeTab->layout())->addWidget(m_propertiesEditor);
        } else if (currentIndex == 2) {
            m_propertiesEditor->setCurrentView(1);
            m_propertiesEditor->setParent(m_propertiesJsonTab);
            static_cast<QVBoxLayout *>(m_propertiesJsonTab->layout())->addWidget(m_propertiesEditor);
        }
    }

    void ResourceTab::showDetailsPage(const QString &id, const QJsonObject &resource, const bool editMode) {
        if (resource.isEmpty()) {
            clearEditor();
            m_currentResourceId = id;
        } else {
            populateEditor(id, resource);
        }
        setEditMode(editMode);
        m_stackedWidget->setCurrentWidget(m_detailsPage);
    }

    void ResourceTab::setEditMode(const bool editMode) {
        m_isEditing = editMode;

        const bool readOnly = !editMode;
        m_nameEdit->setReadOnly(readOnly);
        m_descriptionEdit->setReadOnly(readOnly);
        m_typeEdit->setReadOnly(readOnly);
        m_latitudeSpinBox->setEnabled(editMode);
        m_longitudeSpinBox->setEnabled(editMode);
        m_altitudeSpinBox->setEnabled(editMode);
        m_coordinatesEdit->setReadOnly(readOnly);
        m_geometryEdit->setReadOnly(readOnly);
        m_propertiesEditor->setEditMode(editMode);
        m_hrefEdit->setReadOnly(readOnly);
        m_mimeTypeEdit->setReadOnly(readOnly);
        m_notePositionCheckBox->setEnabled(editMode);
        m_identifierEdit->setReadOnly(readOnly);
        m_chartFormatEdit->setReadOnly(readOnly);
        m_chartUrlEdit->setReadOnly(readOnly);
        m_tilemapUrlEdit->setReadOnly(readOnly);
        m_chartRegionEdit->setReadOnly(readOnly);
        m_chartScaleSpinBox->setEnabled(editMode);
        m_chartLayersEdit->setReadOnly(readOnly);
        m_chartBoundsEdit->setReadOnly(readOnly);
        const bool coordinateEnabled = m_kind == ResourceKind::Note
                                       ? m_notePositionCheckBox->isChecked()
                                       : true;
        m_coordinateDisplayEdit->setEnabled(coordinateEnabled);
        m_coordinateEditButton->setEnabled(editMode && coordinateEnabled);
        updateCoordinateDisplay();

        if (editMode) {
            setButtonsForEdit();
        } else {
            setButtonsForReadOnly();
        }
    }

    void ResourceTab::setButtonsForReadOnly() {
        m_backButton->setVisible(true);
        m_newButton->setVisible(true);
        m_editButton->setVisible(!m_currentResourceId.isEmpty());
        m_saveButton->setVisible(false);
        m_cancelButton->setVisible(false);
        m_deleteButton->setVisible(!m_currentResourceId.isEmpty());
    }

    void ResourceTab::setButtonsForEdit() {
        m_backButton->setVisible(false);
        m_newButton->setVisible(false);
        m_editButton->setVisible(false);
        m_saveButton->setVisible(true);
        m_cancelButton->setVisible(true);
        m_deleteButton->setVisible(!m_isCreating);
    }

    QString ResourceTab::resourceDisplayName(const QJsonObject &resource) const {
        if (m_kind == ResourceKind::Note) {
            if (resource.contains("title") && resource["title"].isString()) {
                return resource["title"].toString();
            }
        }

        if (resource.contains("name") && resource["name"].isString()) {
            return resource["name"].toString();
        }

        const auto properties = featurePropertiesObject(resource);
        if (m_kind == ResourceKind::Note && properties.contains("title") && properties["title"].isString()) {
            return properties["title"].toString();
        }
        if (properties.contains("name") && properties["name"].isString()) {
            return properties["name"].toString();
        }
        if (resource.contains("identifier") && resource["identifier"].isString()) {
            return resource["identifier"].toString();
        }
        return {};
    }

    QString ResourceTab::resourceDescription(const QJsonObject &resource) const {
        if (resource.contains("description") && resource["description"].isString()) {
            return resource["description"].toString();
        }
        const auto properties = featurePropertiesObject(resource);
        if (properties.contains("description") && properties["description"].isString()) {
            return properties["description"].toString();
        }
        return {};
    }

    QString ResourceTab::detailsTitleForCurrentState() const {
        if (m_isCreating) {
            return tr("New %1").arg(resourceKindToSingularTitle(m_kind).toLower());
        }
        if (!m_nameEdit->text().trimmed().isEmpty()) {
            return m_nameEdit->text().trimmed();
        }
        return resourceKindToSingularTitle(m_kind);
    }

    void ResourceTab::populateEditor(const QString &id, const QJsonObject &resource) {
        m_currentResourceId = id;
        m_nameEdit->setText(resourceDisplayName(resource));
        m_descriptionEdit->setText(resourceDescription(resource));
        m_idValueLabel->setText(id);
        m_timestampValueLabel->setText(resource["timestamp"].toString());

        const auto properties = featurePropertiesObject(resource);
        m_propertiesEditor->setJsonObject(properties);

        switch (m_kind) {
            case ResourceKind::Route: {
                m_typeEdit->setText(resource["type"].toString());
                QStringList lines;
                for (const auto &value : coordinateArray(resource)) {
                    const auto point = value.toArray();
                    if (point.size() >= 2) {
                        QString line = QString::number(point.at(1).toDouble(), 'f', 8) + ", " +
                                       QString::number(point.at(0).toDouble(), 'f', 8);
                        if (point.size() > 2) {
                            line += ", " + QString::number(point.at(2).toDouble(), 'f', 2);
                        }
                        lines.append(line);
                    }
                }
                m_coordinatesEdit->setPlainText(lines.join("\n"));
                break;
            }
            case ResourceKind::Region:
                m_geometryEdit->setPlainText(QString::fromUtf8(QJsonDocument(geometryObject(resource)).toJson(QJsonDocument::Indented)));
                break;
            case ResourceKind::Note: {
                m_hrefEdit->setText(resource["href"].toString());
                m_mimeTypeEdit->setText(resource["mimeType"].toString());
                const auto position = resource["position"].toObject();
                const bool hasPosition = !position.isEmpty();
                m_notePositionCheckBox->setChecked(hasPosition);
                m_latitudeSpinBox->setValue(position["latitude"].toDouble());
                m_longitudeSpinBox->setValue(position["longitude"].toDouble());
                m_altitudeSpinBox->setValue(position["altitude"].toDouble());
                updateCoordinateDisplay();
                break;
            }
            case ResourceKind::Chart: {
                m_identifierEdit->setText(resource["identifier"].toString());
                m_chartFormatEdit->setText(resource["chartFormat"].toString());
                m_chartUrlEdit->setText(resource["chartUrl"].toString());
                m_tilemapUrlEdit->setText(resource["tilemapUrl"].toString());
                m_chartRegionEdit->setText(resource["region"].toString());
                m_chartScaleSpinBox->setValue(resource["scale"].toDouble());
                if (resource.contains("chartLayers") && resource["chartLayers"].isArray()) {
                    QStringList layers;
                    for (const auto &layer : resource["chartLayers"].toArray()) {
                        layers.append(layer.toString());
                    }
                    m_chartLayersEdit->setText(layers.join(", "));
                } else {
                    m_chartLayersEdit->clear();
                }
                if (resource.contains("bounds") && resource["bounds"].isArray()) {
                    m_chartBoundsEdit->setPlainText(QString::fromUtf8(QJsonDocument(resource["bounds"].toArray()).toJson(QJsonDocument::Indented)));
                } else {
                    m_chartBoundsEdit->clear();
                }
                break;
            }
            case ResourceKind::Waypoint: {
                m_typeEdit->setText(resource["type"].toString());
                const auto coordinates = coordinateArray(resource);
                m_longitudeSpinBox->setValue(coordinates.size() > 0 ? coordinates.at(0).toDouble() : 0.0);
                m_latitudeSpinBox->setValue(coordinates.size() > 1 ? coordinates.at(1).toDouble() : 0.0);
                m_altitudeSpinBox->setValue(coordinates.size() > 2 ? coordinates.at(2).toDouble() : 0.0);
                updateCoordinateDisplay();
                break;
            }
        }

        m_titleLabel->setText(detailsTitleForCurrentState());
        updatePreview(resource);
        syncDetailTabs();
    }

    QJsonObject ResourceTab::parseJsonObject(const QPlainTextEdit *edit) const {
        const QByteArray text = edit->toPlainText().trimmed().toUtf8();
        if (text.isEmpty()) {
            return {};
        }

        const auto document = QJsonDocument::fromJson(text);
        return document.isObject() ? document.object() : QJsonObject{};
    }

    QJsonArray ResourceTab::coordinatesJson() const {
        QJsonArray coordinates;
        const QStringList lines = m_coordinatesEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(',', Qt::SkipEmptyParts);
            if (parts.size() < 2) {
                continue;
            }

            QJsonArray point;
            point.append(parts.at(1).trimmed().toDouble());
            point.append(parts.at(0).trimmed().toDouble());
            if (parts.size() > 2) {
                point.append(parts.at(2).trimmed().toDouble());
            }
            coordinates.append(point);
        }

        return coordinates;
    }

    bool ResourceTab::validateEditor(QString *message) const {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            *message = tr("%1 is required.").arg(m_kind == ResourceKind::Note ? tr("Title") : tr("Name"));
            return false;
        }

        bool propertiesOk = false;
        m_propertiesEditor->jsonObject(&propertiesOk, message);
        if (!propertiesOk) {
            return false;
        }

        switch (m_kind) {
            case ResourceKind::Route:
                if (coordinatesJson().size() < 2) {
                    *message = tr("Routes require at least two coordinate points.");
                    return false;
                }
                break;
            case ResourceKind::Region: {
                const auto geometryDocument = QJsonDocument::fromJson(m_geometryEdit->toPlainText().trimmed().toUtf8());
                if (!geometryDocument.isObject() || !geometryDocument.object().contains("type") || !geometryDocument.object().contains("coordinates")) {
                    *message = tr("Region geometry must be a JSON object with type and coordinates.");
                    return false;
                }
                break;
            }
            case ResourceKind::Chart:
                if (m_identifierEdit->text().trimmed().isEmpty()) {
                    *message = tr("Chart identifier is required.");
                    return false;
                }
                if (m_chartFormatEdit->text().trimmed().isEmpty()) {
                    *message = tr("Chart format is required.");
                    return false;
                }
                break;
            case ResourceKind::Note:
            case ResourceKind::Waypoint:
                break;
        }

        return true;
    }

    QJsonObject ResourceTab::resourceFromEditor(const QString &idOverride) const {
        const QString id = !idOverride.trimmed().isEmpty()
                               ? idOverride.trimmed()
                               : (m_currentResourceId.isEmpty()
                                      ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                                      : m_currentResourceId);
        const auto properties = m_propertiesEditor->jsonObject();

        QJsonObject resource;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();

        switch (m_kind) {
            case ResourceKind::Route: {
                QJsonObject geometry;
                geometry["type"] = "LineString";
                geometry["coordinates"] = coordinatesJson();

                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;
                if (!properties.isEmpty()) {
                    feature["properties"] = properties;
                }

                resource["name"] = m_nameEdit->text().trimmed();
                resource["description"] = m_descriptionEdit->text().trimmed();
                resource["type"] = m_typeEdit->text().trimmed();
                resource["feature"] = feature;
                break;
            }
            case ResourceKind::Region: {
                const auto geometry = QJsonDocument::fromJson(m_geometryEdit->toPlainText().trimmed().toUtf8()).object();
                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;

                QJsonObject geometryProperties = properties;
                geometryProperties["name"] = m_nameEdit->text().trimmed();
                geometryProperties["description"] = m_descriptionEdit->text().trimmed();
                feature["properties"] = geometryProperties;

                resource["name"] = m_nameEdit->text().trimmed();
                resource["description"] = m_descriptionEdit->text().trimmed();
                resource["feature"] = feature;
                break;
            }
            case ResourceKind::Note: {
                resource["title"] = m_nameEdit->text().trimmed();
                resource["description"] = m_descriptionEdit->text().trimmed();
                resource["href"] = m_hrefEdit->text().trimmed();
                resource["mimeType"] = m_mimeTypeEdit->text().trimmed();
                if (!properties.isEmpty()) {
                    resource["properties"] = properties;
                }
                if (m_notePositionCheckBox->isChecked()) {
                    QJsonObject position;
                    position["latitude"] = m_latitudeSpinBox->value();
                    position["longitude"] = m_longitudeSpinBox->value();
                    position["altitude"] = m_altitudeSpinBox->value();
                    resource["position"] = position;
                }
                break;
            }
            case ResourceKind::Chart: {
                resource["name"] = m_nameEdit->text().trimmed();
                resource["description"] = m_descriptionEdit->text().trimmed();
                resource["identifier"] = m_identifierEdit->text().trimmed();
                resource["chartFormat"] = m_chartFormatEdit->text().trimmed();
                resource["chartUrl"] = m_chartUrlEdit->text().trimmed();
                resource["tilemapUrl"] = m_tilemapUrlEdit->text().trimmed();
                resource["region"] = m_chartRegionEdit->text().trimmed();
                if (m_chartScaleSpinBox->value() > 0.0) {
                    resource["scale"] = m_chartScaleSpinBox->value();
                }
                const QStringList layers = m_chartLayersEdit->text().split(',', Qt::SkipEmptyParts);
                if (!layers.isEmpty()) {
                    QJsonArray chartLayers;
                    for (const auto &layer : layers) {
                        chartLayers.append(layer.trimmed());
                    }
                    resource["chartLayers"] = chartLayers;
                }
                const QByteArray boundsText = m_chartBoundsEdit->toPlainText().trimmed().toUtf8();
                if (!boundsText.isEmpty()) {
                    const auto boundsDocument = QJsonDocument::fromJson(boundsText);
                    if (boundsDocument.isArray()) {
                        resource["bounds"] = boundsDocument.array();
                    }
                }
                break;
            }
            case ResourceKind::Waypoint: {
                QJsonArray coordinates;
                coordinates.append(m_longitudeSpinBox->value());
                coordinates.append(m_latitudeSpinBox->value());
                coordinates.append(m_altitudeSpinBox->value());

                QJsonObject geometry;
                geometry["type"] = "Point";
                geometry["coordinates"] = coordinates;

                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;
                if (!properties.isEmpty()) {
                    feature["properties"] = properties;
                }

                resource["name"] = m_nameEdit->text().trimmed();
                resource["description"] = m_descriptionEdit->text().trimmed();
                resource["type"] = m_typeEdit->text().trimmed();
                resource["feature"] = feature;
                break;
            }
        }

        return resource;
    }

    void ResourceTab::connectEditorPreviewInputs() {
        const auto refresh = [this]() {
            refreshEditorLiveState();
        };

        connect(m_nameEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_descriptionEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_typeEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_latitudeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, refresh);
        connect(m_longitudeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, refresh);
        connect(m_altitudeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, refresh);
        connect(m_coordinatesEdit, &QPlainTextEdit::textChanged, this, refresh);
        connect(m_geometryEdit, &QPlainTextEdit::textChanged, this, refresh);
        connect(m_hrefEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_mimeTypeEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_notePositionCheckBox, &QCheckBox::toggled, this, refresh);
        connect(m_identifierEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_chartFormatEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_chartUrlEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_tilemapUrlEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_chartRegionEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_chartScaleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, refresh);
        connect(m_chartLayersEdit, &QLineEdit::textChanged, this, refresh);
        connect(m_chartBoundsEdit, &QPlainTextEdit::textChanged, this, refresh);
    }

    void ResourceTab::refreshEditorLiveState() {
        m_titleLabel->setText(detailsTitleForCurrentState());
        updateCoordinateDisplay();

        if (m_stackedWidget->currentWidget() != m_detailsPage || !(m_isEditing || m_isCreating)) {
            return;
        }

        updatePreview(resourceFromEditor(QStringLiteral("preview")));
    }

    void ResourceTab::clearEditor() {
        m_currentResourceId.clear();
        m_idValueLabel->setText(tr("New"));
        m_timestampValueLabel->clear();
        m_nameEdit->clear();
        m_descriptionEdit->clear();
        m_typeEdit->clear();
        m_latitudeSpinBox->setValue(0.0);
        m_longitudeSpinBox->setValue(0.0);
        m_altitudeSpinBox->setValue(0.0);
        m_coordinateDisplayEdit->clear();
        m_coordinatesEdit->clear();
        m_geometryEdit->clear();
        m_propertiesEditor->setJsonObject(QJsonObject{});
        m_hrefEdit->clear();
        m_mimeTypeEdit->clear();
        m_notePositionCheckBox->setChecked(false);
        m_identifierEdit->clear();
        m_chartFormatEdit->clear();
        m_chartUrlEdit->clear();
        m_tilemapUrlEdit->clear();
        m_chartRegionEdit->clear();
        m_chartScaleSpinBox->setValue(0.0);
        m_chartLayersEdit->clear();
        m_chartBoundsEdit->clear();

        if (m_kind == ResourceKind::Route) {
            m_typeEdit->setText("route");
        } else if (m_kind == ResourceKind::Waypoint) {
            m_typeEdit->setText("generic");
        } else if (m_kind == ResourceKind::Note) {
            m_mimeTypeEdit->setText("text/plain");
        }

        m_titleLabel->setText(detailsTitleForCurrentState());
        updatePreview(resourceFromEditor());
        if (m_detailTabs) {
            m_detailTabs->setCurrentIndex(0);
            syncDetailTabs();
        }
    }

    void ResourceTab::updateCoordinateDisplay() {
        if (m_kind != ResourceKind::Note && m_kind != ResourceKind::Waypoint) {
            return;
        }

        if (m_kind == ResourceKind::Note && !m_notePositionCheckBox->isChecked()) {
            m_coordinateDisplayEdit->clear();
            return;
        }

        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        m_coordinateDisplayEdit->setText(
            fairwindsk::ui::geo::formatCoordinate(
                m_latitudeSpinBox->value(),
                m_longitudeSpinBox->value(),
                configuration->getCoordinateFormat()));
    }

    void ResourceTab::onCoordinateEditClicked() {
        if (!(m_isEditing || m_isCreating)) {
            return;
        }
        if (m_kind == ResourceKind::Note && !m_notePositionCheckBox->isChecked()) {
            return;
        }

        double latitude = m_latitudeSpinBox->value();
        double longitude = m_longitudeSpinBox->value();
        double altitude = m_altitudeSpinBox->value();
        if (!drawer::editGeoCoordinate(this, tr("Coordinates"), &latitude, &longitude, &altitude)) {
            return;
        }

        m_latitudeSpinBox->setValue(latitude);
        m_longitudeSpinBox->setValue(longitude);
        m_altitudeSpinBox->setValue(altitude);
        updateCoordinateDisplay();
        updatePreview(resourceFromEditor());
    }

    void ResourceTab::updatePreview(const QJsonObject &resource) {
        QList<QPair<QString, QJsonObject>> resources;
        resources.append({m_currentResourceId, resource});
        m_previewWidget->setGeoJson(exportResourcesAsGeoJson(m_kind, resources));
    }

    void ResourceTab::onAddClicked() {
        clearEditor();
        m_isCreating = true;
        showDetailsPage({}, QJsonObject{}, true);
    }

    void ResourceTab::onEditClicked() {
        if (m_stackedWidget->currentWidget() == m_listPage) {
            const QString id = currentResourceIdFromSelection();
            if (id.isEmpty()) {
                showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
                return;
            }
            for (int row = 0; row < m_model->rowCount(); ++row) {
                if (m_model->resourceIdAtRow(row) == id) {
                    showDetailsPage(id, m_model->resourceAtRow(row), true);
                    return;
                }
            }
            return;
        }

        if (!m_currentResourceId.isEmpty()) {
            setEditMode(true);
        }
    }

    void ResourceTab::onDeleteClicked() {
        QString id;
        QString name;
        if (m_stackedWidget->currentWidget() == m_listPage) {
            id = currentResourceIdFromSelection();
            if (id.isEmpty()) {
                showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
                return;
            }
            for (int row = 0; row < m_model->rowCount(); ++row) {
                if (m_model->resourceIdAtRow(row) == id) {
                    name = resourceDisplayName(m_model->resourceAtRow(row));
                    break;
                }
            }
        } else {
            id = m_currentResourceId;
            name = m_nameEdit->text().trimmed();
        }

        const auto choice = drawer::question(
            this,
            resourceKindToTitle(m_kind),
            tr("Delete %1 \"%2\"?").arg(resourceKindToSingularTitle(m_kind).toLower(), name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return;
        }

        if (!m_model->deleteResource(id)) {
            showError(tr("Unable to delete the selected resource."));
            return;
        }

        showListPage();
        rebuildTable();
    }

    void ResourceTab::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
            fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette());
        }
    }

    void ResourceTab::retintToolButtons() const {
        const QColor buttonIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        for (auto *button : {
                 m_refreshButton,
                 m_addButton,
                 m_backButton,
                 m_newButton,
                 m_editButton,
                 m_saveButton,
                 m_cancelButton,
                 m_deleteButton,
                 m_coordinateEditButton
             }) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(28, 28));
        }
    }

    void ResourceTab::onImportClicked() {
        const QString fileName = drawer::getOpenFilePath(
            this,
            tr("Import %1").arg(resourceKindToTitle(m_kind)),
            QString(),
            tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            showError(tr("Unable to open %1.").arg(fileName));
            return;
        }

        QList<QPair<QString, QJsonObject>> resources;
        QString message;
        if (!importResourcesFromGeoJson(m_kind, QJsonDocument::fromJson(file.readAll()), &resources, &message)) {
            showError(message);
            return;
        }

        int importedCount = 0;
        for (const auto &entry : resources) {
            bool updated = false;
            if (!entry.first.isEmpty()) {
                for (int row = 0; row < m_model->rowCount(); ++row) {
                    if (m_model->resourceIdAtRow(row) == entry.first) {
                        updated = m_model->updateResource(entry.first, entry.second);
                        break;
                    }
                }
            }
            if (!updated && !m_model->createResource(entry.second).isEmpty()) {
                updated = true;
            }
            if (updated) {
                ++importedCount;
            }
        }

        if (importedCount == 0) {
            showError(tr("No %1 could be imported from the selected GeoJSON file.")
                              .arg(resourceKindToTitle(m_kind).toLower()));
            return;
        }

        rebuildTable();
        if (!resources.isEmpty()) {
            const QString importedId = !resources.first().first.isEmpty()
                                           ? resources.first().first
                                           : m_model->resourceIdAtRow(0);
            if (!importedId.isEmpty()) {
                selectResource(importedId);
            }
        }

        drawer::information(this,
                            resourceKindToTitle(m_kind),
                            tr("Imported %1 %2 GeoJSON feature(s).")
                                    .arg(importedCount)
                                    .arg(resourceKindToTitle(m_kind).toLower()));
    }

    void ResourceTab::onExportClicked() {
        QStringList ids;
        if (m_stackedWidget->currentWidget() == m_detailsPage && !m_currentResourceId.isEmpty()) {
            ids.append(m_currentResourceId);
        } else {
            const QString id = currentResourceIdFromSelection();
            if (!id.isEmpty()) {
                ids.append(id);
            }
        }

        const QString fileName = drawer::getSaveFilePath(
            this,
            tr("Export %1").arg(resourceKindToTitle(m_kind)),
            QString("%1.geojson").arg(resourceKindToCollection(m_kind)),
            tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            showError(tr("Unable to write %1.").arg(fileName));
            return;
        }

        QList<QPair<QString, QJsonObject>> resources;
        if (ids.isEmpty()) {
            for (int row = 0; row < m_model->rowCount(); ++row) {
                resources.append({m_model->resourceIdAtRow(row), m_model->resourceAtRow(row)});
            }
        } else {
            for (const auto &id : ids) {
                for (int row = 0; row < m_model->rowCount(); ++row) {
                    if (m_model->resourceIdAtRow(row) == id) {
                        resources.append({id, m_model->resourceAtRow(row)});
                        break;
                    }
                }
            }
        }

        file.write(exportResourcesAsGeoJson(m_kind, resources).toJson(QJsonDocument::Indented));
    }

    void ResourceTab::onRefreshClicked() {
        m_model->reload(true);
        rebuildTable();
    }

    void ResourceTab::onSearchTextChanged(const QString &) {
        rebuildTable();
    }

    void ResourceTab::onOpenClicked() {
        const QString id = currentResourceIdFromSelection();
        if (id.isEmpty()) {
            showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
            return;
        }
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                showDetailsPage(id, m_model->resourceAtRow(row), false);
                return;
            }
        }
    }

    void ResourceTab::onTableDoubleClicked(const int row, const int) {
        if (row < 0 || row >= m_visibleResourceIds.size()) {
            return;
        }
        const QString id = m_visibleResourceIds.at(row);
        for (int sourceRow = 0; sourceRow < m_model->rowCount(); ++sourceRow) {
            if (m_model->resourceIdAtRow(sourceRow) == id) {
                showDetailsPage(id, m_model->resourceAtRow(sourceRow), false);
                return;
            }
        }
    }

    void ResourceTab::onNavigateRowClicked() {
        if (!canNavigateResource()) {
            return;
        }

        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("resourceId").toString();
        if (!fairwindsk::FairWindSK::getInstance()->getSignalKClient()->navigateToWaypoint("/resources/waypoints/" + id)) {
            showError(tr("Unable to start navigation to the selected resource."));
        }
    }

    void ResourceTab::onEditRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("resourceId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                showDetailsPage(id, m_model->resourceAtRow(row), true);
                return;
            }
        }
    }

    void ResourceTab::onRemoveRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("resourceId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                const auto resource = m_model->resourceAtRow(row);
                const QString previousId = m_currentResourceId;
                const QString previousName = m_nameEdit->text();
                const bool wasOnList = m_stackedWidget->currentWidget() == m_listPage;
                m_currentResourceId = id;
                m_nameEdit->setText(resourceDisplayName(resource));
                onDeleteClicked();
                if (wasOnList) {
                    m_currentResourceId = previousId;
                    m_nameEdit->setText(previousName);
                }
                return;
            }
        }
    }

    void ResourceTab::onBackClicked() {
        showListPage();
    }

    void ResourceTab::onSaveClicked() {
        QString message;
        if (!validateEditor(&message)) {
            showError(message);
            return;
        }

        const auto resource = resourceFromEditor();
        QString id = m_currentResourceId;
        if (m_isCreating) {
            id = m_model->createResource(resource);
            if (id.isEmpty()) {
                showError(tr("Unable to create the selected resource."));
                return;
            }
        } else if (!m_model->updateResource(m_currentResourceId, resource)) {
            showError(tr("Unable to update the selected resource."));
            return;
        }

        showListPage();
        rebuildTable();
        selectResource(id);
    }

    void ResourceTab::onCancelClicked() {
        showListPage();
    }
}
