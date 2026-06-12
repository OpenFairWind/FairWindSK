//
// Created by Codex on 21/03/26.
//

#include "HistoryTrackTab.hpp"

#include <QAbstractSpinBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include "FairWindSK.hpp"
#include "GeoJsonUtils.hpp"
#include "HistoryTrackModel.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui_HistoryTrackTab.h"

namespace {
    constexpr int kTouchButtonHeight = 58;
    constexpr int kTouchRowHeight = 64;
    constexpr bool kUseSafeTextOnlyToolbarOnThisPlatform =
#if defined(Q_OS_LINUX) && (defined(__arm__) || defined(__aarch64__))
        true;
#else
        false;
#endif

    QString touchToolbarButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors, const bool accent = false) {
        const QColor top = accent ? colors.accentTop : colors.buttonBackground.lighter(112);
        const QColor mid = accent ? colors.accentTop.darker(103) : colors.buttonBackground;
        const QColor bottom = accent ? colors.accentBottom : colors.buttonBackground.darker(118);
        const QColor border = accent ? colors.accentBottom : colors.border;
        return QStringLiteral(
            "QPushButton {"
            " min-width: 58px;"
            " max-width: 58px;"
            " min-height: 58px;"
            " max-height: 58px;"
            " padding: 0px;"
            " border-radius: 16px;"
            " border: 1px solid %1;"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 %2, stop:0.5 %3, stop:1 %4);"
            " }"
            "QPushButton:hover { border-color: %5; }"
            "QPushButton:pressed { background: %6; }"
            "QPushButton:disabled { background: %7; border-color: %8; }")
            .arg(border.name(),
                 top.name(),
                 mid.name(),
                 bottom.name(),
                 colors.accentTop.name(),
                 colors.pressedBackground.name(),
                 colors.window.darker(104).name(),
                 colors.border.darker(130).name());
    }

    QString touchTableStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &baseColor, const QColor &panelColor) {
        return QStringLiteral(
            "QTableWidget {"
            " background: %1;"
            " color: %2;"
            " alternate-background-color: %3;"
            " border: 1px solid %4;"
            " border-radius: 16px;"
            " gridline-color: transparent;"
            " outline: none;"
            " font-size: 18px;"
            " }"
            "QTableWidget::item { padding: 10px; }"
            "QTableWidget::item:selected { background: %5; color: %6; }"
            "QHeaderView::section {"
            " min-height: 48px;"
            " padding: 0 12px;"
            " background: %7;"
            " color: %8;"
            " border: none;"
            " border-bottom: 1px solid %4;"
            " font-size: 16px;"
            " font-weight: 700;"
            " }")
            .arg(baseColor.name(),
                 colors.text.name(),
                 panelColor.name(),
                 colors.border.name(),
                 colors.accentTop.name(),
                 colors.accentText.name(),
                 panelColor.darker(104).name(),
                 colors.text.name());
    }

    bool editTrackPointDialog(QWidget *parent,
                              const QString &title,
                              fairwindsk::ui::mydata::HistoryTrackPoint *point,
                              const bool readOnly) {
        if (!point) {
            return false;
        }

        QDateTime currentTimestamp = point->timestamp.toLocalTime();
        double currentLatitude = point->coordinate.latitude();
        double currentLongitude = point->coordinate.longitude();
        double currentAltitude = std::isnan(point->coordinate.altitude()) ? 0.0 : point->coordinate.altitude();

        while (true) {
            auto *content = new QWidget();
            auto *layout = new QVBoxLayout(content);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(12);

            auto *formLayout = new QFormLayout();
            formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
            formLayout->setHorizontalSpacing(10);
            formLayout->setVerticalSpacing(10);

            auto *timestampEdit = new QDateTimeEdit(currentTimestamp, content);
            timestampEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            timestampEdit->setCalendarPopup(false);
            timestampEdit->setMinimumHeight(58);
            timestampEdit->setReadOnly(readOnly);
            timestampEdit->setButtonSymbols(readOnly ? QAbstractSpinBox::NoButtons : QAbstractSpinBox::UpDownArrows);

            auto *latitudeSpinBox = new QDoubleSpinBox(content);
            latitudeSpinBox->setRange(-90.0, 90.0);
            latitudeSpinBox->setDecimals(8);
            latitudeSpinBox->setValue(currentLatitude);
            latitudeSpinBox->setMinimumHeight(58);
            latitudeSpinBox->setEnabled(!readOnly);

            auto *longitudeSpinBox = new QDoubleSpinBox(content);
            longitudeSpinBox->setRange(-180.0, 180.0);
            longitudeSpinBox->setDecimals(8);
            longitudeSpinBox->setValue(currentLongitude);
            longitudeSpinBox->setMinimumHeight(58);
            longitudeSpinBox->setEnabled(!readOnly);

            auto *altitudeSpinBox = new QDoubleSpinBox(content);
            altitudeSpinBox->setRange(-100000.0, 100000.0);
            altitudeSpinBox->setDecimals(2);
            altitudeSpinBox->setValue(currentAltitude);
            altitudeSpinBox->setMinimumHeight(58);
            altitudeSpinBox->setEnabled(!readOnly);

            formLayout->addRow(QObject::tr("Timestamp"), timestampEdit);
            formLayout->addRow(QObject::tr("Latitude"), latitudeSpinBox);
            formLayout->addRow(QObject::tr("Longitude"), longitudeSpinBox);
            formLayout->addRow(QObject::tr("Altitude"), altitudeSpinBox);
            layout->addLayout(formLayout);

            QPointer<QDateTimeEdit> timestampGuard(timestampEdit);
            QPointer<QDoubleSpinBox> latitudeGuard(latitudeSpinBox);
            QPointer<QDoubleSpinBox> longitudeGuard(longitudeSpinBox);
            QPointer<QDoubleSpinBox> altitudeGuard(altitudeSpinBox);

            const int result = fairwindsk::ui::drawer::execDrawer(
                parent,
                title,
                content,
                readOnly
                    ? QList<fairwindsk::ui::drawer::ButtonSpec>{{QObject::tr("Close"), int(QMessageBox::Close), true}}
                    : QList<fairwindsk::ui::drawer::ButtonSpec>{{QObject::tr("OK"), int(QMessageBox::Ok), true},
                                                                {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}},
                readOnly ? int(QMessageBox::Close) : int(QMessageBox::Cancel));

            if (readOnly || result != QMessageBox::Ok) {
                return false;
            }

            if (!timestampGuard || !latitudeGuard || !longitudeGuard || !altitudeGuard) {
                return false;
            }

            currentTimestamp = timestampGuard->dateTime();
            currentLatitude = latitudeGuard->value();
            currentLongitude = longitudeGuard->value();
            currentAltitude = altitudeGuard->value();

            QGeoCoordinate coordinate;
            coordinate.setLatitude(currentLatitude);
            coordinate.setLongitude(currentLongitude);
            coordinate.setAltitude(currentAltitude);
            if (!coordinate.isValid()) {
                fairwindsk::ui::drawer::warning(parent,
                                                QObject::tr("Invalid Coordinates"),
                                                QObject::tr("Please enter valid latitude and longitude values."));
                continue;
            }

            point->timestamp = currentTimestamp.toUTC();
            point->coordinate = coordinate;
            return true;
        }
    }
}

