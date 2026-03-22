//
// Created by Codex on 21/03/26.
//

#include "ResourceTab.hpp"

#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QUuid>

#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"
#include "JsonObjectEditorWidget.hpp"
#include "signalk/Client.hpp"

namespace {
    const QString kLineEditStyle = QStringLiteral(
        "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kPlainTextStyle = QStringLiteral(
        "QPlainTextEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kSpinBoxStyle = QStringLiteral(
        "QAbstractSpinBox { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kComboBoxStyle = QStringLiteral(
        "QComboBox { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");

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
          m_kind(kind),
          m_model(new ResourceModel(kind, this)),
          m_proxyModel(new QSortFilterProxyModel(this)),
          m_stackedWidget(new QStackedWidget(this)),
          m_listPage(new QWidget(this)),
          m_detailsPage(new QWidget(this)),
          m_searchEdit(new QLineEdit(this)),
          m_tableView(new QTableView(this)),
          m_addButton(new QToolButton(this)),
          m_backButton(new QToolButton(this)),
          m_newButton(new QToolButton(this)),
          m_editButton(new QToolButton(this)),
          m_saveButton(new QToolButton(this)),
          m_cancelButton(new QToolButton(this)),
          m_deleteButton(new QToolButton(this)),
          m_importButton(new QToolButton(this)),
          m_exportButton(new QToolButton(this)),
          m_refreshButton(new QToolButton(this)),
          m_titleLabel(new QLabel(this)),
          m_idValueLabel(new QLabel(this)),
          m_timestampValueLabel(new QLabel(this)),
          m_nameEdit(new QLineEdit(this)),
          m_descriptionEdit(new QLineEdit(this)),
          m_typeEdit(new QLineEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
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
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(m_stackedWidget);

        auto *listLayout = new QVBoxLayout(m_listPage);
        auto *toolbarLayout = new QHBoxLayout();
        listLayout->addLayout(toolbarLayout);

        m_searchEdit->setPlaceholderText(tr("Search %1").arg(resourceKindToTitle(kind).toLower()));
        connect(m_searchEdit, &QLineEdit::textChanged, this, &ResourceTab::onSearchTextChanged);
        toolbarLayout->addWidget(m_searchEdit, 1);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &ResourceTab::onRefreshClicked);
        toolbarLayout->addWidget(m_refreshButton);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &ResourceTab::onImportClicked);
        toolbarLayout->addWidget(m_importButton);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &ResourceTab::onExportClicked);
        toolbarLayout->addWidget(m_exportButton);

        m_addButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_addButton->setToolTip(tr("Add"));
        connect(m_addButton, &QToolButton::clicked, this, &ResourceTab::onAddClicked);
        toolbarLayout->addWidget(m_addButton);

        auto *openButton = new QToolButton(this);
        openButton->setIcon(QIcon(":/resources/svg/gui-view-show-svgrepo-com.svg"));
        openButton->setToolTip(tr("Show details"));
        connect(openButton, &QToolButton::clicked, this, &ResourceTab::onOpenClicked);
        toolbarLayout->addWidget(openButton);

        auto *editListButton = new QToolButton(this);
        editListButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        editListButton->setToolTip(tr("Edit selected resource"));
        connect(editListButton, &QToolButton::clicked, this, &ResourceTab::onEditClicked);
        toolbarLayout->addWidget(editListButton);

        auto *deleteListButton = new QToolButton(this);
        deleteListButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        deleteListButton->setToolTip(tr("Delete selected resource"));
        connect(deleteListButton, &QToolButton::clicked, this, &ResourceTab::onDeleteClicked);
        toolbarLayout->addWidget(deleteListButton);

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setFilterKeyColumn(-1);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxyModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setSortingEnabled(true);
        m_tableView->sortByColumn(0, Qt::AscendingOrder);
        auto *resourceHeader = m_tableView->horizontalHeader();
        resourceHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        resourceHeader->setSectionResizeMode(1, QHeaderView::Stretch);
        resourceHeader->setStretchLastSection(false);
        connect(m_tableView, &QTableView::doubleClicked, this, &ResourceTab::onTableDoubleClicked);
        connect(m_tableView, &QTableView::activated, this, &ResourceTab::onTableDoubleClicked);
        listLayout->addWidget(m_tableView);

        auto *detailsLayout = new QVBoxLayout(m_detailsPage);
        auto *detailsToolbar = new QHBoxLayout();
        detailsLayout->addLayout(detailsToolbar);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &ResourceTab::onBackClicked);
        detailsToolbar->addWidget(m_backButton);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New %1").arg(resourceKindToSingularTitle(kind).toLower()));
        connect(m_newButton, &QToolButton::clicked, this, &ResourceTab::onAddClicked);
        detailsToolbar->addWidget(m_newButton);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit"));
        connect(m_editButton, &QToolButton::clicked, this, &ResourceTab::onEditClicked);
        detailsToolbar->addWidget(m_editButton);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &ResourceTab::onSaveClicked);
        detailsToolbar->addWidget(m_saveButton);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &ResourceTab::onCancelClicked);
        detailsToolbar->addWidget(m_cancelButton);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete"));
        connect(m_deleteButton, &QToolButton::clicked, this, &ResourceTab::onDeleteClicked);
        detailsToolbar->addWidget(m_deleteButton);
        detailsToolbar->addStretch(1);

        m_titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
        detailsLayout->addWidget(m_titleLabel);

        auto *detailsSplitter = new QSplitter(Qt::Horizontal, m_detailsPage);
        detailsSplitter->setChildrenCollapsible(false);
        detailsLayout->addWidget(detailsSplitter, 1);

        auto *formWidget = new QWidget(detailsSplitter);
        auto *formLayout = new QFormLayout(formWidget);
        detailsSplitter->addWidget(formWidget);
        detailsSplitter->addWidget(m_previewWidget);
        detailsSplitter->setStretchFactor(0, 1);
        detailsSplitter->setStretchFactor(1, 1);

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
        m_searchEdit->setStyleSheet(kLineEditStyle);
        m_nameEdit->setStyleSheet(kLineEditStyle);
        m_descriptionEdit->setStyleSheet(kLineEditStyle);
        m_typeEdit->setStyleSheet(kLineEditStyle);
        m_hrefEdit->setStyleSheet(kLineEditStyle);
        m_mimeTypeEdit->setStyleSheet(kLineEditStyle);
        m_identifierEdit->setStyleSheet(kLineEditStyle);
        m_chartFormatEdit->setStyleSheet(kLineEditStyle);
        m_chartUrlEdit->setStyleSheet(kLineEditStyle);
        m_tilemapUrlEdit->setStyleSheet(kLineEditStyle);
        m_chartRegionEdit->setStyleSheet(kLineEditStyle);
        m_chartLayersEdit->setStyleSheet(kLineEditStyle);
        m_latitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_longitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_altitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_chartScaleSpinBox->setStyleSheet(kSpinBoxStyle);
        m_coordinatesEdit->setStyleSheet(kPlainTextStyle);
        m_geometryEdit->setStyleSheet(kPlainTextStyle);
        m_chartBoundsEdit->setStyleSheet(kPlainTextStyle);

        formLayout->addRow(tr("Id"), m_idValueLabel);
        formLayout->addRow(m_kind == ResourceKind::Note ? tr("Title") : tr("Name"), m_nameEdit);
        formLayout->addRow(tr("Description"), m_descriptionEdit);

        switch (m_kind) {
            case ResourceKind::Route:
                formLayout->addRow(tr("Type"), m_typeEdit);
                formLayout->addRow(tr("Coordinates"), m_coordinatesEdit);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
            case ResourceKind::Region:
                formLayout->addRow(tr("Geometry (JSON)"), m_geometryEdit);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
            case ResourceKind::Note:
                formLayout->addRow(tr("Href"), m_hrefEdit);
                formLayout->addRow(tr("MIME Type"), m_mimeTypeEdit);
                formLayout->addRow(QString(), m_notePositionCheckBox);
                formLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
                formLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
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
                formLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
                formLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
                formLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
                formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
                break;
        }

        formLayout->addRow(tr("Timestamp"), m_timestampValueLabel);

        m_stackedWidget->addWidget(m_listPage);
        m_stackedWidget->addWidget(m_detailsPage);
        showListPage();
    }

    QModelIndex ResourceTab::currentSourceIndex() const {
        const QModelIndex proxyIndex = m_tableView->currentIndex();
        return proxyIndex.isValid() ? m_proxyModel->mapToSource(proxyIndex) : QModelIndex{};
    }

    void ResourceTab::selectResource(const QString &id) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                const QModelIndex sourceIndex = m_model->index(row, 0);
                const QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
                if (proxyIndex.isValid()) {
                    m_tableView->selectRow(proxyIndex.row());
                    m_tableView->scrollTo(proxyIndex);
                }
                break;
            }
        }
    }

    void ResourceTab::showError(const QString &message) const {
        QMessageBox::warning(const_cast<ResourceTab *>(this), resourceKindToTitle(m_kind), message);
    }

    void ResourceTab::showListPage() {
        m_stackedWidget->setCurrentWidget(m_listPage);
        m_isEditing = false;
        m_isCreating = false;
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
                break;
            }
        }

        m_titleLabel->setText(detailsTitleForCurrentState());
        updatePreview(resource);
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

    QJsonObject ResourceTab::resourceFromEditor() const {
        const QString id = m_currentResourceId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_currentResourceId;
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
            const QModelIndex index = currentSourceIndex();
            if (!index.isValid()) {
                showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
                return;
            }
            const QString id = m_model->resourceIdAtRow(index.row());
            showDetailsPage(id, m_model->resourceAtRow(index.row()), true);
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
            const QModelIndex index = currentSourceIndex();
            if (!index.isValid()) {
                showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
                return;
            }
            id = m_model->resourceIdAtRow(index.row());
            name = resourceDisplayName(m_model->resourceAtRow(index.row()));
        } else {
            id = m_currentResourceId;
            name = m_nameEdit->text().trimmed();
        }

        const auto choice = QMessageBox::question(
            this,
            resourceKindToTitle(m_kind),
            tr("Delete %1 \"%2\"?").arg(resourceKindToSingularTitle(m_kind).toLower(), name));
        if (choice != QMessageBox::Yes) {
            return;
        }

        if (!m_model->deleteResource(id)) {
            showError(tr("Unable to delete the selected resource."));
            return;
        }

        showListPage();
    }

    void ResourceTab::onImportClicked() {
        const QString fileName = QFileDialog::getOpenFileName(
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

        QMessageBox::information(this,
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
            const QModelIndex index = currentSourceIndex();
            if (index.isValid()) {
                ids.append(m_model->resourceIdAtRow(index.row()));
            }
        }

        const QString fileName = QFileDialog::getSaveFileName(
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
    }

    void ResourceTab::onSearchTextChanged(const QString &text) {
        const QRegularExpression expression(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption);
        m_proxyModel->setFilterRegularExpression(expression);
    }

    void ResourceTab::onOpenClicked() {
        const QModelIndex sourceIndex = currentSourceIndex();
        if (!sourceIndex.isValid()) {
            showError(tr("Select a %1 first.").arg(resourceKindToSingularTitle(m_kind).toLower()));
            return;
        }

        const QString id = m_model->resourceIdAtRow(sourceIndex.row());
        showDetailsPage(id, m_model->resourceAtRow(sourceIndex.row()), false);
    }

    void ResourceTab::onTableDoubleClicked(const QModelIndex &index) {
        const QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
        if (!sourceIndex.isValid()) {
            return;
        }

        const QString id = m_model->resourceIdAtRow(sourceIndex.row());
        showDetailsPage(id, m_model->resourceAtRow(sourceIndex.row()), false);
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
        selectResource(id);
    }

    void ResourceTab::onCancelClicked() {
        showListPage();
    }
}
