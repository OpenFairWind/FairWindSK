//
// Created by Raffaele Montella on 15/02/25.
//

#include "Waypoints.hpp"

#include <algorithm>
#include <cmath>
#include <QBrush>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
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
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSizePolicy>
#include <QUuid>

#include "FairWindSK.hpp"
#include "GeoJsonUtils.hpp"
#include "JsonObjectEditorWidget.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/IconUtils.hpp"
#include "ui/web/SignalKAppView.hpp"
#include "ui_Waypoints.h"

namespace {
    constexpr int kWaypointTableBatchSize = 40;
    constexpr int kWaypointNameColumn = 0;
    constexpr int kWaypointDescriptionColumn = 1;
    constexpr int kWaypointTypeColumn = 2;
    constexpr int kWaypointLatitudeColumn = 3;
    constexpr int kWaypointLongitudeColumn = 4;
    constexpr int kWaypointTimestampColumn = 5;

    const QString kActionButtonStyle = QStringLiteral(
        "QToolButton {"
        " background: transparent;"
        " border: 1px solid rgba(127, 127, 127, 0.35);"
        " border-radius: 6px;"
        " padding: 4px;"
        " min-width: 40px;"
        " max-width: 40px;"
        " min-height: 40px;"
        " max-height: 40px;"
        " margin: 0px;"
        " }"
        "QToolButton:hover { background: rgba(127, 127, 127, 0.18); }"
        "QToolButton:pressed { background: rgba(127, 127, 127, 0.28); padding-top: 5px; padding-bottom: 3px; }");