namespace fairwindsk::ui::mydata {

    HistoryTrackTab::HistoryTrackTab(QWidget *parent)
        : QWidget(parent),
          ui(new ::Ui::HistoryTrackTab),
          m_model(new HistoryTrackModel(this)),
          m_refreshTimer(new QTimer(this)) {
        ui->setupUi(this);

        m_titleLabel = ui->labelTitle;
        m_statusLabel = ui->labelStatus;
        m_durationCombo = ui->comboBoxDuration;
        m_tableWidget = ui->tableWidget;
        m_openButton = ui->toolButtonOpen;
        m_editButton = ui->toolButtonEdit;
        m_addButton = ui->toolButtonAdd;
        m_deleteButton = ui->toolButtonDelete;
        m_importButton = ui->toolButtonImport;
        m_exportButton = ui->toolButtonExport;
        m_refreshButton = ui->toolButtonRefresh;

        m_durationCombo->addItem(tr("Last hour"), QStringLiteral("PT1H"));
        m_durationCombo->addItem(tr("Last 6 hours"), QStringLiteral("PT6H"));
        m_durationCombo->addItem(tr("Last 24 hours"), QStringLiteral("P1D"));
        m_durationCombo->addItem(tr("Last 7 days"), QStringLiteral("P7D"));
        m_durationCombo->setMinimumHeight(kTouchButtonHeight);
        m_durationCombo->setMaximumHeight(kTouchButtonHeight);

        m_openButton->setText(QString());
        m_editButton->setText(QString());
        m_addButton->setText(QString());
        m_deleteButton->setText(QString());
        m_importButton->setText(QString());
        m_exportButton->setText(QString());
        m_refreshButton->setText(QString());
        m_openButton->setToolTip(tr("Open"));
        m_editButton->setToolTip(tr("Edit"));
        m_addButton->setToolTip(tr("New"));
        m_deleteButton->setToolTip(tr("Delete"));
        m_importButton->setToolTip(tr("Import"));
        m_exportButton->setToolTip(tr("Export"));
        m_refreshButton->setToolTip(tr("Refresh"));

        if (!kUseSafeTextOnlyToolbarOnThisPlatform) {
            m_openButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg")));
            m_editButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/edit-google.svg")));
            m_addButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/widget-add-google.svg")));
            m_deleteButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg")));
            m_importButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/route-import-iec.svg")));
            m_exportButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/file-export-google.svg")));
            m_refreshButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg")));
        }

        const QList<QPushButton *> buttons = {
            m_openButton, m_editButton, m_addButton, m_deleteButton, m_importButton, m_exportButton, m_refreshButton
        };
        for (QPushButton *button : buttons) {
            button->setFlat(false);
            button->setIconSize(QSize(28, 28));
            button->setMinimumHeight(kTouchButtonHeight);
            button->setMinimumWidth(kTouchButtonHeight);
        }

        configureTable();
        applyTouchFriendlyStyling();

        connect(m_durationCombo,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &HistoryTrackTab::onDurationChanged);
        connect(m_openButton, &QPushButton::clicked, this, &HistoryTrackTab::onOpenClicked);
        connect(m_editButton, &QPushButton::clicked, this, &HistoryTrackTab::onEditClicked);
        connect(m_addButton, &QPushButton::clicked, this, &HistoryTrackTab::onAddClicked);
        connect(m_deleteButton, &QPushButton::clicked, this, &HistoryTrackTab::onDeleteClicked);
        connect(m_importButton, &QPushButton::clicked, this, &HistoryTrackTab::onImportClicked);
        connect(m_exportButton, &QPushButton::clicked, this, &HistoryTrackTab::onExportClicked);
        connect(m_refreshButton, &QPushButton::clicked, this, &HistoryTrackTab::onRefreshClicked);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &HistoryTrackTab::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &HistoryTrackTab::updateActionState);

