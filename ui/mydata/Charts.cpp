//
// Created by Codex on 16/04/26.
//

#include "Charts.hpp"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "signalk/Client.hpp"
#include "ui_Charts.h"

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
        : ResourceCollectionPageBase(ResourceKind::Chart, parent),
          ui(new Ui::Charts) {
        ui->setupUi(this);
        bindPageUi(ui->labelTitle,
                   ui->lineEditSearch,
                   ui->tableWidget,
                   ui->toolButtonOpen,
                   ui->toolButtonEdit,
                   ui->toolButtonAdd,
                   ui->toolButtonDelete,
                   ui->toolButtonImport,
                   ui->toolButtonExport,
                   ui->toolButtonRefresh);
    }

    Charts::~Charts() {
        delete ui;
    }

    QString Charts::pageTitle() const {
        return tr("Charts");
    }

    QString Charts::searchPlaceholderText() const {
        return tr("Search charts by name, identifier, region, or format");
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

    void Charts::triggerPrimaryAction(const QString &id, const QJsonObject &resource) {
        const QString sourcePath = resource["chartUrl"].toString().trimmed();
        if (!sourcePath.isEmpty()) {
            openChartSourcePath(sourcePath);
            return;
        }
        ResourceCollectionPageBase::triggerPrimaryAction(id, resource);
    }

    bool Charts::importResourcesFromPath(const QString &fileName,
                                         QList<QPair<QString, QJsonObject>> *resources,
                                         QString *message) const {
        const QFileInfo info(fileName);
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("geojson")) {
            return ResourceCollectionPageBase::importResourcesFromPath(fileName, resources, message);
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

            return ResourceCollectionPageBase::importResourcesFromPath(fileName, resources, message);
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

        return ResourceCollectionPageBase::exportResourcesToPath(fileName, resources, message);
    }

    void Charts::openChartSourcePath(const QString &path) const {
        if (path.trimmed().isEmpty()) {
            showPageError(tr("This chart does not have a source path yet."));
            return;
        }

        openPathInSingleWindow(path,
                               tr("This chart does not have a source path yet."),
                               tr("Unable to open the selected chart source."));
    }
}