    void processWaypointUiEvents() {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

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

    QString seaFloorTypeIconPath(const QString &type) {
        const QString normalized = type.trimmed().toLower();
        if (normalized == QLatin1String("rock")) {
            return QStringLiteral(":/resources/svg/mydata/seafloor-rock.svg");
        }
        if (normalized == QLatin1String("sand")) {
            return QStringLiteral(":/resources/svg/mydata/seafloor-sand.svg");
        }
        return QString();
    }

    QString freeboardFocusScript(const double longitude, const double latitude) {
        constexpr double pi = 3.14159265358979323846;
        const double latitudeDelta = 10.0 / 60.0;
        const double cosLatitude = std::cos((latitude * pi) / 180.0);
        const double longitudeDelta = 10.0 / (60.0 * std::max(0.2, cosLatitude));
        const double minLongitude = longitude - longitudeDelta;
        const double maxLongitude = longitude + longitudeDelta;
        const double minLatitude = latitude - latitudeDelta;
        const double maxLatitude = latitude + latitudeDelta;

        return QStringLiteral(R"(
(function() {
  const lon = %1;
  const lat = %2;
  const geoExtent = [%3, %4, %5, %6];
  function projectToMercator(longitude, latitude) {
    const x = longitude * 20037508.34 / 180.0;
    let y = Math.log(Math.tan((90.0 + latitude) * Math.PI / 360.0)) / (Math.PI / 180.0);
    y = y * 20037508.34 / 180.0;
    return [x, y];
  }
  const visited = new Set();
  const queue = [window];
  const center = projectToMercator(lon, lat);
  while (queue.length) {
    const current = queue.shift();
    if (!current || (typeof current !== 'object' && typeof current !== 'function') || visited.has(current)) {
      continue;
    }
    visited.add(current);
    try {
      const mapObject = typeof current.getView === 'function' ? current : (current.map && typeof current.map.getView === 'function' ? current.map : null);
      if (mapObject) {
        const view = mapObject.getView();
        if (view && typeof view.fit === 'function') {
          if (window.ol && window.ol.proj && typeof window.ol.proj.transformExtent === 'function') {
            view.fit(window.ol.proj.transformExtent(geoExtent, 'EPSG:4326', 'EPSG:3857'), {
              padding: [40, 40, 40, 40],
              duration: 0,
              maxZoom: 13
            });
          } else {
            view.setCenter(center);
            if (typeof view.setZoom === 'function') {
              view.setZoom(12);
            }
          }
        } else if (view && typeof view.setCenter === 'function') {
          view.setCenter(center);
          if (typeof view.setZoom === 'function') {
            view.setZoom(12);
          }
        }
        if (typeof mapObject.updateSize === 'function') {
          mapObject.updateSize();
        }
        return true;
      }
      if (current.setView && current.flyTo) {
        current.setView([lat, lon], 12, { animate: false });
        return true;
      }
    } catch (error) {}
    try {
      for (const key of Object.keys(current).slice(0, 120)) {
        const next = current[key];
        if (next && !visited.has(next)) {
          queue.push(next);
        }
      }
    } catch (error) {}
  }
  return false;
})();
)")
            .arg(longitude, 0, 'f', 8)
            .arg(latitude, 0, 'f', 8)
            .arg(minLongitude, 0, 'f', 8)
            .arg(minLatitude, 0, 'f', 8)
            .arg(maxLongitude, 0, 'f', 8)
            .arg(maxLatitude, 0, 'f', 8);
    }
}

namespace fairwindsk::ui::mydata {
    Waypoints::Waypoints(QWidget *parent)
        : QWidget(parent),
          m_model(new ResourceModel(ResourceKind::Waypoint, this)),
          ui(new ::Ui::Waypoints),
          m_propertiesEditor(new JsonObjectEditorWidget(this)),
          m_searchTimer(new QTimer(this)) {
        ui->setupUi(this);

        m_stackedWidget = ui->stackedWidget;
        m_listPage = ui->pageList;
        m_detailsPage = ui->pageDetails;
        m_searchStack = ui->stackedWidgetSearch;
        m_searchEdit = ui->lineEditSearch;
        m_progressBar = ui->progressBar;
        m_tableWidget = ui->tableWidget;
        m_addButton = ui->toolButtonAdd;
        m_importButton = ui->toolButtonImport;
        m_exportButton = ui->toolButtonExport;
        m_refreshButton = ui->toolButtonRefresh;
        m_selectAllButton = ui->toolButtonSelectAll;
        m_bulkRemoveButton = ui->toolButtonBulkRemove;
        m_backButton = ui->toolButtonBack;
        m_newButton = ui->toolButtonNew;
        m_navigateButton = ui->toolButtonNavigate;
        m_editButton = ui->toolButtonEdit;
        m_saveButton = ui->toolButtonSave;
        m_cancelButton = ui->toolButtonCancel;
        m_deleteButton = ui->toolButtonDelete;
        m_titleLabel = ui->labelTitle;
        m_detailTabs = ui->tabWidgetDetails;
        m_detailsSplitter = ui->splitterDetails;
        m_propertiesTreeTab = ui->widgetPropertiesTreeContainer;
        m_propertiesJsonTab = ui->widgetPropertiesJsonContainer;
        m_geoJsonDetailsEdit = ui->plainTextEditGeoJson;
        m_detailsFormLayout = ui->formLayoutDetails;
        m_nameEdit = ui->lineEditName;
        m_descriptionEdit = ui->plainTextEditDescription;
        m_typeEdit = ui->lineEditType;
        m_coordinateDisplayEdit = new QLineEdit(this);
        m_coordinateEditButton = new QToolButton(this);
        m_latitudeSpinBox = ui->doubleSpinBoxLatitude;
        m_longitudeSpinBox = ui->doubleSpinBoxLongitude;
        m_altitudeSpinBox = ui->doubleSpinBoxAltitude;
        m_contactsEdit = ui->plainTextEditContacts;
        m_contactsLabel = ui->labelContacts;
        m_seaFloorRowLabel = ui->labelSeaFloor;
        m_slipsRowLabel = ui->labelSlips;
        m_seaFloorWidget = ui->widgetSeaFloor;
        m_slipsWidget = ui->widgetSlips;
        m_seaFloorTypeWidget = ui->widgetSeaFloorType;
        m_seaFloorTypeLayout = ui->horizontalLayoutSeaFloorType;
        m_seaFloorMinValueLabel = ui->labelSeaFloorMinValue;
        m_seaFloorMaxValueLabel = ui->labelSeaFloorMaxValue;
        m_slipsValueLabel = ui->labelSlipsValue;

        m_searchEdit->setPlaceholderText(tr("Search waypoints"));
        connect(m_searchEdit, &QLineEdit::textChanged, this, &Waypoints::onSearchTextChanged);
        m_progressBar->setTextVisible(true);
        m_progressBar->setVisible(false);
        m_searchStack->setCurrentWidget(ui->pageSearch);
        m_searchEdit->setMaximumHeight(28);
        m_searchStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_refreshButton->setIcon(QIcon(":/resources/svg/OpenBridge/refresh-google.svg"));
        m_refreshButton->setToolTip(tr("Refresh"));
        connect(m_refreshButton, &QToolButton::clicked, this, &Waypoints::onRefreshClicked);

        m_importButton->setText(tr("Import"));
        connect(m_importButton, &QToolButton::clicked, this, &Waypoints::onImportClicked);

        m_exportButton->setText(tr("Export"));
        connect(m_exportButton, &QToolButton::clicked, this, &Waypoints::onExportClicked);

        m_addButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_addButton->setToolTip(tr("Add waypoint"));
        connect(m_addButton, &QToolButton::clicked, this, &Waypoints::onAddClicked);

        m_selectAllButton->setIcon(QIcon(":/resources/svg/OpenBridge/content-copy-google.svg"));
        m_selectAllButton->setToolTip(tr("Select all shown waypoints"));
        connect(m_selectAllButton, &QToolButton::clicked, this, &Waypoints::onSelectAllClicked);

        m_bulkRemoveButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_bulkRemoveButton->setToolTip(tr("Remove all shown waypoints"));
        connect(m_bulkRemoveButton, &QToolButton::clicked, this, &Waypoints::onRemoveSelectedClicked);

        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setColumnCount(m_model->columnCount() + 1);
        styleTable();
        configureTableColumns();
        m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &Waypoints::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::cellActivated, this, &Waypoints::onTableDoubleClicked);

