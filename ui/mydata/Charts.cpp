//
// Created by Codex on 16/04/26.
//

#include "Charts.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QUrl>

#include "signalk/Client.hpp"
#include "ui/DrawerDialogHost.hpp"

namespace {
    QString chartFormatFromPath(const QFileInfo &info) {
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("mbtiles")) {
            return QStringLiteral("mbtiles");
        }
        if (suffix == QStringLiteral("kap") || suffix == QStringLiteral("bsb")) {
            return QStringLiteral("kap");
        }
        if (suffix == QStringLiteral("gemf")) {
            return QStringLiteral("gemf");
        }
        if (suffix == QStringLiteral("xml")) {
            return QStringLiteral("tilemapresource");
        }
        if (suffix == QStringLiteral("zip")) {
            return QStringLiteral("package");
        }
        return suffix.isEmpty() ? QStringLiteral("file") : suffix;
    }

    QJsonObject chartResourceFromPath(const QString &fileName) {
        const QFileInfo info(fileName);
        QJsonObject resource;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        resource["name"] = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
        resource["description"] = QObject::tr("Imported chart source %1").arg(info.fileName());
        resource["identifier"] = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
        resource["chartFormat"] = chartFormatFromPath(info);
        resource["chartUrl"] = info.absoluteFilePath();
        if (info.fileName().compare(QStringLiteral("tilemapresource.xml"), Qt::CaseInsensitive) == 0) {
            resource["tilemapUrl"] = info.absoluteFilePath();
            resource["chartUrl"] = info.absolutePath();
        }
        return resource;
    }

    QJsonDocument chartManifestDocument(const QList<QPair<QString, QJsonObject>> &resources) {
        QJsonArray charts;
        for (const auto &entry : resources) {
            charts.append(entry.second);
        }

        QJsonObject root;
        root["type"] = QStringLiteral("FairWindSKChartPackage");
        root["charts"] = charts;
        return QJsonDocument(root);
    }
}

namespace fairwindsk::ui::mydata {

    Charts::Charts(QWidget *parent)
        : ResourceTab(ResourceKind::Chart, parent) {
        refreshWorkflowTexts();

        m_browseChartButton = chooseChartSourceButton();
        m_openChartButton = openChartSourceButton();
        m_browseTileMapButton = chooseTileMapSourceButton();

        connect(m_browseChartButton, &QPushButton::clicked, this, [this]() {
            chooseChartSource();
        });
        connect(m_openChartButton, &QPushButton::clicked, this, [this]() {
            openChartSourcePath(chartUrlEdit() ? chartUrlEdit()->text().trimmed() : QString());
        });
        connect(m_browseTileMapButton, &QPushButton::clicked, this, [this]() {
            chooseTileMapSource();
        });
        connect(chartUrlEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
            if (m_openChartButton) {
                m_openChartButton->setEnabled(!text.trimmed().isEmpty());
            }
        });

