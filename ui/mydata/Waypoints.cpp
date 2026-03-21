//
// Created by Raffaele Montella on 15/02/25.
//

#include "Waypoints.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QUuid>

#include "FairWindSK.hpp"
#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"

namespace {
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
    class WaypointTableProxyModel final : public QSortFilterProxyModel {
    public:
        explicit WaypointTableProxyModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent) {}

        int columnCount(const QModelIndex &parent = QModelIndex()) const override {
            const int baseCount = QSortFilterProxyModel::columnCount(parent);
            return parent.isValid() ? 0 : baseCount + 2;
        }

        QVariant data(const QModelIndex &index, const int role = Qt::DisplayRole) const override {
            const int baseCount = QSortFilterProxyModel::columnCount();
            if (index.column() >= baseCount) {
                return {};
            }
            return QSortFilterProxyModel::data(index, role);
        }

        QVariant headerData(const int section, const Qt::Orientation orientation, const int role) const override {
            const int baseCount = QSortFilterProxyModel::columnCount();
            if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= baseCount) {
                return section == baseCount ? QObject::tr("Navigate") : QObject::tr("Details");
            }
            return QSortFilterProxyModel::headerData(section, orientation, role);
        }

        bool lessThan(const QModelIndex &left, const QModelIndex &right) const override {
            const int baseCount = QSortFilterProxyModel::columnCount();
            if (left.column() >= baseCount || right.column() >= baseCount) {
                return left.row() < right.row();
            }
            return QSortFilterProxyModel::lessThan(left, right);
        }
    };

    Waypoints::Waypoints(QWidget *parent)
        : QWidget(parent),
          m_model(new ResourceModel(ResourceKind::Waypoint, this)),
          m_proxyModel(new WaypointTableProxyModel(this)),
          m_stackedWidget(new QStackedWidget(this)),
          m_listPage(new QWidget(this)),
          m_detailsPage(new QWidget(this)),
          m_searchEdit(new QLineEdit(this)),
          m_tableView(new QTableView(this)),
          m_addButton(new QToolButton(this)),
          m_importButton(new QToolButton(this)),
          m_exportButton(new QToolButton(this)),
          m_refreshButton(new QToolButton(this)),
          m_backButton(new QToolButton(this)),
          m_newButton(new QToolButton(this)),
          m_navigateButton(new QToolButton(this)),
          m_editButton(new QToolButton(this)),
          m_saveButton(new QToolButton(this)),
          m_cancelButton(new QToolButton(this)),
          m_deleteButton(new QToolButton(this)),
          m_titleLabel(new QLabel(this)),
          m_nameEdit(new QLineEdit(this)),
          m_descriptionEdit(new QLineEdit(this)),
          m_typeEdit(new QLineEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
          m_propertiesEdit(new QPlainTextEdit(this)),
          m_previewWidget(new GeoJsonPreviewWidget(this)),
          m_idValueLabel(new QLabel(this)),
          m_timestampValueLabel(new QLabel(this)) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(m_stackedWidget);

        auto *listLayout = new QVBoxLayout(m_listPage);
        auto *toolbarLayout = new QHBoxLayout();
        listLayout->addLayout(toolbarLayout);

        m_searchEdit->setPlaceholderText(tr("Search waypoints"));
        connect(m_searchEdit, &QLineEdit::textChanged, this, &Waypoints::onSearchTextChanged);
        toolbarLayout->addWidget(m_searchEdit, 1);

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

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setFilterKeyColumn(-1);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxyModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setSortingEnabled(true);
        m_tableView->sortByColumn(0, Qt::AscendingOrder);
        m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_tableView->horizontalHeader()->setStretchLastSection(false);
        connect(m_tableView, &QTableView::doubleClicked, this, &Waypoints::onTableDoubleClicked);
        listLayout->addWidget(m_tableView);

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

        auto *formLayout = new QFormLayout();
        detailsLayout->addLayout(formLayout);

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        m_propertiesEdit->setPlaceholderText("{\n  \"color\": \"red\"\n}");

        formLayout->addRow(tr("Id"), m_idValueLabel);
        formLayout->addRow(tr("Name"), m_nameEdit);
        formLayout->addRow(tr("Description"), m_descriptionEdit);
        formLayout->addRow(tr("Type"), m_typeEdit);
        formLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
        formLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
        formLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
        formLayout->addRow(tr("Feature properties (JSON)"), m_propertiesEdit);
        formLayout->addRow(tr("Timestamp"), m_timestampValueLabel);
        detailsLayout->addWidget(m_previewWidget, 1);

        m_stackedWidget->addWidget(m_listPage);
        m_stackedWidget->addWidget(m_detailsPage);
        showListPage();

        connect(m_model, &QAbstractItemModel::modelReset, this, &Waypoints::updateActionButtons);
        connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, &Waypoints::updateActionButtons);
        connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &Waypoints::updateActionButtons);
        connect(m_tableView->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &Waypoints::updateActionButtons);

        updateActionButtons();
    }

    QModelIndex Waypoints::currentSourceIndex() const {
        return sourceIndexForProxyRow(m_tableView->currentIndex().row());
    }

    QModelIndex Waypoints::sourceIndexForProxyRow(const int proxyRow) const {
        if (proxyRow < 0) {
            return {};
        }
        const auto proxyIndex = m_proxyModel->index(proxyRow, 0);
        return proxyIndex.isValid() ? m_proxyModel->mapToSource(proxyIndex) : QModelIndex{};
    }

    void Waypoints::updateActionButtons() {
        const int navigateColumn = m_model->columnCount();
        const int detailsColumn = navigateColumn + 1;

        m_tableView->setColumnWidth(navigateColumn, 56);
        m_tableView->setColumnWidth(detailsColumn, 56);

        for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
            const auto sourceIndex = sourceIndexForProxyRow(row);
            if (!sourceIndex.isValid()) {
                continue;
            }

            const QString id = m_model->resourceIdAtRow(sourceIndex.row());

            auto *navigateButton = new QToolButton(m_tableView);
            navigateButton->setAutoRaise(true);
            navigateButton->setIcon(QIcon(":/resources/svg/OpenBridge/navigation-route.svg"));
            navigateButton->setToolTip(tr("Navigate to waypoint"));
            navigateButton->setProperty("waypointId", id);
            connect(navigateButton, &QToolButton::clicked, this, &Waypoints::onNavigateRowClicked);
            m_tableView->setIndexWidget(m_proxyModel->index(row, navigateColumn), navigateButton);

            auto *detailsButton = new QToolButton(m_tableView);
            detailsButton->setAutoRaise(true);
            detailsButton->setIcon(QIcon(":/resources/svg/gui-view-show-svgrepo-com.svg"));
            detailsButton->setToolTip(tr("Show details"));
            detailsButton->setProperty("waypointId", id);
            connect(detailsButton, &QToolButton::clicked, this, &Waypoints::onShowDetailsRowClicked);
            m_tableView->setIndexWidget(m_proxyModel->index(row, detailsColumn), detailsButton);
        }
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
        m_descriptionEdit->setText(descriptionForWaypoint(resource));
        m_typeEdit->setText(resource["type"].toString());

        const auto coordinates = coordinateArray(resource);
        m_longitudeSpinBox->setValue(coordinates.size() > 0 ? coordinates.at(0).toDouble() : 0.0);
        m_latitudeSpinBox->setValue(coordinates.size() > 1 ? coordinates.at(1).toDouble() : 0.0);
        m_altitudeSpinBox->setValue(coordinates.size() > 2 ? coordinates.at(2).toDouble() : 0.0);

        const auto properties = featurePropertiesObject(resource);
        m_propertiesEdit->setPlainText(QString::fromUtf8(QJsonDocument(properties).toJson(QJsonDocument::Indented)));
        m_timestampValueLabel->setText(resource["timestamp"].toString());
        updatePreview(resource);
    }

    QJsonObject Waypoints::waypointFromEditor() const {
        const QString id = m_currentWaypointId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_currentWaypointId;

        const auto propertiesDocument = QJsonDocument::fromJson(m_propertiesEdit->toPlainText().trimmed().toUtf8());
        QJsonObject properties = propertiesDocument.isObject() ? propertiesDocument.object() : QJsonObject{};
        properties["name"] = m_nameEdit->text().trimmed();
        properties["description"] = m_descriptionEdit->text().trimmed();

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
        resource["description"] = m_descriptionEdit->text().trimmed();
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

        const auto propertiesText = m_propertiesEdit->toPlainText().trimmed();
        if (!propertiesText.isEmpty()) {
            const auto document = QJsonDocument::fromJson(propertiesText.toUtf8());
            if (!document.isObject()) {
                *message = tr("Feature properties must be a valid JSON object.");
                return false;
            }
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
        m_propertiesEdit->setReadOnly(!editMode);

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
        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == id) {
                const auto sourceIndex = m_model->index(row, 0);
                const auto proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
                if (proxyIndex.isValid()) {
                    m_tableView->selectRow(proxyIndex.row());
                    m_tableView->scrollTo(proxyIndex);
                }
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
        m_propertiesEdit->clear();
        m_timestampValueLabel->setText({});
        updatePreview(waypointFromEditor());
    }

    void Waypoints::updatePreview(const QJsonObject &resource) {
        QList<QPair<QString, QJsonObject>> resources;
        resources.append({m_currentWaypointId, resource});
        m_previewWidget->setGeoJson(exportResourcesAsGeoJson(ResourceKind::Waypoint, resources), tr("Waypoint GeoJSON Preview"));
    }

    QString Waypoints::waypointHref(const QString &id) const {
        return "/resources/waypoints/" + id;
    }

    void Waypoints::onSearchTextChanged(const QString &text) {
        const QRegularExpression expression(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption);
        m_proxyModel->setFilterRegularExpression(expression);
        updateActionButtons();
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
            showError(tr("No waypoint could be imported from the selected GeoJSON file."));
            return;
        }

        QMessageBox::information(this, tr("Waypoints"), tr("Imported %1 waypoint feature(s).").arg(importedCount));
    }

    void Waypoints::onExportClicked() {
        QStringList ids;
        const auto index = currentSourceIndex();
        if (index.isValid()) {
            ids.append(m_model->resourceIdAtRow(index.row()));
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

        file.write(exportResourcesAsGeoJson(ResourceKind::Waypoint, resources).toJson(QJsonDocument::Indented));
    }

    void Waypoints::onRefreshClicked() {
        m_model->reload(true);
    }

    void Waypoints::onTableDoubleClicked(const QModelIndex &index) {
        const auto sourceIndex = sourceIndexForProxyRow(index.row());
        if (!sourceIndex.isValid()) {
            return;
        }

        const auto id = m_model->resourceIdAtRow(sourceIndex.row());
        showDetailsPage(id, m_model->resourceAtRow(sourceIndex.row()), false);
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

    void Waypoints::onShowDetailsRowClicked() {
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
    }

    void Waypoints::onCancelClicked() {
        showListPage();
    }
}
