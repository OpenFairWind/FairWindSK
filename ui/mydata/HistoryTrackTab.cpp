//
// Created by Codex on 21/03/26.
//

#include "HistoryTrackTab.hpp"

#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIdentityProxyModel>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QComboBox>

#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"
#include "HistoryTrackModel.hpp"
#include "JsonObjectEditorWidget.hpp"

namespace fairwindsk::ui::mydata {
    class HistoryTrackTableProxyModel final : public QIdentityProxyModel {
    public:
        explicit HistoryTrackTableProxyModel(QObject *parent = nullptr)
            : QIdentityProxyModel(parent) {}

        int columnCount(const QModelIndex &parent = QModelIndex()) const override {
            const int baseCount = QIdentityProxyModel::columnCount(parent);
            return parent.isValid() ? 0 : baseCount + 1;
        }

        QVariant data(const QModelIndex &index, const int role = Qt::DisplayRole) const override {
            const int baseCount = QIdentityProxyModel::columnCount();
            if (index.column() >= baseCount) {
                return {};
            }
            return QIdentityProxyModel::data(index, role);
        }

        QVariant headerData(const int section, const Qt::Orientation orientation, const int role) const override {
            const int baseCount = QIdentityProxyModel::columnCount();
            if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= baseCount) {
                return QObject::tr("Actions");
            }
            return QIdentityProxyModel::headerData(section, orientation, role);
        }
    };

    HistoryTrackTab::HistoryTrackTab(QWidget *parent)
        : QWidget(parent),
          m_model(new HistoryTrackModel(this)),
          m_proxyModel(new HistoryTrackTableProxyModel(this)),
          m_stackedWidget(new QStackedWidget(this)),
          m_listPage(new QWidget(this)),
          m_detailsPage(new QWidget(this)),
          m_statusLabel(new QLabel(this)),
          m_titleLabel(new QLabel(this)),
          m_indexValueLabel(new QLabel(this)),
          m_durationCombo(new QComboBox(this)),
          m_tableView(new QTableView(this)),
          m_refreshButton(new QToolButton(this)),
          m_importButton(new QToolButton(this)),
          m_exportButton(new QToolButton(this)),
          m_backButton(new QToolButton(this)),
          m_newButton(new QToolButton(this)),
          m_editButton(new QToolButton(this)),
          m_saveButton(new QToolButton(this)),
          m_cancelButton(new QToolButton(this)),
          m_deleteButton(new QToolButton(this)),
          m_timestampEdit(new QDateTimeEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
          m_propertiesEditor(new JsonObjectEditorWidget(this)),
          m_previewWidget(new GeoJsonPreviewWidget(this)),
          m_refreshTimer(new QTimer(this)) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(m_stackedWidget);

        auto *listLayout = new QVBoxLayout(m_listPage);
        auto *toolbarLayout = new QHBoxLayout();
        listLayout->addLayout(toolbarLayout);

        m_durationCombo->addItem(tr("Last hour"), "PT1H");
        m_durationCombo->addItem(tr("Last 6 hours"), "PT6H");
        m_durationCombo->addItem(tr("Last 24 hours"), "P1D");
        m_durationCombo->addItem(tr("Last 7 days"), "P7D");
        connect(m_durationCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &HistoryTrackTab::onDurationChanged);
        toolbarLayout->addWidget(m_durationCombo);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &HistoryTrackTab::onRefreshClicked);
        toolbarLayout->addWidget(m_refreshButton);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &HistoryTrackTab::onImportClicked);
        toolbarLayout->addWidget(m_importButton);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &HistoryTrackTab::onExportClicked);
        toolbarLayout->addWidget(m_exportButton);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New sample"));
        connect(m_newButton, &QToolButton::clicked, this, &HistoryTrackTab::onAddClicked);
        toolbarLayout->addWidget(m_newButton);

        toolbarLayout->addStretch(1);
        toolbarLayout->addWidget(m_statusLabel, 1);

        m_proxyModel->setSourceModel(m_model);
        m_tableView->setModel(m_proxyModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setSortingEnabled(false);
        auto *trackHeader = m_tableView->horizontalHeader();
        trackHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        if (m_model->columnCount() > 1) {
            trackHeader->setSectionResizeMode(m_model->columnCount() - 2, QHeaderView::Stretch);
        }
        trackHeader->setSectionResizeMode(m_model->columnCount(), QHeaderView::Fixed);
        trackHeader->setStretchLastSection(false);
        connect(m_tableView, &QTableView::doubleClicked, this, &HistoryTrackTab::onTableDoubleClicked);
        connect(m_tableView, &QTableView::activated, this, &HistoryTrackTab::onTableDoubleClicked);
        listLayout->addWidget(m_tableView);

        auto *detailsLayout = new QVBoxLayout(m_detailsPage);
        auto *detailsToolbar = new QHBoxLayout();
        detailsLayout->addLayout(detailsToolbar);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &HistoryTrackTab::onBackClicked);
        detailsToolbar->addWidget(m_backButton);

        auto *detailsNewButton = new QToolButton(this);
        detailsNewButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        detailsNewButton->setToolTip(tr("New sample"));
        connect(detailsNewButton, &QToolButton::clicked, this, &HistoryTrackTab::onAddClicked);
        detailsToolbar->addWidget(detailsNewButton);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit"));
        connect(m_editButton, &QToolButton::clicked, this, &HistoryTrackTab::onEditClicked);
        detailsToolbar->addWidget(m_editButton);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &HistoryTrackTab::onSaveClicked);
        detailsToolbar->addWidget(m_saveButton);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &HistoryTrackTab::onCancelClicked);
        detailsToolbar->addWidget(m_cancelButton);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete"));
        connect(m_deleteButton, &QToolButton::clicked, this, &HistoryTrackTab::onDeleteClicked);
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

        m_timestampEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        m_timestampEdit->setCalendarPopup(true);
        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        const QString inputStyle =
            QStringLiteral("QAbstractSpinBox { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_timestampEdit->setStyleSheet(inputStyle);
        m_latitudeSpinBox->setStyleSheet(inputStyle);
        m_longitudeSpinBox->setStyleSheet(inputStyle);
        m_altitudeSpinBox->setStyleSheet(inputStyle);
        m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));

        formLayout->addRow(tr("Sample"), m_indexValueLabel);
        formLayout->addRow(tr("Timestamp"), m_timestampEdit);
        formLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
        formLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
        formLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
        formLayout->addRow(tr("Feature properties"), m_propertiesEditor);

        m_stackedWidget->addWidget(m_listPage);
        m_stackedWidget->addWidget(m_detailsPage);

        connect(m_refreshTimer, &QTimer::timeout, this, &HistoryTrackTab::onRefreshClicked);
        m_refreshTimer->start(5000);

        connect(m_model, &QAbstractItemModel::modelReset, this, &HistoryTrackTab::updateActionButtons);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &HistoryTrackTab::updateActionButtons);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &HistoryTrackTab::updateActionButtons);
        showListPage();
        onRefreshClicked();
    }

    QModelIndex HistoryTrackTab::proxyIndexForRow(const int row, const int column) const {
        return row >= 0 ? m_proxyModel->index(row, column) : QModelIndex{};
    }

    void HistoryTrackTab::clearActionWidgets() {
        const int actionsColumn = m_model->columnCount();
        const int visibleColumns = m_proxyModel->columnCount();
        for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
            for (int column = 0; column < visibleColumns; ++column) {
                if (column == actionsColumn) {
                    continue;
                }
                if (QWidget *widget = m_tableView->indexWidget(proxyIndexForRow(row, column))) {
                    m_tableView->setIndexWidget(proxyIndexForRow(row, column), nullptr);
                    widget->deleteLater();
                }
            }
            if (QWidget *widget = m_tableView->indexWidget(proxyIndexForRow(row, actionsColumn))) {
                m_tableView->setIndexWidget(proxyIndexForRow(row, actionsColumn), nullptr);
                widget->deleteLater();
            }
        }
    }

    void HistoryTrackTab::updateActionButtons() {
        const int actionsColumn = m_model->columnCount();
        clearActionWidgets();
        m_tableView->setColumnWidth(actionsColumn, 152);

        for (int row = 0; row < m_model->rowCount(); ++row) {
            auto *actionsWidget = new QWidget();
            auto *actionsLayout = new QHBoxLayout(actionsWidget);
            actionsLayout->setContentsMargins(4, 0, 4, 0);
            actionsLayout->setSpacing(2);

            auto *navigateButton = new QToolButton(actionsWidget);
            navigateButton->setAutoRaise(true);
            navigateButton->setIcon(QIcon(":/resources/svg/OpenBridge/navigation-route.svg"));
            navigateButton->setToolTip(tr("Navigate is not available for track samples"));
            navigateButton->setEnabled(false);
            navigateButton->setProperty("trackRow", row);
            connect(navigateButton, &QToolButton::clicked, this, &HistoryTrackTab::onNavigateRowClicked);
            actionsLayout->addWidget(navigateButton);

            auto *editButton = new QToolButton(actionsWidget);
            editButton->setAutoRaise(true);
            editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
            editButton->setToolTip(tr("Edit track sample"));
            editButton->setProperty("trackRow", row);
            connect(editButton, &QToolButton::clicked, this, &HistoryTrackTab::onEditRowClicked);
            actionsLayout->addWidget(editButton);

            auto *removeButton = new QToolButton(actionsWidget);
            removeButton->setAutoRaise(true);
            removeButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
            removeButton->setToolTip(tr("Remove track sample"));
            removeButton->setProperty("trackRow", row);
            connect(removeButton, &QToolButton::clicked, this, &HistoryTrackTab::onRemoveRowClicked);
            actionsLayout->addWidget(removeButton);

            actionsLayout->addStretch(1);
            m_tableView->setIndexWidget(proxyIndexForRow(row, actionsColumn), actionsWidget);
        }
    }

    QString HistoryTrackTab::currentDuration() const {
        return m_durationCombo->currentData().toString();
    }

    QString HistoryTrackTab::currentResolution() const {
        const QString duration = currentDuration();
        if (duration == "PT1H") {
            return "PT1M";
        }
        if (duration == "PT6H") {
            return "PT5M";
        }
        if (duration == "P7D") {
            return "PT30M";
        }
        return "PT10M";
    }

    void HistoryTrackTab::updateStatus(const QString &message) {
        m_statusLabel->setText(message);
    }

    void HistoryTrackTab::updateStatusLabel() {
        updateStatus(m_model->hasPoints()
                             ? tr("Loaded %1 track samples.").arg(m_model->rowCount())
                             : tr("No track samples are available from the History API."));
    }

    QList<HistoryTrackPoint> HistoryTrackTab::allPoints() const {
        QList<HistoryTrackPoint> points;
        for (int row = 0; row < m_model->rowCount(); ++row) {
            points.append(m_model->pointAtRow(row));
        }
        return points;
    }

    void HistoryTrackTab::updatePreview() {
        const auto points = allPoints();
        if (points.isEmpty()) {
            m_previewWidget->setMessage(tr("No track samples are available for preview."));
            return;
        }
        m_previewWidget->setGeoJson(exportTrackPointsAsGeoJson(points));
    }

    void HistoryTrackTab::showListPage() {
        m_stackedWidget->setCurrentWidget(m_listPage);
        m_currentRow = -1;
        m_isEditing = false;
        m_isCreating = false;
        m_refreshTimer->start(5000);
    }

    void HistoryTrackTab::showDetailsPage(const int row, const bool editMode) {
        if (row >= 0) {
            populateEditor(row);
        } else {
            clearEditor();
        }

        setEditMode(editMode);
        m_stackedWidget->setCurrentWidget(m_detailsPage);
        m_refreshTimer->stop();
        updatePreview();
    }

    void HistoryTrackTab::setEditMode(const bool editMode) {
        m_isEditing = editMode;

        m_timestampEdit->setReadOnly(!editMode);
        m_timestampEdit->setButtonSymbols(editMode ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
        m_latitudeSpinBox->setEnabled(editMode);
        m_longitudeSpinBox->setEnabled(editMode);
        m_altitudeSpinBox->setEnabled(editMode);
        m_propertiesEditor->setEditMode(editMode);

        m_backButton->setVisible(!editMode);
        m_editButton->setVisible(!editMode && !m_isCreating && m_currentRow >= 0);
        m_saveButton->setVisible(editMode);
        m_cancelButton->setVisible(editMode);
        m_deleteButton->setVisible(!m_isCreating && m_currentRow >= 0);
        m_titleLabel->setText(m_isCreating ? tr("New track sample") :
                              (m_currentRow >= 0 ? tr("Track sample %1").arg(m_currentRow + 1) : tr("Track sample")));
    }

    void HistoryTrackTab::populateEditor(const int row) {
        const auto point = m_model->pointAtRow(row);
        m_currentRow = row;
        m_isCreating = false;
        m_indexValueLabel->setText(QString::number(row + 1));
        m_timestampEdit->setDateTime(point.timestamp.toLocalTime());
        m_latitudeSpinBox->setValue(point.coordinate.latitude());
        m_longitudeSpinBox->setValue(point.coordinate.longitude());
        m_altitudeSpinBox->setValue(point.coordinate.altitude());
        QJsonObject properties;
        properties["sampleIndex"] = row + 1;
        properties["timestamp"] = point.timestamp.toUTC().toString(Qt::ISODateWithMs);
        properties["latitude"] = point.coordinate.latitude();
        properties["longitude"] = point.coordinate.longitude();
        properties["altitude"] = point.coordinate.altitude();
        m_propertiesEditor->setJsonObject(properties);
        updatePreview();
    }

    void HistoryTrackTab::clearEditor() {
        m_currentRow = -1;
        m_isCreating = true;
        m_indexValueLabel->setText(tr("New"));
        m_timestampEdit->setDateTime(QDateTime::currentDateTime());
        m_latitudeSpinBox->setValue(0.0);
        m_longitudeSpinBox->setValue(0.0);
        m_altitudeSpinBox->setValue(0.0);
        QJsonObject properties;
        properties["sampleIndex"] = 0;
        properties["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        properties["latitude"] = 0.0;
        properties["longitude"] = 0.0;
        properties["altitude"] = 0.0;
        m_propertiesEditor->setJsonObject(properties);
        updatePreview();
    }

    int HistoryTrackTab::currentRow() const {
        const QModelIndex currentIndex = m_tableView->currentIndex();
        return currentIndex.isValid() ? m_proxyModel->mapToSource(currentIndex).row() : -1;
    }

    void HistoryTrackTab::onRefreshClicked() {
        QString message;
        m_model->reload(currentDuration(), currentResolution(), &message);
        updateStatus(message);
        if (m_stackedWidget->currentWidget() == m_detailsPage) {
            updatePreview();
        }
    }

    void HistoryTrackTab::onDurationChanged() {
        onRefreshClicked();
    }

    void HistoryTrackTab::onImportClicked() {
        const QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Import Tracks"),
                QString(),
                tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to open %1.").arg(fileName));
            return;
        }

        QList<HistoryTrackPoint> points;
        QString message;
        if (!importTrackPointsFromGeoJson(QJsonDocument::fromJson(file.readAll()), &points, &message)) {
            QMessageBox::warning(this, tr("Tracks"), message);
            return;
        }

        while (m_model->rowCount() > 0) {
            m_model->removePointAtRow(m_model->rowCount() - 1);
        }
        for (const auto &point : points) {
            m_model->appendPoint(point);
        }

        updateStatus(message);
        updatePreview();
    }

    void HistoryTrackTab::onExportClicked() {
        const QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Export Tracks"),
                QString("tracks-history.geojson"),
                tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to write %1.").arg(fileName));
            return;
        }

        file.write(exportTrackPointsAsGeoJson(allPoints()).toJson(QJsonDocument::Indented));
    }

    void HistoryTrackTab::onOpenClicked() {
        const int row = currentRow();
        if (row < 0) {
            QMessageBox::warning(this, tr("Tracks"), tr("Select a track sample first."));
            return;
        }

        showDetailsPage(row, false);
    }

    void HistoryTrackTab::onTableDoubleClicked(const QModelIndex &index) {
        if (!index.isValid()) {
            return;
        }

        showDetailsPage(m_proxyModel->mapToSource(index).row(), false);
    }

    void HistoryTrackTab::onNavigateRowClicked() {
    }

    void HistoryTrackTab::onEditRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        showDetailsPage(button->property("trackRow").toInt(), true);
    }

    void HistoryTrackTab::onRemoveRowClicked() {
        const auto *button = qobject_cast<QToolButton *>(sender());
        if (!button) {
            return;
        }

        const int row = button->property("trackRow").toInt();
        if (row < 0 || row >= m_model->rowCount()) {
            return;
        }

        m_currentRow = row;
        onDeleteClicked();
    }

    void HistoryTrackTab::onBackClicked() {
        showListPage();
    }

    void HistoryTrackTab::onAddClicked() {
        showDetailsPage(-1, true);
    }

    void HistoryTrackTab::onEditClicked() {
        if (m_currentRow < 0) {
            return;
        }

        setEditMode(true);
    }

    void HistoryTrackTab::onSaveClicked() {
        bool propertiesOk = false;
        QString message;
        const auto properties = m_propertiesEditor->jsonObject(&propertiesOk, &message);
        if (!propertiesOk) {
            QMessageBox::warning(this, tr("Tracks"), message);
            return;
        }

        HistoryTrackPoint point;
        point.timestamp = QDateTime::fromString(properties["timestamp"].toString(), Qt::ISODateWithMs);
        if (!point.timestamp.isValid()) {
            point.timestamp = m_timestampEdit->dateTime().toUTC();
        }
        point.coordinate.setLatitude(properties.contains("latitude") ? properties["latitude"].toDouble() : m_latitudeSpinBox->value());
        point.coordinate.setLongitude(properties.contains("longitude") ? properties["longitude"].toDouble() : m_longitudeSpinBox->value());
        point.coordinate.setAltitude(properties.contains("altitude") ? properties["altitude"].toDouble() : m_altitudeSpinBox->value());

        if (!point.timestamp.isValid() || !point.coordinate.isValid()) {
            QMessageBox::warning(this, tr("Tracks"), tr("Please enter a valid timestamp and coordinates."));
            return;
        }

        if (m_isCreating) {
            m_model->appendPoint(point);
        } else if (!m_model->updatePointAtRow(m_currentRow, point)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to update the selected sample."));
            return;
        }

        updateStatusLabel();
        updatePreview();
        showListPage();
    }

    void HistoryTrackTab::onCancelClicked() {
        showListPage();
    }

    void HistoryTrackTab::onDeleteClicked() {
        if (m_currentRow < 0) {
            return;
        }

        if (QMessageBox::question(this,
                                  tr("Delete sample"),
                                  tr("Delete the selected track sample?")) != QMessageBox::Yes) {
            return;
        }

        if (!m_model->removePointAtRow(m_currentRow)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to delete the selected sample."));
            return;
        }

        updateStatusLabel();
        updatePreview();
        showListPage();
    }
}