        if (m_openChartButton) {
            m_openChartButton->setEnabled(false);
        }
    }

    QString Charts::searchPlaceholderText() const {
        return tr("Search charts by name, identifier, region, or format");
    }

    QString Charts::namePlaceholderText() const {
        return tr("Chart display name");
    }

    QString Charts::descriptionPlaceholderText() const {
        return tr("Coverage, source, scale usage, or onboard deployment note");
    }

    QString Charts::importButtonText() const {
        return tr("Import package");
    }

    QString Charts::exportButtonText() const {
        return tr("Export package");
    }

    QString Charts::importFileFilter() const {
        return tr("Chart packages (*.json *.geojson *.mbtiles *.kap *.bsb *.gemf *.xml *.zip);;All files (*)");
    }

    QString Charts::exportFileFilter() const {
        return tr("Chart package manifest (*.json);;GeoJSON charts (*.geojson)");
    }

    QString Charts::exportDefaultFileName() const {
        return QStringLiteral("charts-package.json");
    }

    QString Charts::primaryRowActionToolTip() const {
        return tr("Open chart source or inspect metadata");
    }

    QIcon Charts::primaryRowActionIcon() const {
        return QIcon(QStringLiteral(":/resources/svg/OpenBridge/chart-aton-iec.svg"));
    }

    void Charts::triggerPrimaryAction(const QString &id, const QJsonObject &resource) {
        const QString sourcePath = resource["chartUrl"].toString().trimmed();
        if (!sourcePath.isEmpty()) {
            openChartSourcePath(sourcePath);
            return;
        }
        ResourceTab::triggerPrimaryAction(id, resource);
    }

    bool Charts::importResourcesFromPath(const QString &fileName,
                                         QList<QPair<QString, QJsonObject>> *resources,
                                         QString *message) const {
        const QFileInfo info(fileName);
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("geojson")) {
            return ResourceTab::importResourcesFromPath(fileName, resources, message);
        }

        if (suffix == QStringLiteral("json")) {
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) {
                if (message) {
                    *message = tr("Unable to open %1.").arg(fileName);
                }
                return false;
            }

            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            if (document.isObject()) {
                const QJsonObject object = document.object();
                if (object.value(QStringLiteral("type")).toString() == QStringLiteral("FairWindSKChartPackage")
                    && object.value(QStringLiteral("charts")).isArray()) {
                    resources->clear();
                    const QJsonArray charts = object.value(QStringLiteral("charts")).toArray();
                    for (const auto &chartValue : charts) {
                        if (chartValue.isObject()) {
                            resources->append({chartValue.toObject().value(QStringLiteral("identifier")).toString(), chartValue.toObject()});
                        }
                    }
                    if (resources->isEmpty()) {
                        if (message) {
                            *message = tr("The selected chart package does not contain chart definitions.");
                        }
                        return false;
                    }
                    if (message) {
                        *message = tr("Imported %1 chart definition(s) from the package manifest.").arg(resources->size());
                    }
                    return true;
                }
            }

            return ResourceTab::importResourcesFromPath(fileName, resources, message);
        }

        resources->clear();
        resources->append({QString(), chartResourceFromPath(fileName)});
        if (message) {
            *message = tr("Imported chart source %1.").arg(info.fileName());
        }
        return true;
    }

    bool Charts::exportResourcesToPath(const QString &fileName,
                                       const QList<QPair<QString, QJsonObject>> &resources,
                                       QString *message) const {
        const QString suffix = QFileInfo(fileName).suffix().toLower();
        if (suffix == QStringLiteral("json")) {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                if (message) {
                    *message = tr("Unable to write %1.").arg(fileName);
                }
                return false;
            }
            if (file.write(chartManifestDocument(resources).toJson(QJsonDocument::Indented)) < 0) {
                if (message) {
                    *message = tr("Unable to export the chart package manifest.");
                }
                return false;
            }
            return true;
        }

        return ResourceTab::exportResourcesToPath(fileName, resources, message);
    }

    void Charts::chooseChartSource() {
        const QString selectedPath = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Select chart source"),
            QString(),
            tr("Chart sources (*.mbtiles *.kap *.bsb *.gemf *.xml *.zip);;All files (*)"));
        if (selectedPath.isEmpty()) {
            return;
        }

        const QFileInfo info(selectedPath);
        if (chartUrlEdit()) {
            chartUrlEdit()->setText(info.absoluteFilePath());
        }
        if (chartFormatEdit()) {
            chartFormatEdit()->setText(chartFormatFromPath(info));
        }
        if (identifierEdit() && identifierEdit()->text().trimmed().isEmpty()) {
            identifierEdit()->setText(info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName());
        }
        if (nameEdit() && nameEdit()->text().trimmed().isEmpty()) {
            nameEdit()->setText(info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName());
        }
        if (descriptionEdit() && descriptionEdit()->text().trimmed().isEmpty()) {
            descriptionEdit()->setText(tr("Local chart source %1").arg(info.fileName()));
        }
        if (info.fileName().compare(QStringLiteral("tilemapresource.xml"), Qt::CaseInsensitive) == 0 && tilemapUrlEdit()) {
            tilemapUrlEdit()->setText(info.absoluteFilePath());
        }
    }

    void Charts::chooseTileMapSource() {
        const QString selectedPath = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Select tile map source"),
            QString(),
            tr("Tile map sources (*.xml *.json);;All files (*)"));
        if (selectedPath.isEmpty()) {
            return;
        }

        if (tilemapUrlEdit()) {
            tilemapUrlEdit()->setText(QFileInfo(selectedPath).absoluteFilePath());
        }
    }

    void Charts::openChartSourcePath(const QString &path) const {
        if (path.trimmed().isEmpty()) {
            showWorkflowError(tr("This chart does not have a source path yet."));
            return;
        }

        const QFileInfo localInfo(path);
        const QUrl url = localInfo.exists() ? QUrl::fromLocalFile(localInfo.absoluteFilePath()) : QUrl::fromUserInput(path);
        if (!url.isValid() || !QDesktopServices::openUrl(url)) {
            showWorkflowError(tr("Unable to open the selected chart source."));
        }
    }
}