        m_backButton->setIcon(QIcon(":/resources/svg/OpenBridge/arrow-left-google.svg"));
        m_backButton->setToolTip(tr("Back to list"));
        connect(m_backButton, &QToolButton::clicked, this, &Waypoints::onBackClicked);

        m_newButton->setIcon(QIcon(":/resources/svg/OpenBridge/widget-add-google.svg"));
        m_newButton->setToolTip(tr("New waypoint"));
        connect(m_newButton, &QToolButton::clicked, this, &Waypoints::onAddClicked);

        m_navigateButton->setIcon(QIcon(":/resources/svg/OpenBridge/navigation-route.svg"));
        m_navigateButton->setToolTip(tr("Navigate to waypoint"));
        connect(m_navigateButton, &QToolButton::clicked, this, &Waypoints::onNavigateCurrentClicked);

        m_editButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_editButton->setToolTip(tr("Edit waypoint"));
        connect(m_editButton, &QToolButton::clicked, this, &Waypoints::onEditClicked);

        m_saveButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_saveButton->setText(tr("Save"));
        connect(m_saveButton, &QToolButton::clicked, this, &Waypoints::onSaveClicked);

        m_cancelButton->setIcon(QIcon(":/resources/svg/OpenBridge/close-google.svg"));
        m_cancelButton->setText(tr("Cancel"));
        connect(m_cancelButton, &QToolButton::clicked, this, &Waypoints::onCancelClicked);

        m_deleteButton->setIcon(QIcon(":/resources/svg/OpenBridge/delete-google.svg"));
        m_deleteButton->setToolTip(tr("Delete waypoint"));
        connect(m_deleteButton, &QToolButton::clicked, this, &Waypoints::onDeleteClicked);

