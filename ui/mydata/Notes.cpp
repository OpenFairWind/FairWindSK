//
// Created by Codex on 16/04/26.
//

#include "Notes.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QUrl>

#include "signalk/Client.hpp"
#include "ui_Notes.h"

namespace fairwindsk::ui::mydata {

    Notes::Notes(QWidget *parent)
        : ResourceCollectionPageBase(ResourceKind::Note, parent),
          ui(new Ui::Notes) {
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

    Notes::~Notes() {
        delete ui;
    }

    QString Notes::pageTitle() const {
        return tr("Notes");
    }

    QString Notes::searchPlaceholderText() const {
        return tr("Search notes by title, description, href, or MIME type");
    }

    QString Notes::importFileFilter() const {
        return tr("Notes and attachments (*.geojson *.json *.txt *.md *.markdown *.html *.htm *.pdf *.jpg *.jpeg *.png);;All files (*)");
    }

    QString Notes::exportFileFilter() const {
        return tr("GeoJSON or JSON notes (*.geojson *.json);;Attachment copy (*)");
    }

    QString Notes::importSuccessMessage(const int importedCount) const {
        return tr("Imported %1 note attachment(s).").arg(importedCount);
    }

    void Notes::triggerPrimaryAction(const QString &id, const QJsonObject &resource) {
        const QString href = resource["href"].toString().trimmed();
        if (!href.isEmpty()) {
            openAttachmentPath(href);
            return;
        }
        ResourceCollectionPageBase::triggerPrimaryAction(id, resource);
    }

    bool Notes::importResourcesFromPath(const QString &fileName,
                                        QList<QPair<QString, QJsonObject>> *resources,
                                        QString *message) const {
        const QFileInfo info(fileName);
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("geojson") || suffix == QStringLiteral("json")) {
            return ResourceCollectionPageBase::importResourcesFromPath(fileName, resources, message);
        }

        const QMimeDatabase mimeDatabase;
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(info, QMimeDatabase::MatchContent);

        QJsonObject note;
        note["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        note["title"] = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
        note["description"] = tr("Imported attachment from %1").arg(info.fileName());
        note["href"] = info.absoluteFilePath();
        note["mimeType"] = mimeType.isValid() ? mimeType.name() : QStringLiteral("application/octet-stream");

        if (mimeType.name().startsWith(QStringLiteral("text/"))) {
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QString firstLine = QString::fromUtf8(file.readLine()).trimmed();
                if (!firstLine.isEmpty()) {
                    note["description"] = firstLine.left(120);
                }
            }
        }

        resources->clear();
        resources->append({QString(), note});
        if (message) {
            *message = tr("Imported note attachment from %1.").arg(info.fileName());
        }
        return true;
    }

    bool Notes::exportResourcesToPath(const QString &fileName,
                                      const QList<QPair<QString, QJsonObject>> &resources,
                                      QString *message) const {
        const QString suffix = QFileInfo(fileName).suffix().toLower();
        if ((suffix != QStringLiteral("json") && suffix != QStringLiteral("geojson")) && resources.size() == 1) {
            const QString href = resources.first().second["href"].toString().trimmed();
            const QFileInfo attachmentInfo(href);
            if (attachmentInfo.exists() && attachmentInfo.isFile()) {
                QFile source(href);
                if (!source.open(QIODevice::ReadOnly)) {
                    if (message) {
                        *message = tr("Unable to read the linked attachment.");
                    }
                    return false;
                }

                QFile target(fileName);
                if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    if (message) {
                        *message = tr("Unable to write %1.").arg(fileName);
                    }
                    return false;
                }

                if (target.write(source.readAll()) < 0) {
                    if (message) {
                        *message = tr("Unable to export the linked attachment.");
                    }
                    return false;
                }
                return true;
            }
        }

        return ResourceCollectionPageBase::exportResourcesToPath(fileName, resources, message);
    }

    void Notes::openAttachmentPath(const QString &path) const {
        if (path.trimmed().isEmpty()) {
            showPageError(tr("This note does not reference an attachment or external link yet."));
            return;
        }

        const QFileInfo localInfo(path);
        const QUrl url = localInfo.exists() ? QUrl::fromLocalFile(localInfo.absoluteFilePath()) : QUrl::fromUserInput(path);
        if (!url.isValid() || !QDesktopServices::openUrl(url)) {
            showPageError(tr("Unable to open the selected attachment or link."));
        }
    }
}
