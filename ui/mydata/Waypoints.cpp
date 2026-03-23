//
// Created by Raffaele Montella on 15/02/25.
//

#include "Waypoints.hpp"

#include <algorithm>
#include <QBrush>
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QApplication>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSizePolicy>
#include <QUuid>

#include "FairWindSK.hpp"
#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"
#include "JsonObjectEditorWidget.hpp"

namespace {
    const QString kLineEditStyle = QStringLiteral(
        "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kPlainTextStyle = QStringLiteral(
        "QPlainTextEdit { background: #f7f7f4; color: #1f2937; border: 1px solid #d1d5db; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kSpinBoxStyle = QStringLiteral(
        "QAbstractSpinBox { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
    const QString kTableStyle = QStringLiteral(
        "QTableWidget { background: #f7f7f4; color: #1f2937; gridline-color: #d1d5db; selection-background-color: #c7d2fe; selection-color: #111827; }"
        "QTableCornerButton::section, QHeaderView::section { background: #e5e7eb; color: #111827; border: 1px solid #d1d5db; padding: 4px; }");
    const QString kActionButtonStyle = QStringLiteral(
        "QToolButton { background: #f3f4f6; color: #111827; border: 1px solid #d1d5db; border-radius: 4px; padding: 2px; }"
        "QToolButton:hover { background: #e5e7eb; }"
        "QToolButton:pressed { background: #d1d5db; }");

    QJsonObject featureObject(const QJsonObject &resource) {
        return resource["feature"].toObject();
    }

    QJsonObject featurePropertiesObject(const QJsonObject &resource) {
        return featureObject(resource)["properties"].toObject();
    }

    QJsonArray coordinateArray(const QJsonObject &resource) {
        return featureObject(resource)["geometry"].toObject()["coordinates"].toArray();
    }

    QString displayNameForWaypoint(const QJsonObject &resource) {
        if (resource.contains("name") && resource["name"].isString()) {
            return resource["name"].toString();
        }

        const auto properties = featurePropertiesObject(resource);
        if (properties.contains("name") && properties["name"].isString()) {
            return properties["name"].toString();
        }

        return {};
    }

    QString descriptionForWaypoint(const QJsonObject &resource) {
        if (resource.contains("description") && resource["description"].isString()) {
            return resource["description"].toString();
        }

        const auto properties = featurePropertiesObject(resource);
        if (properties.contains("description") && properties["description"].isString()) {
            return properties["description"].toString();
        }

        return {};
    }
}

namespace fairwindsk::ui::mydata {
    Waypoints::Waypoints(QWidget *parent)
        : QWidget(parent),
          m_model(new ResourceModel(ResourceKind::Waypoint, this)),
          m_stackedWidget(new QStackedWidget(this)),
          m_listPage(new QWidget(this)),
          m_detailsPage(new QWidget(this)),
          m_searchStack(new QStackedWidget(this)),
          m_searchEdit(new QLineEdit(this)),
          m_progressBar(new QProgressBar(this)),
          m_tableWidget(new QTableWidget(this)),
          m_addButton(new QToolButton(this)),
          m_importButton(new QToolButton(this)),
          m_exportButton(new QToolButton(this)),
          m_refreshButton(new QToolButton(this)),
          m_selectAllButton(new QToolButton(this)),
          m_bulkRemoveButton(new QToolButton(this)),
          m_backButton(new QToolButton(this)),
          m_newButton(new QToolButton(this)),
          m_navigateButton(new QToolButton(this)),
          m_editButton(new QToolButton(this)),
          m_saveButton(new QToolButton(this)),
          m_cancelButton(new QToolButton(this)),
          m_deleteButton(new QToolButton(this)),
          m_titleLabel(new QLabel(this)),
          m_nameEdit(new QLineEdit(this)),
          m_descriptionEdit(new QPlainTextEdit(this)),
          m_typeEdit(new QLineEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
          m_contactsEdit(new QPlainTextEdit(this)),
          m_contactsLabel(new QLabel(tr("Contacts"), this)),
          m_propertiesEditor(new JsonObjectEditorWidget(this)),
          m_previewWidget(new GeoJsonPreviewWidget(this)),
          m_idValueLabel(new QLabel(this)),
          m_timestampValueLabel(new QLabel(this)),
          m_searchTimer(new QTimer(this)) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(m_stackedWidget);

        auto *listLayout = new QVBoxLayout(m_listPage);
        listLayout->setContentsMargins(0, 0, 0, 0);
        listLayout->setSpacing(6);
        auto *toolbarLayout = new QHBoxLayout();
        toolbarLayout->setContentsMargins(0, 0, 0, 0);
        toolbarLayout->setSpacing(6);
        listLayout->addLayout(toolbarLayout);

        m_searchEdit->setPlaceholderText(tr("Search waypoints"));
        connect(m_searchEdit, &QLineEdit::textChanged, this, &Waypoints::onSearchTextChanged);
        m_progressBar->setTextVisible(true);
        m_progressBar->setVisible(false);
        m_searchStack->addWidget(m_searchEdit);
        m_searchStack->addWidget(m_progressBar);
        m_searchStack->setCurrentWidget(m_searchEdit);
        m_searchEdit->setMaximumHeight(28);
        m_searchStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        toolbarLayout->addWidget(m_searchStack, 1);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &Waypoints::onRefreshClicked);
        toolbarLayout->addWidget(m_refreshButton);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &Waypoints::onImportClicked);
        toolbarLayout->addWidget(m_importButton);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &Waypoints::onExportClicked);
        toolbarLayout->addWidget(m_exportButton);

        m_addButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_addButton->setToolTip(tr("Add waypoint"));
        connect(m_addButton, &QToolButton::clicked, this, &Waypoints::onAddClicked);
        toolbarLayout->addWidget(m_addButton);

        m_selectAllButton->setIcon(QIcon(":/resources/svg/OpenBridge/content-copy-google.svg"));
        m_selectAllButton->setToolTip(tr("Select all shown waypoints"));
        connect(m_selectAllButton, &QToolButton::clicked, this, &Waypoints::onSelectAllClicked);
        toolbarLayout->addWidget(m_selectAllButton);

        m_bulkRemoveButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_bulkRemoveButton->setToolTip(tr("Remove all shown waypoints"));
        connect(m_bulkRemoveButton, &QToolButton::clicked, this, &Waypoints::onRemoveSelectedClicked);
        toolbarLayout->addWidget(m_bulkRemoveButton);

        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setColumnCount(m_model->columnCount() + 1);
        styleTable();
        auto *waypointHeader = m_tableWidget->horizontalHeader();
        waypointHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        waypointHeader->setSectionResizeMode(1, QHeaderView::Stretch);
        waypointHeader->setSectionResizeMode(m_model->columnCount(), QHeaderView::Fixed);
        waypointHeader->setStretchLastSection(false);
        m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &Waypoints::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::cellActivated, this, &Waypoints::onTableDoubleClicked);
        listLayout->addWidget(m_tableWidget, 1);

        auto *detailsLayout = new QVBoxLayout(m_detailsPage);
        auto *detailsToolbarLayout = new QHBoxLayout();
        detailsLayout->addLayout(detailsToolbarLayout);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &Waypoints::onBackClicked);
        detailsToolbarLayout->addWidget(m_backButton);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New waypoint"));
        connect(m_newButton, &QToolButton::clicked, this, &Waypoints::onAddClicked);
        detailsToolbarLayout->addWidget(m_newButton);

        m_navigateButton->setIcon(QIcon(":/resources/svg/OpenBridge/navigation-route.svg"));
        m_navigateButton->setToolTip(tr("Navigate to waypoint"));
        connect(m_navigateButton, &QToolButton::clicked, this, &Waypoints::onNavigateCurrentClicked);
        detailsToolbarLayout->addWidget(m_navigateButton);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit waypoint"));
        connect(m_editButton, &QToolButton::clicked, this, &Waypoints::onEditClicked);
        detailsToolbarLayout->addWidget(m_editButton);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &Waypoints::onSaveClicked);
        detailsToolbarLayout->addWidget(m_saveButton);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &Waypoints::onCancelClicked);
        detailsToolbarLayout->addWidget(m_cancelButton);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete waypoint"));
        connect(m_deleteButton, &QToolButton::clicked, this, &Waypoints::onDeleteClicked);
        detailsToolbarLayout->addWidget(m_deleteButton);
        detailsToolbarLayout->addStretch(1);

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
        m_descriptionEdit->setTabChangesFocus(true);
        m_contactsEdit->setReadOnly(true);
        m_searchEdit->setStyleSheet(kLineEditStyle);
        m_nameEdit->setStyleSheet(kLineEditStyle);
        m_descriptionEdit->setStyleSheet(kPlainTextStyle);
        m_typeEdit->setStyleSheet(kLineEditStyle);
        m_latitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_longitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_altitudeSpinBox->setStyleSheet(kSpinBoxStyle);
        m_contactsEdit->setStyleSheet(kPlainTextStyle);
        m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));
        m_propertiesEditor->setHiddenKeys({QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("contacts")});
        m_previewWidget->setFreeboardEnabled(false);
        m_previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        formLayout->addRow(tr("Id"), m_idValueLabel);
        auto *nameTypeWidget = new QWidget(formWidget);
        auto *nameTypeLayout = new QHBoxLayout(nameTypeWidget);
        nameTypeLayout->setContentsMargins(0, 0, 0, 0);
        nameTypeLayout->setSpacing(6);
        auto *typeLabel = new QLabel(tr("Type"), nameTypeWidget);
        nameTypeLayout->addWidget(m_nameEdit, 1);
        nameTypeLayout->addWidget(typeLabel);
        nameTypeLayout->addWidget(m_typeEdit);
        formLayout->addRow(tr("Name"), nameTypeWidget);
        formLayout->addRow(tr("Description"), m_descriptionEdit);
        auto *positionWidget = new QWidget(formWidget);
        auto *positionLayout = new QHBoxLayout(positionWidget);
        positionLayout->setContentsMargins(0, 0, 0, 0);
        positionLayout->setSpacing(6);
        auto *latitudeLabel = new QLabel(tr("Lat"), positionWidget);
        auto *longitudeLabel = new QLabel(tr("Lon"), positionWidget);
        auto *altitudeLabel = new QLabel(tr("Alt"), positionWidget);
        positionLayout->addWidget(latitudeLabel);
        positionLayout->addWidget(m_latitudeSpinBox, 1);
        positionLayout->addWidget(longitudeLabel);
        positionLayout->addWidget(m_longitudeSpinBox, 1);
        positionLayout->addWidget(altitudeLabel);
        positionLayout->addWidget(m_altitudeSpinBox, 1);
        formLayout->addRow(tr("Position"), positionWidget);
        formLayout->addRow(m_contactsLabel, m_contactsEdit);
        formLayout->addRow(tr("Feature properties"), m_propertiesEditor);
        formLayout->addRow(tr("Timestamp"), m_timestampValueLabel);

        m_stackedWidget->addWidget(m_listPage);
        m_stackedWidget->addWidget(m_detailsPage);
        showListPage();

        connect(m_model, &QAbstractItemModel::modelReset, this, &Waypoints::rebuildTable);
        m_searchTimer->setSingleShot(true);
        m_searchTimer->setInterval(180);
        connect(m_searchTimer, &QTimer::timeout, this, &Waypoints::onSearchTimeout);
        rebuildTable();
    }

    void Waypoints::styleTable() {
        m_tableWidget->setStyleSheet(kTableStyle);
        m_tableWidget->verticalHeader()->setVisible(false);
    }

    QString Waypoints::currentWaypointIdFromSelection() const {
        const auto selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.isEmpty()) {
            return {};
        }

        const int row = selectedItems.first()->row();
        if (row < 0 || row >= m_visibleWaypointIds.size()) {
            return {};
        }
        return m_visibleWaypointIds.at(row);
    }

    void Waypoints::rebuildTable() {
        const int actionsColumn = m_model->columnCount();
        QStringList headers;
        for (int column = 0; column < m_model->columnCount(); ++column) {
            headers.append(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString());
        }
        headers.append(tr("Actions"));
        const QSignalBlocker blocker(m_tableWidget);
        m_tableWidget->clearContents();
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
        m_tableWidget->setRowCount(0);
        m_visibleWaypointIds.clear();
        m_searchHaystacks.clear();

        for (int row = 0; row < m_model->rowCount(); ++row) {
            const auto resource = m_model->resourceAtRow(row);
            const QString id = m_model->resourceIdAtRow(row);
            QString haystack = displayNameForWaypoint(resource) + " " + descriptionForWaypoint(resource);
            for (int column = 0; column < m_model->columnCount(); ++column) {
                haystack += " " + m_model->data(m_model->index(row, column), Qt::DisplayRole).toString();
            }

            const int visibleRow = m_tableWidget->rowCount();
            m_tableWidget->insertRow(visibleRow);
            m_visibleWaypointIds.append(id);
            m_searchHaystacks.append(haystack);

            for (int column = 0; column < m_model->columnCount(); ++column) {
                auto *item = new QTableWidgetItem(m_model->data(m_model->index(row, column), Qt::DisplayRole).toString());
                item->setForeground(QBrush(QColor("#1f2937")));
                item->setBackground(QBrush(QColor("#f7f7f4")));
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
            navigateButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-navigate-to.svg"));
            navigateButton->setToolTip(tr("Navigate to waypoint"));
            navigateButton->setProperty("waypointId", id);
            navigateButton->setStyleSheet(kActionButtonStyle);
            connect(navigateButton, &QToolButton::clicked, this, &Waypoints::onNavigateRowClicked);
            actionsLayout->addWidget(navigateButton);

            auto *detailsButton = new QToolButton(actionsWidget);
            detailsButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-details.svg"));
            detailsButton->setToolTip(tr("Waypoint details"));
            detailsButton->setProperty("waypointId", id);
            detailsButton->setStyleSheet(kActionButtonStyle);
            connect(detailsButton, &QToolButton::clicked, this, &Waypoints::onDetailsRowClicked);
            actionsLayout->addWidget(detailsButton);

            auto *editButton = new QToolButton(actionsWidget);
            editButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-edit.svg"));
            editButton->setToolTip(tr("Edit waypoint"));
            editButton->setProperty("waypointId", id);
            editButton->setStyleSheet(kActionButtonStyle);
            connect(editButton, &QToolButton::clicked, this, &Waypoints::onEditRowClicked);
            actionsLayout->addWidget(editButton);

            auto *removeButton = new QToolButton(actionsWidget);
            removeButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-remove.svg"));
            removeButton->setToolTip(tr("Remove waypoint"));
            removeButton->setProperty("waypointId", id);
            removeButton->setStyleSheet(kActionButtonStyle);
            connect(removeButton, &QToolButton::clicked, this, &Waypoints::onRemoveRowClicked);
            actionsLayout->addWidget(removeButton);

            actionsLayout->addStretch(1);
            m_tableWidget->setCellWidget(visibleRow, actionsColumn, actionsWidget);
        }

        m_tableWidget->setColumnWidth(actionsColumn, 196);
        applySearchFilter();
        updateBulkButtons();
    }

    void Waypoints::applySearchFilter() {
        const QString filter = m_searchEdit->text().trimmed();
        for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
            const bool matches = filter.isEmpty()
                    || (row < m_searchHaystacks.size() && m_searchHaystacks.at(row).contains(filter, Qt::CaseInsensitive));
            m_tableWidget->setRowHidden(row, !matches);
        }
        updateBulkButtons();
    }

    void Waypoints::showListPage() {
        m_stackedWidget->setCurrentWidget(m_listPage);
        m_isEditing = false;
        m_isCreating = false;
    }

    void Waypoints::showDetailsPage(const QString &id, const QJsonObject &resource, const bool editMode) {
        if (resource.isEmpty()) {
            clearEditor();
            m_currentWaypointId = id;
        } else {
            applyWaypointToEditor(id, resource);
        }
        setEditMode(editMode);
        m_stackedWidget->setCurrentWidget(m_detailsPage);
    }

    void Waypoints::applyWaypointToEditor(const QString &id, const QJsonObject &resource) {
        m_currentWaypointId = id;
        const auto waypointName = displayNameForWaypoint(resource);
        m_titleLabel->setText(waypointName.isEmpty() ? tr("Waypoint details") : waypointName);
        m_idValueLabel->setText(id);
        m_nameEdit->setText(waypointName);
        m_descriptionEdit->setPlainText(descriptionForWaypoint(resource));
        m_typeEdit->setText(resource["type"].toString());

        const auto coordinates = coordinateArray(resource);
        m_longitudeSpinBox->setValue(coordinates.size() > 0 ? coordinates.at(0).toDouble() : 0.0);
        m_latitudeSpinBox->setValue(coordinates.size() > 1 ? coordinates.at(1).toDouble() : 0.0);
        m_altitudeSpinBox->setValue(coordinates.size() > 2 ? coordinates.at(2).toDouble() : 0.0);

        const auto properties = featurePropertiesObject(resource);
        m_propertiesEditor->setJsonObject(properties);
        const auto contacts = properties.value("contacts").toArray();
        QStringList contactLines;
        for (const auto &contact : contacts) {
            if (contact.isString()) {
                contactLines.append(contact.toString());
            }
        }
        m_contactsEdit->setPlainText(contactLines.join('\n'));
        m_contactsLabel->setVisible(!contactLines.isEmpty());
        m_contactsEdit->setVisible(!contactLines.isEmpty());
        m_timestampValueLabel->setText(resource["timestamp"].toString());
        updatePreview(resource);
    }

    QJsonObject Waypoints::waypointFromEditor() const {
        const QString id = m_currentWaypointId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_currentWaypointId;

        QJsonObject properties = m_propertiesEditor->jsonObject();
        properties["name"] = m_nameEdit->text().trimmed();
        properties["description"] = m_descriptionEdit->toPlainText().trimmed();
        if (!m_contactsEdit->toPlainText().trimmed().isEmpty()) {
            QJsonArray contacts;
            for (const QString &line : m_contactsEdit->toPlainText().split('\n', Qt::SkipEmptyParts)) {
                contacts.append(line.trimmed());
            }
            properties["contacts"] = contacts;
        } else {
            properties.remove("contacts");
        }

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
        feature["properties"] = properties;

        QJsonObject resource;
        resource["name"] = m_nameEdit->text().trimmed();
        resource["description"] = m_descriptionEdit->toPlainText().trimmed();
        resource["type"] = m_typeEdit->text().trimmed();
        resource["feature"] = feature;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        return resource;
    }

    bool Waypoints::validateEditor(QString *message) const {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            *message = tr("Name is required.");
            return false;
        }

        bool propertiesOk = false;
        m_propertiesEditor->jsonObject(&propertiesOk, message);
        if (!propertiesOk) {
            return false;
        }

        return true;
    }

    void Waypoints::setEditMode(const bool editMode) {
        m_isEditing = editMode;

        m_nameEdit->setReadOnly(!editMode);
        m_descriptionEdit->setReadOnly(!editMode);
        m_typeEdit->setReadOnly(!editMode);
        m_latitudeSpinBox->setEnabled(editMode);
        m_longitudeSpinBox->setEnabled(editMode);
        m_altitudeSpinBox->setEnabled(editMode);
        m_contactsEdit->setReadOnly(!editMode);
        m_propertiesEditor->setEditMode(editMode);

        if (editMode) {
            setActionButtonsForEdit();
        } else {
            setActionButtonsForReadOnly();
        }
    }

    void Waypoints::setActionButtonsForReadOnly() {
        m_backButton->setVisible(true);
        m_newButton->setVisible(true);
        m_navigateButton->setVisible(!m_currentWaypointId.isEmpty());
        m_editButton->setVisible(!m_currentWaypointId.isEmpty());
        m_saveButton->setVisible(false);
        m_cancelButton->setVisible(false);
        m_deleteButton->setVisible(!m_currentWaypointId.isEmpty());
    }

    void Waypoints::setActionButtonsForEdit() {
        m_backButton->setVisible(false);
        m_newButton->setVisible(false);
        m_navigateButton->setVisible(false);
        m_editButton->setVisible(false);
        m_saveButton->setVisible(true);
        m_cancelButton->setVisible(true);
        m_deleteButton->setVisible(!m_isCreating);
    }

    void Waypoints::selectWaypoint(const QString &id) {
        for (int row = 0; row < m_visibleWaypointIds.size(); ++row) {
            if (m_visibleWaypointIds.at(row) == id) {
                m_tableWidget->selectRow(row);
                m_tableWidget->scrollToItem(m_tableWidget->item(row, 0));
                return;
            }
        }
    }

    void Waypoints::navigateToWaypoint(const QString &id, const QJsonObject &) {
        const auto client = FairWindSK::getInstance()->getSignalKClient();
        if (!client->navigateToWaypoint(waypointHref(id))) {
            showError(tr("Unable to start navigation to the selected waypoint."));
        }
    }

    void Waypoints::deleteWaypoint(const QString &id, const QString &name) {
        const auto choice = QMessageBox::question(
            this,
            tr("Waypoints"),
            tr("Delete waypoint \"%1\"?").arg(name));
        if (choice != QMessageBox::Yes) {
            return;
        }

        if (!m_model->deleteResource(id)) {
            showError(tr("Unable to delete the selected waypoint."));
            return;
        }

        showListPage();
    }

    void Waypoints::showError(const QString &message) const {
        QMessageBox::warning(const_cast<Waypoints *>(this), tr("Waypoints"), message);
    }

    void Waypoints::clearEditor() {
        m_currentWaypointId.clear();
        m_titleLabel->setText(tr("New waypoint"));
        m_idValueLabel->setText(tr("New"));
        m_nameEdit->clear();
        m_descriptionEdit->clear();
        m_typeEdit->setText("generic");
        m_latitudeSpinBox->setValue(0.0);
        m_longitudeSpinBox->setValue(0.0);
        m_altitudeSpinBox->setValue(0.0);
        m_contactsEdit->clear();
        m_contactsLabel->setVisible(false);
        m_contactsEdit->setVisible(false);
        m_propertiesEditor->setJsonObject(QJsonObject{});
        m_timestampValueLabel->setText({});
        updatePreview(waypointFromEditor());
    }

    void Waypoints::updatePreview(const QJsonObject &resource) {
        QList<QPair<QString, QJsonObject>> resources;
        resources.append({m_currentWaypointId, resource});
        m_previewWidget->setGeoJson(exportResourcesAsGeoJson(ResourceKind::Waypoint, resources));
    }

    QString Waypoints::waypointHref(const QString &id) const {
        return "/resources/waypoints/" + id;
    }

    void Waypoints::onSearchTextChanged(const QString &) {
        m_searchTimer->start();
    }

    void Waypoints::onSearchTimeout() {
        applySearchFilter();
    }

    void Waypoints::onAddClicked() {
        clearEditor();
        m_isCreating = true;
        showDetailsPage({}, QJsonObject{}, true);
    }

    void Waypoints::onImportClicked() {
        const QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Import Waypoints"),
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
        if (!importResourcesFromGeoJson(ResourceKind::Waypoint, QJsonDocument::fromJson(file.readAll()), &resources, &message)) {
            showError(message);
            return;
        }

        setBusy(true, tr("Importing waypoints..."), resources.size());
        int importedCount = 0;
        for (int index = 0; index < resources.size(); ++index) {
            const auto &entry = resources.at(index);
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
            m_progressBar->setValue(index + 1);
            QApplication::processEvents();
        }
        setBusy(false);

        if (importedCount == 0) {
            showError(tr("No waypoint could be imported from the selected GeoJSON file."));
            return;
        }

        rebuildTable();
        QMessageBox::information(this, tr("Waypoints"), tr("Imported %1 waypoint feature(s).").arg(importedCount));
    }

    void Waypoints::onExportClicked() {
        QStringList ids;
        const QString id = currentWaypointIdFromSelection();
        if (!id.isEmpty()) {
            ids.append(id);
        }

        const QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("Export Waypoints"),
            QStringLiteral("waypoints.geojson"),
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
            ids = visibleWaypointIds();
        }
        setBusy(true, tr("Exporting waypoints..."), ids.size());
        for (int index = 0; index < ids.size(); ++index) {
            const auto &currentId = ids.at(index);
            for (int row = 0; row < m_model->rowCount(); ++row) {
                if (m_model->resourceIdAtRow(row) == currentId) {
                    resources.append({currentId, m_model->resourceAtRow(row)});
                    break;
                }
            }
            m_progressBar->setValue(index + 1);
            QApplication::processEvents();
        }

        file.write(exportResourcesAsGeoJson(ResourceKind::Waypoint, resources).toJson(QJsonDocument::Indented));
        setBusy(false);
    }

    void Waypoints::onRefreshClicked() {
        m_model->reload(true);
        rebuildTable();
    }

    void Waypoints::onTableDoubleClicked(const int row, const int) {
        if (row < 0 || row >= m_visibleWaypointIds.size()) {
            return;
        }

        const QString id = m_visibleWaypointIds.at(row);
        for (int sourceRow = 0; sourceRow < m_model->rowCount(); ++sourceRow) {
            if (m_model->resourceIdAtRow(sourceRow) == id) {
                showDetailsPage(id, m_model->resourceAtRow(sourceRow), false);
                return;
            }
        }
    }

    void Waypoints::onNavigateRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("waypointId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                navigateToWaypoint(id, m_model->resourceAtRow(row));
                return;
            }
        }
    }

    void Waypoints::onDetailsRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("waypointId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                showDetailsPage(id, m_model->resourceAtRow(row), false);
                return;
            }
        }
    }

    void Waypoints::onEditRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("waypointId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                showDetailsPage(id, m_model->resourceAtRow(row), true);
                return;
            }
        }
    }

    void Waypoints::onRemoveRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const QString id = button->property("waypointId").toString();
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                deleteWaypoint(id, displayNameForWaypoint(m_model->resourceAtRow(row)));
                return;
            }
        }
    }

    void Waypoints::onSelectAllClicked() {
        m_tableWidget->clearSelection();
        for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
            if (!m_tableWidget->isRowHidden(row)) {
                m_tableWidget->selectRow(row);
            }
        }
    }

    void Waypoints::onRemoveSelectedClicked() {
        const auto ids = visibleWaypointIds();
        if (ids.isEmpty()) {
            return;
        }

        const auto choice = QMessageBox::question(
            this,
            tr("Waypoints"),
            tr("Delete %1 shown waypoint(s)?").arg(ids.size()));
        if (choice != QMessageBox::Yes) {
            return;
        }

        setBusy(true, tr("Removing waypoints..."), ids.size());
        int removed = 0;
        for (int index = 0; index < ids.size(); ++index) {
            if (m_model->deleteResource(ids.at(index))) {
                ++removed;
            }
            m_progressBar->setValue(index + 1);
            QApplication::processEvents();
        }
        setBusy(false);
        rebuildTable();
        if (removed == 0) {
            showError(tr("Unable to delete the shown waypoints."));
        }
    }

    void Waypoints::onBackClicked() {
        showListPage();
    }

    void Waypoints::onNavigateCurrentClicked() {
        if (m_currentWaypointId.isEmpty()) {
            return;
        }

        navigateToWaypoint(m_currentWaypointId, waypointFromEditor());
    }

    void Waypoints::onEditClicked() {
        if (m_currentWaypointId.isEmpty()) {
            return;
        }

        setEditMode(true);
    }

    void Waypoints::onDeleteClicked() {
        if (m_currentWaypointId.isEmpty()) {
            return;
        }

        deleteWaypoint(m_currentWaypointId, m_nameEdit->text().trimmed());
    }

    void Waypoints::onSaveClicked() {
        QString message;
        if (!validateEditor(&message)) {
            showError(message);
            return;
        }

        const auto resource = waypointFromEditor();
        QString id = m_currentWaypointId;
        if (m_isCreating) {
            id = m_model->createResource(resource);
            if (id.isEmpty()) {
                showError(tr("Unable to create the waypoint."));
                return;
            }
        } else if (!m_model->updateResource(m_currentWaypointId, resource)) {
            showError(tr("Unable to update the waypoint."));
            return;
        }

        showListPage();
        selectWaypoint(id);
        rebuildTable();
    }

    void Waypoints::onCancelClicked() {
        showListPage();
    }

    void Waypoints::setBusy(const bool busy, const QString &label, const int maximum) {
        m_isBusy = busy;
        m_searchStack->setCurrentWidget(busy ? static_cast<QWidget *>(m_progressBar) : static_cast<QWidget *>(m_searchEdit));
        if (busy) {
            m_progressBar->setFormat(label + QStringLiteral(" %v/%m"));
            m_progressBar->setRange(0, std::max(0, maximum));
            m_progressBar->setValue(0);
        }
        m_importButton->setEnabled(!busy);
        m_exportButton->setEnabled(!busy);
        m_addButton->setEnabled(!busy);
        m_selectAllButton->setEnabled(!busy);
        m_bulkRemoveButton->setEnabled(!busy && !visibleWaypointIds().isEmpty());
    }

    void Waypoints::updateBulkButtons() {
        const bool hasVisibleRows = !visibleWaypointIds().isEmpty();
        m_selectAllButton->setEnabled(!m_isBusy && hasVisibleRows);
        m_bulkRemoveButton->setEnabled(!m_isBusy && hasVisibleRows);
    }

    QStringList Waypoints::visibleWaypointIds() const {
        QStringList ids;
        for (int row = 0; row < m_visibleWaypointIds.size(); ++row) {
            if (!m_tableWidget->isRowHidden(row)) {
                ids.append(m_visibleWaypointIds.at(row));
            }
        }
        return ids;
    }
}