        m_titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
        ui->verticalLayoutDetails->setStretch(2, 1);
        m_detailsSplitter->setStretchFactor(0, 2);
        m_detailsSplitter->setStretchFactor(1, 1);

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        m_descriptionEdit->setTabChangesFocus(true);
        m_contactsEdit->setReadOnly(true);
        m_seaFloorWidget->setVisible(false);
        m_slipsWidget->setVisible(false);
        m_contactsEdit->setMaximumHeight(84);
        m_nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_descriptionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_typeEdit->setMaximumWidth(180);
        m_coordinateDisplayEdit->setReadOnly(true);
        m_coordinateDisplayEdit->setPlaceholderText(tr("No position"));
        m_coordinateEditButton->setIcon(QIcon(":/resources/svg/OpenBridge/edit-google.svg"));
        m_coordinateEditButton->setToolTip(tr("Edit coordinates"));
        m_contactsEdit->setMaximumHeight(72);
        m_propertiesEditor->setLabels(tr("Properties Tree"), tr("Properties JSON"));
        m_propertiesEditor->setHiddenKeys({QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("contacts"), QStringLiteral("seaFloor"), QStringLiteral("slips")});
        auto *previewLayout = new QVBoxLayout(ui->widgetPreviewHost);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(0);
        m_previewAppView = new fairwindsk::ui::web::SignalKAppView(this);
        previewLayout->addWidget(m_previewAppView);
        m_propertiesEditor->setTabBarVisible(false);
        m_geoJsonDetailsEdit->setReadOnly(true);
        m_geoJsonDetailsEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        connect(m_previewAppView, &fairwindsk::ui::web::SignalKAppView::loadFinished, this, [this](const bool ok) {
            if (ok) {
                schedulePreviewFocus();
            }
        });
        connect(m_detailTabs, &QTabWidget::currentChanged, this, [this](const int currentIndex) {
            syncDetailTabs();
            if (currentIndex == 0) {
                schedulePreviewFocus();
            }
        });
        m_seaFloorTypeLayout->setContentsMargins(0, 0, 0, 0);
        m_seaFloorTypeLayout->setSpacing(4);
        ui->labelInlineLatitude->setVisible(false);
        ui->doubleSpinBoxLatitude->setVisible(false);
        ui->labelInlineLongitude->setVisible(false);
        ui->doubleSpinBoxLongitude->setVisible(false);
        auto *positionLayout = ui->horizontalLayoutPosition;
        positionLayout->insertWidget(0, m_coordinateDisplayEdit, 1);
        positionLayout->insertWidget(1, m_coordinateEditButton);
        connect(m_coordinateEditButton, &QToolButton::clicked, this, &Waypoints::onCoordinateEditClicked);

        retintToolButtons();

        showListPage();

        connect(m_model, &QAbstractItemModel::modelReset, this, &Waypoints::rebuildTable);
        m_searchTimer->setSingleShot(true);
        m_searchTimer->setInterval(180);
        connect(m_searchTimer, &QTimer::timeout, this, &Waypoints::onSearchTimeout);
        m_rebuildTimer = new QTimer(this);
        m_rebuildTimer->setSingleShot(true);
        connect(m_rebuildTimer, &QTimer::timeout, this, &Waypoints::appendTableBatch);
        rebuildTable();
        QTimer::singleShot(0, this, &Waypoints::applyDetailSplitterRatio);
    }

    Waypoints::~Waypoints() {
        delete ui;
    }