        connect(m_model, &QAbstractItemModel::modelReset, this, &HistoryTrackTab::rebuildTable);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &HistoryTrackTab::rebuildTable);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &HistoryTrackTab::rebuildTable);

        connect(m_refreshTimer, &QTimer::timeout, this, &HistoryTrackTab::onRefreshClicked);
        m_refreshTimer->start(5000);

        updateStatus(tr("No track samples are available from the History API."));
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

    void HistoryTrackTab::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::StyleChange) {
            applyTouchFriendlyStyling();
        }
    }

    void HistoryTrackTab::configureTable() {
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setWordWrap(false);
        m_tableWidget->setSortingEnabled(!kUseSafeTextOnlyToolbarOnThisPlatform);
        m_tableWidget->verticalHeader()->setVisible(false);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(kTouchRowHeight);
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
        if (kUseSafeTextOnlyToolbarOnThisPlatform) {
            m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        } else {
            m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            for (int column = 1; column < m_model->columnCount(); ++column) {
                m_tableWidget->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            }
        }
    }

    void HistoryTrackTab::applyTouchFriendlyStyling() {
        if (!m_tableWidget || !m_titleLabel || !m_statusLabel) {
            return;
        }

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QColor panelColor = colors.window.darker(104);
        const QColor baseColor = colors.window.lighter(106);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 20.0);
        m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 15px; }").arg(colors.text.name()));
        m_tableWidget->setStyleSheet(touchTableStyle(colors, baseColor, panelColor));
        m_durationCombo->setAccentButton(true);

        const QList<QPushButton *> buttons = {
            m_openButton, m_editButton, m_addButton, m_deleteButton, m_importButton, m_exportButton, m_refreshButton
        };
        for (QPushButton *button : buttons) {
            const bool accent = button == m_addButton || button == m_openButton;
            button->setStyleSheet(touchToolbarButtonStyle(colors, accent));
            fairwindsk::ui::applyTintedButtonIcon(button, accent ? colors.accentText : colors.buttonText, QSize(28, 28));
        }
    }

    void HistoryTrackTab::rebuildTable() {
        QStringList headers;
        for (int column = 0; column < m_model->columnCount(); ++column) {
            headers.append(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString());
        }

        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->clearContents();
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
        m_tableWidget->setRowCount(0);

        for (int row = 0; row < m_model->rowCount(); ++row) {
            const int visibleRow = m_tableWidget->rowCount();
            m_tableWidget->insertRow(visibleRow);
            m_tableWidget->setRowHeight(visibleRow, kTouchRowHeight);

            for (int column = 0; column < m_model->columnCount(); ++column) {
                auto *item = new QTableWidgetItem(m_model->data(m_model->index(row, column), Qt::DisplayRole).toString());
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                const QVariant alignment = m_model->data(m_model->index(row, column), Qt::TextAlignmentRole);
                if (alignment.isValid()) {
                    item->setTextAlignment(static_cast<Qt::Alignment>(alignment.toInt()));
                }
                m_tableWidget->setItem(visibleRow, column, item);
            }
        }

        if (!kUseSafeTextOnlyToolbarOnThisPlatform) {
            m_tableWidget->setSortingEnabled(true);
            m_tableWidget->sortByColumn(0, Qt::DescendingOrder);
        }

        updateActionState();
    }

    void HistoryTrackTab::onRefreshClicked() {
        if (m_isShuttingDown || !m_model) {
            return;
        }

        QString message;
        m_model->reload(currentDuration(), currentResolution(), &message);
        updateStatus(message);
        rebuildTable();
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
        rebuildTable();
    }

    void HistoryTrackTab::onExportClicked() {
        const QString fileName = drawer::getSaveFilePath(
            this,
            tr("Export Tracks"),
            QStringLiteral("tracks-history.geojson"),
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
        const int row = selectedSourceRow();
        if (row < 0) {
            drawer::warning(this, tr("Tracks"), tr("Select a track sample first."));
            return;
        }

        HistoryTrackPoint point = m_model->pointAtRow(row);
        editTrackPointDialog(this, tr("Track Sample"), &point, true);
    }

    void HistoryTrackTab::onEditClicked() {
        const int row = selectedSourceRow();
        if (row < 0) {
            drawer::warning(this, tr("Tracks"), tr("Select a track sample first."));
            return;
        }

        editTrackPoint(row, false);
    }

    void HistoryTrackTab::onAddClicked() {
        editTrackPoint(-1, true);
    }

    void HistoryTrackTab::onDeleteClicked() {
        const int row = selectedSourceRow();
        if (row < 0) {
            return;
        }

        if (drawer::question(this,
                             tr("Delete Sample"),
                             tr("Delete the selected track sample?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        if (!m_model->removePointAtRow(row)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to delete the selected sample."));
            return;
        }

        updateStatusLabel();
        rebuildTable();
    }

    void HistoryTrackTab::onTableDoubleClicked(const int row, const int column) {
        Q_UNUSED(row)
        Q_UNUSED(column)
        onOpenClicked();
    }

    void HistoryTrackTab::updateActionState() {
        const bool hasSelection = selectedSourceRow() >= 0;
        m_openButton->setEnabled(hasSelection);
        m_editButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
        m_exportButton->setEnabled(m_model->rowCount() > 0);
    }

    void HistoryTrackTab::updateStatus(const QString &message) {
        m_statusLabel->setText(message);
    }

    void HistoryTrackTab::updateStatusLabel() {
        updateStatus(m_model->hasPoints()
                         ? tr("Loaded %1 track samples.").arg(m_model->rowCount())
                         : tr("No track samples are available from the History API."));
    }

    int HistoryTrackTab::selectedSourceRow() const {
        if (!m_tableWidget) {
            return -1;
        }
        const auto selectedItems = m_tableWidget->selectedItems();
        if (selectedItems.isEmpty()) {
            return -1;
        }
        return selectedItems.first()->row();
    }

    QList<HistoryTrackPoint> HistoryTrackTab::allPoints() const {
        QList<HistoryTrackPoint> points;
        for (int row = 0; row < m_model->rowCount(); ++row) {
            points.append(m_model->pointAtRow(row));
        }
        return points;
    }

    bool HistoryTrackTab::editTrackPoint(const int row, const bool creating) {
        HistoryTrackPoint point = creating ? HistoryTrackPoint{} : m_model->pointAtRow(row);
        if (creating) {
            point.timestamp = QDateTime::currentDateTimeUtc();
            point.coordinate = QGeoCoordinate(0.0, 0.0, 0.0);
        }

        const QString title = creating ? tr("New Track Sample") : tr("Edit Track Sample");
        if (!editTrackPointDialog(this, title, &point, false)) {
            return false;
        }

        if (creating) {
            m_model->appendPoint(point);
        } else if (!m_model->updatePointAtRow(row, point)) {
            drawer::warning(this, tr("Tracks"), tr("Unable to update the selected sample."));
            return false;
        }

        updateStatusLabel();
        rebuildTable();

        const int selectedRow = creating ? m_model->rowCount() - 1 : row;
        if (selectedRow >= 0 && selectedRow < m_tableWidget->rowCount()) {
            m_tableWidget->selectRow(selectedRow);
        }
        return true;
    }

    QString HistoryTrackTab::currentDuration() const {
        return m_durationCombo->currentData().toString();
    }

    QString HistoryTrackTab::currentResolution() const {
        const QString duration = currentDuration();
        if (duration == QStringLiteral("PT1H")) {
            return QStringLiteral("PT1M");
        }
        if (duration == QStringLiteral("PT6H")) {
            return QStringLiteral("PT5M");
        }
        if (duration == QStringLiteral("P7D")) {
            return QStringLiteral("PT30M");
        }
        return QStringLiteral("PT10M");
    }
}
