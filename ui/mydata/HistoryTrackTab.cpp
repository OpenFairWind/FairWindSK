//
// Created by Codex on 21/03/26.
//

#include "HistoryTrackTab.hpp"

#include <QBrush>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSizePolicy>

#include "GeoJsonPreviewWidget.hpp"
#include "GeoJsonUtils.hpp"
#include "FairWindSK.hpp"
#include "HistoryTrackModel.hpp"
#include "JsonObjectEditorWidget.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui_HistoryTrackTab.h"

namespace {}

namespace fairwindsk::ui::mydata {

    HistoryTrackTab::HistoryTrackTab(QWidget *parent)
        : QWidget(parent),
          ui(new ::Ui::HistoryTrackTab),
          m_model(new HistoryTrackModel(this)),
          m_refreshTimer(new QTimer(this)) {
        ui->setupUi(this);

        m_stackedWidget = ui->stackedWidget;
        m_listPage = ui->pageList;
        m_detailsPage = ui->pageDetails;
        m_statusLabel = ui->labelStatus;
        m_titleLabel = ui->labelTitle;
        m_indexValueLabel = ui->labelSampleValue;
        m_durationCombo = ui->comboBoxDuration;
        m_tableWidget = ui->tableWidget;
        m_refreshButton = ui->toolButtonRefresh;
        m_importButton = ui->toolButtonImport;
        m_exportButton = ui->toolButtonExport;
        m_backButton = ui->toolButtonBack;
        m_newButton = ui->toolButtonNew;
        m_editButton = ui->toolButtonEdit;
        m_saveButton = ui->toolButtonSave;
        m_cancelButton = ui->toolButtonCancel;
        m_deleteButton = ui->toolButtonDelete;
        m_timestampEdit = ui->dateTimeEditTimestamp;
        m_latitudeSpinBox = ui->doubleSpinBoxLatitude;
        m_longitudeSpinBox = ui->doubleSpinBoxLongitude;
        m_altitudeSpinBox = ui->doubleSpinBoxAltitude;
        m_propertiesEditor = ui->jsonObjectEditorProperties;
        m_previewWidget = ui->geoJsonPreviewWidget;

        m_durationCombo->addItem(tr("Last hour"), "PT1H");
        m_durationCombo->addItem(tr("Last 6 hours"), "PT6H");
        m_durationCombo->addItem(tr("Last 24 hours"), "P1D");
        m_durationCombo->addItem(tr("Last 7 days"), "P7D");
        m_durationCombo->setMaximumHeight(28);
        m_durationCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        connect(m_durationCombo,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &HistoryTrackTab::onDurationChanged);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &HistoryTrackTab::onRefreshClicked);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &HistoryTrackTab::onImportClicked);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &HistoryTrackTab::onExportClicked);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New sample"));
        connect(m_newButton, &QToolButton::clicked, this, &HistoryTrackTab::onAddClicked);

        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setColumnCount(m_model->columnCount() + 1);
        styleTable();
        configureTableColumns();
        m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &HistoryTrackTab::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::cellActivated, this, &HistoryTrackTab::onTableDoubleClicked);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &HistoryTrackTab::onBackClicked);

        auto *detailsNewButton = ui->toolButtonNewDetails;
        detailsNewButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        detailsNewButton->setToolTip(tr("New sample"));
        connect(detailsNewButton, &QToolButton::clicked, this, &HistoryTrackTab::onAddClicked);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit"));
        connect(m_editButton, &QToolButton::clicked, this, &HistoryTrackTab::onEditClicked);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &HistoryTrackTab::onSaveClicked);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &HistoryTrackTab::onCancelClicked);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete"));
        connect(m_deleteButton, &QToolButton::clicked, this, &HistoryTrackTab::onDeleteClicked);

        retintToolButtons();

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette());
        ui->splitterDetails->setStretchFactor(0, 1);
        ui->splitterDetails->setStretchFactor(1, 1);

        m_timestampEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        m_timestampEdit->setCalendarPopup(true);
        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));

        connect(m_refreshTimer, &QTimer::timeout, this, &HistoryTrackTab::onRefreshClicked);
        m_refreshTimer->start(5000);

        connect(m_model, &QAbstractItemModel::modelReset, this, &HistoryTrackTab::rebuildTable);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &HistoryTrackTab::rebuildTable);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &HistoryTrackTab::rebuildTable);
        showListPage();
        onRefreshClicked();
    }

    HistoryTrackTab::~HistoryTrackTab() {
        m_isShuttingDown = true;
        if (m_refreshTimer) {
            m_refreshTimer->stop();
            disconnect(m_refreshTimer, nullptr, this, nullptr);
        }
        disconnect(m_model, nullptr, this, nullptr);
        delete ui;
    }

    void HistoryTrackTab::styleTable() {
        m_tableWidget->horizontalHeader()->setVisible(true);
        m_tableWidget->verticalHeader()->setVisible(false);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(62);
    }

    void HistoryTrackTab::configureTableColumns() {
        auto *trackHeader = m_tableWidget->horizontalHeader();
        trackHeader->setVisible(true);
        trackHeader->setMinimumSectionSize(96);
        trackHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        trackHeader->setSectionResizeMode(0, QHeaderView::Stretch);
        if (m_model->columnCount() > 1) {
            m_tableWidget->setColumnWidth(1, 140);
            m_tableWidget->setColumnWidth(2, 140);
        }
        trackHeader->setSectionResizeMode(m_model->columnCount(), QHeaderView::Fixed);
        trackHeader->setStretchLastSection(false);
    }

    void HistoryTrackTab::rebuildTable() {
        const int actionsColumn = m_model->columnCount();
        QStringList headers;
        for (int column = 0; column < m_model->columnCount(); ++column) {
            headers.append(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString());
        }
        headers.append(tr("Actions"));
        m_tableWidget->clearContents();
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
        m_tableWidget->setRowCount(0);
        configureTableColumns();
        m_visibleRows.clear();

        for (int row = 0; row < m_model->rowCount(); ++row) {
            const int visibleRow = m_tableWidget->rowCount();
            m_tableWidget->insertRow(visibleRow);
            m_visibleRows.append(row);
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
            navigateButton->setToolTip(tr("Navigate is not available for track samples"));
            navigateButton->setEnabled(false);
            navigateButton->setProperty("trackRow", row);
            connect(navigateButton, &QToolButton::clicked, this, &HistoryTrackTab::onNavigateRowClicked);
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
            editButton->setToolTip(tr("Edit track sample"));
            editButton->setProperty("trackRow", row);
            connect(editButton, &QToolButton::clicked, this, &HistoryTrackTab::onEditRowClicked);
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
            removeButton->setToolTip(tr("Remove track sample"));
            removeButton->setProperty("trackRow", row);
            connect(removeButton, &QToolButton::clicked, this, &HistoryTrackTab::onRemoveRowClicked);
            actionsLayout->addWidget(removeButton);

            actionsLayout->addStretch(1);
            m_tableWidget->setCellWidget(visibleRow, actionsColumn, actionsWidget);
        }

        m_tableWidget->setColumnWidth(actionsColumn, 152);
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
        return selectedSourceRow();
    }

    void HistoryTrackTab::onRefreshClicked() {
        if (m_isShuttingDown || !m_model) {
            return;
        }

        QString message;
        m_model->reload(currentDuration(), currentResolution(), &message);
        updateStatus(message);
        rebuildTable();
        if (m_stackedWidget->currentWidget() == m_detailsPage) {
            updatePreview();
        }
    }

    void HistoryTrackTab::onDurationChanged() {
        onRefreshClicked();
    }

    void HistoryTrackTab::onImportClicked() {
        const QString fileName = drawer::getOpenFilePath(
                this,
                tr("Import Tracks"),
                QString(),
                tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to open %1.").arg(fileName));
            return;
        }

        QList<HistoryTrackPoint> points;
        QString message;
        if (!importTrackPointsFromGeoJson(QJsonDocument::fromJson(file.readAll()), &points, &message)) {
            drawer::warning(this, tr("Tracks"), message);
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
        const QString fileName = drawer::getSaveFilePath(
                this,
                tr("Export Tracks"),
                QString("tracks-history.geojson"),
                tr("GeoJSON files (*.geojson *.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to write %1.").arg(fileName));
            return;
        }

        file.write(exportTrackPointsAsGeoJson(allPoints()).toJson(QJsonDocument::Indented));
    }

    void HistoryTrackTab::onOpenClicked() {
        const int row = currentRow();
        if (row < 0) {
            drawer::warning(this, tr("Tracks"), tr("Select a track sample first."));
            return;
        }

        showDetailsPage(row, false);
    }

    int HistoryTrackTab::selectedSourceRow() const {
        const auto selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.isEmpty()) {
            return -1;
        }
        const int row = selectedItems.first()->row();
        return row >= 0 && row < m_visibleRows.size() ? m_visibleRows.at(row) : -1;
    }

    void HistoryTrackTab::onTableDoubleClicked(const int row, const int) {
        if (row < 0 || row >= m_visibleRows.size()) {
            return;
        }

        showDetailsPage(m_visibleRows.at(row), false);
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

    void HistoryTrackTab::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
            fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette());
        }
    }

    void HistoryTrackTab::retintToolButtons() const {
        const QColor buttonIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        for (auto *button : {
                 m_refreshButton,
                 m_newButton,
                 m_backButton,
                 ui->toolButtonNewDetails,
                 m_editButton,
                 m_saveButton,
                 m_cancelButton,
                 m_deleteButton
             }) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(28, 28));
        }
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
            drawer::warning(this, tr("Tracks"), message);
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
            drawer::warning(this, tr("Tracks"), tr("Please enter a valid timestamp and coordinates."));
            return;
        }

        if (m_isCreating) {
            m_model->appendPoint(point);
        } else if (!m_model->updatePointAtRow(m_currentRow, point)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to update the selected sample."));
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

        if (drawer::question(this,
                             tr("Delete sample"),
                             tr("Delete the selected track sample?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        if (!m_model->removePointAtRow(m_currentRow)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to delete the selected sample."));
            return;
        }

        updateStatusLabel();
        updatePreview();
        showListPage();
    }
}