    void Waypoints::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        applyDetailSplitterRatio();
    }

    void Waypoints::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
        }
    }

    void Waypoints::retintToolButtons() const {
        const QColor buttonIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        for (auto *button : {
                 m_refreshButton,
                 m_addButton,
                 m_selectAllButton,
                 m_bulkRemoveButton,
                 m_backButton,
                 m_newButton,
                 m_navigateButton,
                 m_editButton,
                 m_saveButton,
                 m_cancelButton,
                 m_deleteButton,
                 m_coordinateEditButton
             }) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(28, 28));
        }
    }

    void Waypoints::styleTable() {
        m_tableWidget->verticalHeader()->setVisible(false);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(62);
    }

    void Waypoints::configureTableColumns() {
        auto *waypointHeader = m_tableWidget->horizontalHeader();
        waypointHeader->setSectionResizeMode(QHeaderView::Interactive);
        waypointHeader->setSectionResizeMode(kWaypointNameColumn, QHeaderView::Stretch);
        waypointHeader->setSectionResizeMode(m_model->columnCount(), QHeaderView::Fixed);
        waypointHeader->setStretchLastSection(false);

        m_tableWidget->setColumnWidth(kWaypointDescriptionColumn, 220);
        m_tableWidget->setColumnWidth(kWaypointTypeColumn, 110);
        m_tableWidget->setColumnWidth(kWaypointLatitudeColumn, 150);
        m_tableWidget->setColumnWidth(kWaypointLongitudeColumn, 150);
        m_tableWidget->setColumnWidth(kWaypointTimestampColumn, 190);
        m_tableWidget->setColumnWidth(m_model->columnCount(), 196);
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
        if (m_rebuildTimer) {
            m_rebuildTimer->stop();
        }

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
        configureTableColumns();
        m_visibleWaypointIds.clear();
        m_searchHaystacks.clear();
        m_pendingBuildRow = 0;
        m_totalBuildRows = m_model->rowCount();
        if (m_totalBuildRows == 0) {
            setBusy(false);
            updateBulkButtons();
            return;
        }

        setBusy(true, tr("Loading waypoints..."), m_totalBuildRows);
        m_progressBar->setValue(0);
        m_rebuildTimer->start(0);
    }

    void Waypoints::appendTableBatch() {
        const int startRow = m_pendingBuildRow;
        const int endRow = std::min(m_totalBuildRows, startRow + kWaypointTableBatchSize);
        for (int row = startRow; row < endRow; ++row) {
            appendTableRow(row);
        }

        m_pendingBuildRow = endRow;
        m_progressBar->setValue(m_pendingBuildRow);

        if (m_pendingBuildRow < m_totalBuildRows) {
            m_rebuildTimer->start(0);
            return;
        }

        setBusy(false);
        updateBulkButtons();
    }

    void Waypoints::appendTableRow(const int sourceRow) {
        const int actionsColumn = m_model->columnCount();
        const auto resource = m_model->resourceAtRow(sourceRow);
        const QString id = m_model->resourceIdAtRow(sourceRow);
        QString haystack = displayNameForWaypoint(resource) + " " + descriptionForWaypoint(resource);
        const int visibleRow = m_tableWidget->rowCount();
        m_tableWidget->insertRow(visibleRow);
        m_visibleWaypointIds.append(id);
        const QColor baseColor = m_tableWidget->palette().color(QPalette::Base);
        const QColor textColor = fairwindsk::ui::bestContrastingColor(
            baseColor,
            {m_tableWidget->palette().color(QPalette::Text),
             m_tableWidget->palette().color(QPalette::WindowText),
             m_tableWidget->palette().color(QPalette::ButtonText),
             QColor(Qt::black),
             QColor(Qt::white)});

        for (int column = 0; column < m_model->columnCount(); ++column) {
            const QModelIndex modelIndex = m_model->index(sourceRow, column);
            const QString displayText = m_model->data(modelIndex, Qt::DisplayRole).toString();
            haystack += " " + displayText;

            auto *item = new QTableWidgetItem(displayText);
            item->setForeground(QBrush(textColor));
            item->setBackground(QBrush(baseColor));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            const auto alignment = m_model->data(modelIndex, Qt::TextAlignmentRole);
            if (alignment.isValid()) {
                item->setTextAlignment(static_cast<Qt::Alignment>(alignment.toInt()));
            }
            item->setData(Qt::DecorationRole, m_model->data(modelIndex, Qt::DecorationRole));
            m_tableWidget->setItem(visibleRow, column, item);
        }

        m_searchHaystacks.append(haystack);

        auto *actionsWidget = new QWidget();
        actionsWidget->setStyleSheet(QStringLiteral("QWidget { background: transparent; }"));
        auto *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(6, 4, 6, 4);
        actionsLayout->setSpacing(4);
        actionsLayout->setAlignment(Qt::AlignCenter);

        auto *navigateButton = new QToolButton(actionsWidget);
        navigateButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-navigate-to.svg"));
        navigateButton->setToolTip(tr("Navigate to waypoint"));
        navigateButton->setProperty("waypointId", id);
        navigateButton->setStyleSheet(kActionButtonStyle);
        navigateButton->setIconSize(QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(
            navigateButton,
            fairwindsk::ui::bestContrastingColor(
                m_tableWidget->palette().color(QPalette::Base),
                {m_tableWidget->palette().color(QPalette::Text),
                 m_tableWidget->palette().color(QPalette::ButtonText),
                 m_tableWidget->palette().color(QPalette::WindowText)}),
            QSize(20, 20));
        connect(navigateButton, &QToolButton::clicked, this, &Waypoints::onNavigateRowClicked);
        actionsLayout->addWidget(navigateButton);

        auto *detailsButton = new QToolButton(actionsWidget);
        detailsButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-details.svg"));
        detailsButton->setToolTip(tr("Waypoint details"));
        detailsButton->setProperty("waypointId", id);
        detailsButton->setStyleSheet(kActionButtonStyle);
        detailsButton->setIconSize(QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(
            detailsButton,
            fairwindsk::ui::bestContrastingColor(
                m_tableWidget->palette().color(QPalette::Base),
                {m_tableWidget->palette().color(QPalette::Text),
                 m_tableWidget->palette().color(QPalette::ButtonText),
                 m_tableWidget->palette().color(QPalette::WindowText)}),
            QSize(20, 20));
        connect(detailsButton, &QToolButton::clicked, this, &Waypoints::onDetailsRowClicked);
        actionsLayout->addWidget(detailsButton);

        auto *editButton = new QToolButton(actionsWidget);
        editButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-edit.svg"));
        editButton->setToolTip(tr("Edit waypoint"));
        editButton->setProperty("waypointId", id);
        editButton->setStyleSheet(kActionButtonStyle);
        editButton->setIconSize(QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(
            editButton,
            fairwindsk::ui::bestContrastingColor(
                m_tableWidget->palette().color(QPalette::Base),
                {m_tableWidget->palette().color(QPalette::Text),
                 m_tableWidget->palette().color(QPalette::ButtonText),
                 m_tableWidget->palette().color(QPalette::WindowText)}),
            QSize(20, 20));
        connect(editButton, &QToolButton::clicked, this, &Waypoints::onEditRowClicked);
        actionsLayout->addWidget(editButton);

        auto *removeButton = new QToolButton(actionsWidget);
        removeButton->setIcon(QIcon(":/resources/svg/mydata/waypoint-remove.svg"));
        removeButton->setToolTip(tr("Remove waypoint"));
        removeButton->setProperty("waypointId", id);
        removeButton->setStyleSheet(kActionButtonStyle);
        removeButton->setIconSize(QSize(20, 20));
        fairwindsk::ui::applyTintedButtonIcon(
            removeButton,
            fairwindsk::ui::bestContrastingColor(
                m_tableWidget->palette().color(QPalette::Base),
                {m_tableWidget->palette().color(QPalette::Text),
                 m_tableWidget->palette().color(QPalette::ButtonText),
                 m_tableWidget->palette().color(QPalette::WindowText)}),
            QSize(20, 20));
        connect(removeButton, &QToolButton::clicked, this, &Waypoints::onRemoveRowClicked);
        actionsLayout->addWidget(removeButton);

        m_tableWidget->setCellWidget(visibleRow, actionsColumn, actionsWidget);
        m_tableWidget->setRowHeight(visibleRow, 50);

        const QString filter = m_searchEdit->text().trimmed();
        const bool matches = filter.isEmpty() || haystack.contains(filter, Qt::CaseInsensitive);
        m_tableWidget->setRowHidden(visibleRow, !matches);
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

    void Waypoints::syncDetailTabs() {
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

    void Waypoints::updateSpecialFieldVisibility() {
        m_contactsLabel->setVisible(m_contactsEdit->isVisible());
        m_seaFloorRowLabel->setVisible(m_seaFloorWidget->isVisible());
        m_slipsRowLabel->setVisible(m_slipsWidget->isVisible());
    }

    void Waypoints::rebuildSeaFloorTypeIcons(const QJsonArray &types) {
        while (m_seaFloorTypeLayout->count() > 0) {
            auto *item = m_seaFloorTypeLayout->takeAt(0);
            if (auto *widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }

        for (const auto &value : types) {
            if (!value.isString()) {
                continue;
            }

            const QString label = value.toString().trimmed();
            auto *iconLabel = new QLabel(m_seaFloorTypeWidget);
            const QString iconPath = seaFloorTypeIconPath(label);
            if (!iconPath.isEmpty()) {
                iconLabel->setPixmap(QIcon(iconPath).pixmap(18, 18));
            } else {
                iconLabel->setText(label.left(1).toUpper());
                iconLabel->setStyleSheet("QLabel { border: 1px solid rgba(127, 127, 127, 0.35); border-radius: 9px; min-width: 18px; min-height: 18px; padding: 0 4px; }");
                iconLabel->setAlignment(Qt::AlignCenter);
            }
            iconLabel->setToolTip(label);
            m_seaFloorTypeLayout->addWidget(iconLabel);
        }

        m_seaFloorTypeWidget->setVisible(m_seaFloorTypeLayout->count() > 0);
    }

    void Waypoints::showDetailsPage(const QString &id, const QJsonObject &resource, const bool editMode) {
        if (resource.isEmpty()) {
            clearEditor();
            m_currentWaypointId = id;
        } else {
            applyWaypointToEditor(id, resource);
        }
        m_detailTabs->setCurrentIndex(0);
        setEditMode(editMode);
        m_stackedWidget->setCurrentWidget(m_detailsPage);
        QTimer::singleShot(0, this, &Waypoints::applyDetailSplitterRatio);
        QTimer::singleShot(0, this, &Waypoints::schedulePreviewFocus);
    }

    void Waypoints::applyWaypointToEditor(const QString &id, const QJsonObject &resource) {
        m_currentWaypointId = id;
        const auto waypointName = displayNameForWaypoint(resource);
        m_titleLabel->setText(waypointName.isEmpty() ? tr("Waypoint details") : waypointName);
        m_nameEdit->setText(waypointName);
        m_descriptionEdit->setPlainText(descriptionForWaypoint(resource));
        m_typeEdit->setText(resource["type"].toString());

        const auto coordinates = coordinateArray(resource);
        m_longitudeSpinBox->setValue(coordinates.size() > 0 ? coordinates.at(0).toDouble() : 0.0);
        m_latitudeSpinBox->setValue(coordinates.size() > 1 ? coordinates.at(1).toDouble() : 0.0);
        m_altitudeSpinBox->setValue(coordinates.size() > 2 ? coordinates.at(2).toDouble() : 0.0);
        updateCoordinateDisplay();

        const auto properties = featurePropertiesObject(resource);
        m_propertiesEditor->setJsonObject(properties);
        const auto seaFloor = properties.value("seaFloor").toObject();
        rebuildSeaFloorTypeIcons(seaFloor.value("type").toArray());
        const bool hasSeaFloor = !seaFloor.isEmpty();
        m_seaFloorWidget->setVisible(hasSeaFloor);
        m_seaFloorMinValueLabel->setText(seaFloor.contains("minDepth") ? QString::number(seaFloor.value("minDepth").toDouble()) : QString());
        m_seaFloorMaxValueLabel->setText(seaFloor.contains("maxDepth") ? QString::number(seaFloor.value("maxDepth").toDouble()) : QString());
        const auto slipsValue = properties.value("slips");
        const bool hasSlips = !slipsValue.isUndefined() && !slipsValue.isNull();
        m_slipsWidget->setVisible(hasSlips);
        m_slipsValueLabel->setText(hasSlips ? slipsValue.toVariant().toString() : QString());
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
        updateSpecialFieldVisibility();
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
        m_coordinateEditButton->setEnabled(editMode);
        m_altitudeSpinBox->setEnabled(editMode);
        m_contactsEdit->setReadOnly(!editMode);
        m_propertiesEditor->setEditMode(editMode);
        updateCoordinateDisplay();

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
        const auto choice = drawer::question(
            this,
            tr("Waypoints"),
            tr("Delete waypoint \"%1\"?").arg(name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
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
        drawer::warning(const_cast<Waypoints *>(this), tr("Waypoints"), message);
    }

    void Waypoints::clearEditor() {
        m_currentWaypointId.clear();
        m_titleLabel->setText(tr("New waypoint"));
        m_nameEdit->clear();
        m_descriptionEdit->clear();
        m_typeEdit->setText("generic");
        m_latitudeSpinBox->setValue(0.0);
        m_longitudeSpinBox->setValue(0.0);
        m_altitudeSpinBox->setValue(0.0);
        m_coordinateDisplayEdit->clear();
        m_contactsEdit->clear();
        m_contactsLabel->setVisible(false);
        m_contactsEdit->setVisible(false);
        m_seaFloorWidget->setVisible(false);
        rebuildSeaFloorTypeIcons(QJsonArray{});
        m_slipsWidget->setVisible(false);
        m_slipsValueLabel->clear();
        updateSpecialFieldVisibility();
        m_propertiesEditor->setJsonObject(QJsonObject{});
        updatePreview(waypointFromEditor());
    }

    void Waypoints::updateCoordinateDisplay() {
        const auto configuration = FairWindSK::getInstance()->getConfiguration();
        m_coordinateDisplayEdit->setText(
            fairwindsk::ui::geo::formatCoordinate(
                m_latitudeSpinBox->value(),
                m_longitudeSpinBox->value(),
                configuration->getCoordinateFormat()));
    }

    void Waypoints::updatePreview(const QJsonObject &resource) {
        QList<QPair<QString, QJsonObject>> resources;
        resources.append({m_currentWaypointId, resource});
        const auto geoJson = exportResourcesAsGeoJson(ResourceKind::Waypoint, resources);
        const auto coordinates = coordinateArray(resource);
        const double longitude = coordinates.size() > 0 ? coordinates.at(0).toDouble() : 0.0;
        const double latitude = coordinates.size() > 1 ? coordinates.at(1).toDouble() : 0.0;
        m_previewLongitude = longitude;
        m_previewLatitude = latitude;
        if (m_previewAppView->url().isEmpty() || !m_previewAppView->url().toString().contains(QStringLiteral("freeboard"), Qt::CaseInsensitive)) {
            if (!m_previewAppView->loadAppByKeyword(QStringLiteral("freeboard"))) {
                m_previewAppView->setHtml(QStringLiteral(
                    "<html><body style=\"margin:0;background:#07111b;color:#e5edf7;font-family:sans-serif;display:flex;align-items:center;justify-content:center;\">"
                    "<div>Freeboard-SK is not installed or not active on the current Signal K server.</div>"
                    "</body></html>"
                ));
            }
        } else {
            schedulePreviewFocus();
        }
        m_geoJsonDetailsEdit->setPlainText(QString::fromUtf8(geoJson.toJson(QJsonDocument::Indented)));
        syncDetailTabs();
    }

    void Waypoints::applyDetailSplitterRatio() {
        if (!m_detailsSplitter || m_detailsSplitter->width() <= 0) {
            return;
        }

        const int leftWidth = (m_detailsSplitter->width() * 2) / 3;
        const int rightWidth = std::max(1, m_detailsSplitter->width() - leftWidth);
        m_detailsSplitter->setSizes({leftWidth, rightWidth});
    }

    void Waypoints::schedulePreviewFocus() {
        applyPreviewFocus();
        QTimer::singleShot(600, this, &Waypoints::applyPreviewFocus);
        QTimer::singleShot(1500, this, &Waypoints::applyPreviewFocus);
        QTimer::singleShot(3000, this, &Waypoints::applyPreviewFocus);
    }

    void Waypoints::applyPreviewFocus() {
        if (!m_previewAppView || !m_previewAppView->page() || m_detailTabs->currentIndex() != 0) {
            return;
        }

        m_previewAppView->runJavaScript(freeboardFocusScript(m_previewLongitude, m_previewLatitude));
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
        const QString fileName = drawer::getOpenFilePath(
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
            processWaypointUiEvents();
        }
        setBusy(false);

        if (importedCount == 0) {
            showError(tr("No waypoint could be imported from the selected GeoJSON file."));
            return;
        }

        rebuildTable();
        drawer::information(this, tr("Waypoints"), tr("Imported %1 waypoint feature(s).").arg(importedCount));
    }

    void Waypoints::onExportClicked() {
        QStringList ids;
        const QString id = currentWaypointIdFromSelection();
        if (!id.isEmpty()) {
            ids.append(id);
        }

        const QString fileName = drawer::getSaveFilePath(
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
            processWaypointUiEvents();
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

        const auto choice = drawer::question(
            this,
            tr("Waypoints"),
            tr("Delete %1 shown waypoint(s)?").arg(ids.size()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
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
            processWaypointUiEvents();
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

    void Waypoints::onCoordinateEditClicked() {
        if (!(m_isEditing || m_isCreating)) {
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
        updatePreview(waypointFromEditor());
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
        m_searchStack->setCurrentWidget(busy ? static_cast<QWidget *>(ui->pageProgress) : static_cast<QWidget *>(ui->pageSearch));
        if (busy) {
            m_progressBar->setFormat(label + QStringLiteral(" %v/%m"));
            m_progressBar->setRange(0, std::max(0, maximum));
            m_progressBar->setValue(0);
        }
        m_importButton->setEnabled(!busy);
        m_exportButton->setEnabled(!busy);
        m_refreshButton->setEnabled(!busy);
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
