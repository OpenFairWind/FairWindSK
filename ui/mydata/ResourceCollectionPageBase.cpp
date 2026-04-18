//
// Created by Codex on 18/04/26.
//

#include "ResourceCollectionPageBase.hpp"

#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>

#include "FairWindSK.hpp"
#include "GeoJsonUtils.hpp"
#include "ResourceDialog.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::mydata {
    namespace {
        constexpr int kTouchButtonHeight = 58;
        constexpr int kTouchRowHeight = 64;

        QString touchToolButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors, const bool accent = false) {
            const QColor top = accent ? colors.accentTop : colors.buttonBackground.lighter(112);
            const QColor mid = accent ? colors.accentTop.darker(103) : colors.buttonBackground;
            const QColor bottom = accent ? colors.accentBottom : colors.buttonBackground.darker(118);
            const QColor border = accent ? colors.accentBottom : colors.border;
            const QColor text = accent ? colors.accentText : colors.buttonText;
            return QStringLiteral(
                "QToolButton {"
                " min-width: 76px;"
                " min-height: 58px;"
                " padding: 6px 10px;"
                " border-radius: 16px;"
                " border: 1px solid %1;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.5 %3, stop:1 %4);"
                " color: %5;"
                " font-size: 15px;"
                " font-weight: 700;"
                " }"
                "QToolButton:hover { border-color: %6; }"
                "QToolButton:pressed { background: %7; }"
                "QToolButton:disabled { color: %8; background: %9; border-color: %10; }")
                .arg(border.name(),
                     top.name(),
                     mid.name(),
                     bottom.name(),
                     text.name(),
                     colors.accentTop.name(),
                     colors.pressedBackground.name(),
                     colors.disabledText.name(),
                     colors.window.darker(104).name(),
                     colors.border.darker(130).name());
        }

        QString touchLineEditStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &baseColor) {
            return QStringLiteral(
                "QLineEdit {"
                " min-height: 54px;"
                " border: 1px solid %1;"
                " border-radius: 14px;"
                " padding: 6px 14px;"
                " background: %2;"
                " color: %3;"
                " font-size: 18px;"
                " }"
                "QLineEdit:focus { border-color: %4; }")
                .arg(colors.border.name(), baseColor.name(), colors.text.name(), colors.accentTop.name());
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
    }

    ResourceCollectionPageBase::ResourceCollectionPageBase(const ResourceKind kind, QWidget *parent)
        : QWidget(parent),
          m_kind(kind),
          m_model(new ResourceModel(kind, this)) {
        qInfo() << "ResourceCollectionPageBase ctor kind =" << resourceKindToTitle(kind);
        connect(m_model, &QAbstractItemModel::modelReset, this, &ResourceCollectionPageBase::rebuildTable);
    }

    void ResourceCollectionPageBase::bindPageUi(QLabel *titleLabel,
                                                QLineEdit *searchEdit,
                                                QTableWidget *tableWidget,
                                                QToolButton *openButton,
                                                QToolButton *editButton,
                                                QToolButton *addButton,
                                                QToolButton *deleteButton,
                                                QToolButton *importButton,
                                                QToolButton *exportButton,
                                                QToolButton *refreshButton) {
        m_titleLabel = titleLabel;
        m_searchEdit = searchEdit;
        m_tableWidget = tableWidget;
        m_openButton = openButton;
        m_editButton = editButton;
        m_addButton = addButton;
        m_deleteButton = deleteButton;
        m_importButton = importButton;
        m_exportButton = exportButton;
        m_refreshButton = refreshButton;

        m_titleLabel->setText(pageTitle());
        m_searchEdit->setPlaceholderText(searchPlaceholderText());

        m_openButton->setText(tr("Open"));
        m_editButton->setText(tr("Edit"));
        m_addButton->setText(tr("New"));
        m_deleteButton->setText(tr("Delete"));
        m_importButton->setText(tr("Import"));
        m_exportButton->setText(tr("Export"));
        m_refreshButton->setText(tr("Refresh"));

        qInfo() << "Binding MyData collection page UI for" << pageTitle();
        m_openButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg")));
        m_editButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/edit-google.svg")));
        m_addButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/widget-add-google.svg")));
        m_deleteButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/delete-google.svg")));
        m_importButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/route-import-iec.svg")));
        m_exportButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/file-export-google.svg")));
        m_refreshButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/refresh-google.svg")));

        const QList<QToolButton *> buttons = {
            m_openButton, m_editButton, m_addButton, m_deleteButton, m_importButton, m_exportButton, m_refreshButton
        };
        for (QToolButton *button : buttons) {
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setIconSize(QSize(26, 26));
            button->setMinimumHeight(kTouchButtonHeight);
        }

        configureTable();
        applyTouchFriendlyStyling();

        connect(m_searchEdit, &QLineEdit::textChanged, this, &ResourceCollectionPageBase::onSearchTextChanged);
        connect(m_openButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onOpenClicked);
        connect(m_editButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onEditClicked);
        connect(m_addButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onAddClicked);
        connect(m_deleteButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onDeleteClicked);
        connect(m_importButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onImportClicked);
        connect(m_exportButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onExportClicked);
        connect(m_refreshButton, &QToolButton::clicked, this, &ResourceCollectionPageBase::onRefreshClicked);
        connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &ResourceCollectionPageBase::onTableDoubleClicked);
        connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &ResourceCollectionPageBase::updateActionState);

        qInfo() << "MyData collection page UI bound for" << pageTitle() << "- rebuilding table";
        rebuildTable();
        qInfo() << "MyData collection page ready for" << pageTitle();
    }

    ResourceModel *ResourceCollectionPageBase::model() const {
        return m_model;
    }

    ResourceKind ResourceCollectionPageBase::kind() const {
        return m_kind;
    }

    QString ResourceCollectionPageBase::selectedResourceId() const {
        if (!m_tableWidget || m_tableWidget->selectedItems().isEmpty()) {
            return {};
        }
        return resourceIdForVisibleRow(m_tableWidget->selectedItems().first()->row());
    }

    QJsonObject ResourceCollectionPageBase::selectedResource() const {
        const QString resourceId = selectedResourceId();
        if (resourceId.isEmpty()) {
            return {};
        }

        for (int row = 0; row < m_model->rowCount(); ++row) {
            if (m_model->resourceIdAtRow(row) == resourceId) {
                return m_model->resourceAtRow(row);
            }
        }
        return {};
    }

    void ResourceCollectionPageBase::showPageError(const QString &message) const {
        fairwindsk::ui::drawer::warning(const_cast<ResourceCollectionPageBase *>(this), pageTitle(), message);
    }

    QString ResourceCollectionPageBase::importFileFilter() const {
        return tr("GeoJSON or JSON files (*.geojson *.json);;All files (*)");
    }

    QString ResourceCollectionPageBase::exportFileFilter() const {
        return tr("GeoJSON files (*.geojson);;JSON files (*.json)");
    }

    QString ResourceCollectionPageBase::exportDefaultFileName() const {
        return resourceKindToCollection(m_kind) + QStringLiteral(".geojson");
    }

    QString ResourceCollectionPageBase::importSuccessMessage(const int importedCount) const {
        return tr("Imported %1 %2.")
            .arg(importedCount)
            .arg(importedCount == 1 ? resourceKindToSingularTitle(m_kind).toLower() : resourceKindToTitle(m_kind).toLower());
    }

    bool ResourceCollectionPageBase::importResourcesFromPath(const QString &fileName,
                                                             QList<QPair<QString, QJsonObject>> *resources,
                                                             QString *message) const {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            if (message) {
                *message = tr("Unable to open %1.").arg(fileName);
            }
            return false;
        }
        return importResourcesFromGeoJson(m_kind, QJsonDocument::fromJson(file.readAll()), resources, message);
    }

    bool ResourceCollectionPageBase::exportResourcesToPath(const QString &fileName,
                                                           const QList<QPair<QString, QJsonObject>> &resources,
                                                           QString *message) const {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (message) {
                *message = tr("Unable to write %1.").arg(fileName);
            }
            return false;
        }

        if (file.write(exportResourcesAsGeoJson(m_kind, resources).toJson(QJsonDocument::Indented)) < 0) {
            if (message) {
                *message = tr("Unable to export data to %1.").arg(fileName);
            }
            return false;
        }
        return true;
    }

    void ResourceCollectionPageBase::triggerPrimaryAction(const QString &id, const QJsonObject &resource) {
        openEditor(id, resource, false);
    }

    void ResourceCollectionPageBase::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);

        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
            applyTouchFriendlyStyling();
        } else if (event->type() == QEvent::LanguageChange) {
            if (m_titleLabel) {
                m_titleLabel->setText(pageTitle());
            }
            if (m_searchEdit) {
                m_searchEdit->setPlaceholderText(searchPlaceholderText());
            }
            rebuildTable();
        }
    }

    void ResourceCollectionPageBase::rebuildTable() {
        if (!m_tableWidget) {
            return;
        }

        qInfo() << "Rebuilding MyData table for" << pageTitle()
                << "rows =" << m_model->rowCount();

        const QString selectedId = selectedResourceId();
        const QString filter = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
        QStringList headers;
        for (int column = 0; column < m_model->columnCount(); ++column) {
            headers.append(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString());
        }

        m_tableWidget->setSortingEnabled(false);
        m_tableWidget->clearContents();
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
        m_tableWidget->setRowCount(0);
        m_visibleResourceIds.clear();

        for (int row = 0; row < m_model->rowCount(); ++row) {
            QStringList values;
            QString haystack;
            for (int column = 0; column < m_model->columnCount(); ++column) {
                const QString value = m_model->data(m_model->index(row, column), Qt::DisplayRole).toString();
                values.append(value);
                haystack += value + QLatin1Char(' ');
            }

            if (!filter.isEmpty() && !haystack.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            const int visibleRow = m_tableWidget->rowCount();
            m_tableWidget->insertRow(visibleRow);
            m_tableWidget->setRowHeight(visibleRow, kTouchRowHeight);
            m_visibleResourceIds.append(m_model->resourceIdAtRow(row));

            for (int column = 0; column < values.size(); ++column) {
                auto *item = new QTableWidgetItem(values.at(column));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                const QVariant alignment = m_model->data(m_model->index(row, column), Qt::TextAlignmentRole);
                if (alignment.isValid()) {
                    item->setTextAlignment(static_cast<Qt::Alignment>(alignment.toInt()));
                }
                m_tableWidget->setItem(visibleRow, column, item);
            }
        }

        m_tableWidget->setSortingEnabled(true);
        m_tableWidget->sortByColumn(0, Qt::AscendingOrder);

        if (!selectedId.isEmpty()) {
            for (int row = 0; row < m_visibleResourceIds.size(); ++row) {
                if (m_visibleResourceIds.at(row) == selectedId) {
                    m_tableWidget->selectRow(row);
                    break;
                }
            }
        }

        updateActionState();
    }

    void ResourceCollectionPageBase::onSearchTextChanged(const QString &text) {
        Q_UNUSED(text)
        rebuildTable();
    }

    void ResourceCollectionPageBase::onOpenClicked() {
        const QString id = selectedResourceId();
        if (!id.isEmpty()) {
            triggerPrimaryAction(id, selectedResource());
        }
    }

    void ResourceCollectionPageBase::onEditClicked() {
        const QString id = selectedResourceId();
        if (!id.isEmpty()) {
            openEditor(id, selectedResource(), false);
        }
    }

    void ResourceCollectionPageBase::onAddClicked() {
        openEditor(QString(), QJsonObject{}, true);
    }

    void ResourceCollectionPageBase::onDeleteClicked() {
        const QString id = selectedResourceId();
        if (id.isEmpty()) {
            return;
        }

        if (fairwindsk::ui::drawer::question(
                this,
                pageTitle(),
                tr("Delete the selected %1?").arg(resourceKindToSingularTitle(m_kind).toLower()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        if (!m_model->deleteResource(id)) {
            showPageError(tr("Unable to delete the selected %1.").arg(resourceKindToSingularTitle(m_kind).toLower()));
            return;
        }

        rebuildTable();
    }

    void ResourceCollectionPageBase::onImportClicked() {
        const QString fileName = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Import %1").arg(resourceKindToTitle(m_kind)),
            QString(),
            importFileFilter());
        if (fileName.isEmpty()) {
            return;
        }

        QList<QPair<QString, QJsonObject>> resources;
        QString message;
        if (!importResourcesFromPath(fileName, &resources, &message)) {
            showPageError(message);
            return;
        }

        int importedCount = 0;
        QString firstImportedId;
        for (const auto &entry : resources) {
            QString currentId = entry.first;
            bool updated = false;

            if (!currentId.isEmpty()) {
                updated = m_model->updateResource(currentId, entry.second);
            } else {
                currentId = m_model->createResource(entry.second);
                updated = !currentId.isEmpty();
            }

            if (updated) {
                ++importedCount;
                if (firstImportedId.isEmpty()) {
                    firstImportedId = currentId;
                }
            }
        }

        if (importedCount == 0) {
            showPageError(tr("No %1 could be imported from the selected file.").arg(resourceKindToTitle(m_kind).toLower()));
            return;
        }

        rebuildTable();
        if (!firstImportedId.isEmpty()) {
            for (int row = 0; row < m_visibleResourceIds.size(); ++row) {
                if (m_visibleResourceIds.at(row) == firstImportedId) {
                    m_tableWidget->selectRow(row);
                    break;
                }
            }
        }

        fairwindsk::ui::drawer::information(this, pageTitle(), importSuccessMessage(importedCount));
    }

    void ResourceCollectionPageBase::onExportClicked() {
        const QList<QPair<QString, QJsonObject>> resources = collectExportResources();
        if (resources.isEmpty()) {
            showPageError(tr("There are no %1 to export.").arg(resourceKindToTitle(m_kind).toLower()));
            return;
        }

        const QString fileName = fairwindsk::ui::drawer::getSaveFilePath(
            this,
            tr("Export %1").arg(resourceKindToTitle(m_kind)),
            exportDefaultFileName(),
            exportFileFilter());
        if (fileName.isEmpty()) {
            return;
        }

        QString message;
        if (!exportResourcesToPath(fileName, resources, &message)) {
            showPageError(message.isEmpty() ? tr("Unable to export the selected resources.") : message);
        }
    }

    void ResourceCollectionPageBase::onRefreshClicked() {
        m_model->reload(true);
        rebuildTable();
    }

    void ResourceCollectionPageBase::onTableDoubleClicked(const int row, const int column) {
        Q_UNUSED(row)
        Q_UNUSED(column)
        onOpenClicked();
    }

    void ResourceCollectionPageBase::updateActionState() {
        const bool hasSelection = !selectedResourceId().isEmpty();
        m_openButton->setEnabled(hasSelection);
        m_editButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
        m_exportButton->setEnabled(m_model->rowCount() > 0);
    }

    void ResourceCollectionPageBase::applyTouchFriendlyStyling() {
        if (!m_tableWidget || !m_searchEdit || !m_titleLabel) {
            return;
        }

        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto colors = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QColor panelColor = colors.window.darker(104);
        const QColor baseColor = colors.window.lighter(106);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 20.0);
        m_searchEdit->setStyleSheet(touchLineEditStyle(colors, baseColor));
        m_tableWidget->setStyleSheet(touchTableStyle(colors, baseColor, panelColor));

        const QList<QToolButton *> buttons = {
            m_openButton, m_editButton, m_addButton, m_deleteButton, m_importButton, m_exportButton, m_refreshButton
        };
        for (QToolButton *button : buttons) {
            const bool accent = button == m_addButton || button == m_openButton;
            button->setStyleSheet(touchToolButtonStyle(colors, accent));
            fairwindsk::ui::applyTintedButtonIcon(button, accent ? colors.accentText : colors.buttonText, QSize(26, 26));
        }
    }

    void ResourceCollectionPageBase::configureTable() {
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setWordWrap(false);
        m_tableWidget->setSortingEnabled(true);
        m_tableWidget->verticalHeader()->setVisible(false);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(kTouchRowHeight);
        m_tableWidget->horizontalHeader()->setStretchLastSection(true);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int column = 1; column < m_model->columnCount(); ++column) {
            m_tableWidget->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
        }
    }

    void ResourceCollectionPageBase::openEditor(const QString &resourceId, const QJsonObject &resource, const bool creating) {
        ResourceDialog dialog(m_kind, this);
        if (!creating && !resourceId.isEmpty()) {
            dialog.setResource(resourceId, resource);
        }

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        const QJsonObject resourceObject = dialog.resourceObject();
        QString resultingId = resourceId;
        if (creating) {
            resultingId = m_model->createResource(resourceObject);
            if (resultingId.isEmpty()) {
                resultingId = dialog.resourceId();
            }
        } else if (!m_model->updateResource(resourceId, resourceObject)) {
            showPageError(tr("Unable to save the selected %1.").arg(resourceKindToSingularTitle(m_kind).toLower()));
            return;
        }

        rebuildTable();
        if (!resultingId.isEmpty()) {
            for (int row = 0; row < m_visibleResourceIds.size(); ++row) {
                if (m_visibleResourceIds.at(row) == resultingId) {
                    m_tableWidget->selectRow(row);
                    break;
                }
            }
        }
    }

    QList<QPair<QString, QJsonObject>> ResourceCollectionPageBase::collectExportResources() const {
        QList<QPair<QString, QJsonObject>> resources;
        const QString selectedId = selectedResourceId();
        if (!selectedId.isEmpty()) {
            resources.append({selectedId, selectedResource()});
            return resources;
        }

        for (int row = 0; row < m_model->rowCount(); ++row) {
            resources.append({m_model->resourceIdAtRow(row), m_model->resourceAtRow(row)});
        }
        return resources;
    }

    QString ResourceCollectionPageBase::resourceIdForVisibleRow(const int row) const {
        if (row < 0 || row >= m_visibleResourceIds.size()) {
            return {};
        }
        return m_visibleResourceIds.at(row);
    }
}
