//
// Created by Codex on 21/03/26.
//

#include "HistoryTrackTab.hpp"

#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QComboBox>

#include "HistoryTrackModel.hpp"

namespace fairwindsk::ui::mydata {
    HistoryTrackTab::HistoryTrackTab(QWidget *parent)
        : QWidget(parent),
          m_model(new HistoryTrackModel(this)),
          m_statusLabel(new QLabel(this)),
          m_durationCombo(new QComboBox(this)),
          m_tableView(new QTableView(this)),
          m_refreshButton(new QToolButton(this)),
          m_importButton(new QToolButton(this)),
          m_exportButton(new QToolButton(this)),
          m_refreshTimer(new QTimer(this)) {
        auto *mainLayout = new QVBoxLayout(this);
        auto *toolbarLayout = new QHBoxLayout();
        mainLayout->addLayout(toolbarLayout);

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

        toolbarLayout->addStretch(1);
        toolbarLayout->addWidget(m_statusLabel, 1);

        m_tableView->setModel(m_model);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        mainLayout->addWidget(m_tableView);

        connect(m_refreshTimer, &QTimer::timeout, this, &HistoryTrackTab::onRefreshClicked);
        m_refreshTimer->start(5000);

        onRefreshClicked();
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

    void HistoryTrackTab::onRefreshClicked() {
        QString message;
        m_model->reload(currentDuration(), currentResolution(), &message);
        updateStatus(message);
    }

    void HistoryTrackTab::onDurationChanged() {
        onRefreshClicked();
    }

    void HistoryTrackTab::onImportClicked() {
        const QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Import Tracks"),
                QString(),
                tr("JSON files (*.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to open %1.").arg(fileName));
            return;
        }

        QString message;
        if (!m_model->importDocument(QJsonDocument::fromJson(file.readAll()), &message)) {
            QMessageBox::warning(this, tr("Tracks"), message);
            return;
        }

        updateStatus(message);
    }

    void HistoryTrackTab::onExportClicked() {
        const QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Export Tracks"),
                QString("tracks-history.json"),
                tr("JSON files (*.json);;All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("Tracks"), tr("Unable to write %1.").arg(fileName));
            return;
        }

        file.write(m_model->exportDocument().toJson(QJsonDocument::Indented));
    }
}
