//
// Created by Codex on 16/04/26.
//

#include "Notes.hpp"

#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

#include "signalk/Client.hpp"
#include "ui/DrawerDialogHost.hpp"

namespace fairwindsk::ui::mydata {

    Notes::Notes(QWidget *parent)
        : ResourceTab(ResourceKind::Note, parent) {
        refreshWorkflowTexts();

        auto *attachmentActions = new QWidget(this);
        auto *attachmentLayout = new QHBoxLayout(attachmentActions);
        attachmentLayout->setContentsMargins(0, 0, 0, 0);
        attachmentLayout->setSpacing(8);

        m_browseAttachmentButton = new QPushButton(tr("Browse attachment"), attachmentActions);
        m_openAttachmentButton = new QPushButton(tr("Open attachment"), attachmentActions);
        attachmentLayout->addWidget(m_browseAttachmentButton);
        attachmentLayout->addWidget(m_openAttachmentButton);
        attachmentLayout->addStretch(1);

        if (auto *layout = editorFormLayout()) {
            layout->insertRow(4, tr("Attachment"), attachmentActions);
        }

        connect(m_browseAttachmentButton, &QPushButton::clicked, this, [this]() {
            chooseAttachment();
        });
        connect(m_openAttachmentButton, &QPushButton::clicked, this, [this]() {
            openAttachmentPath(hrefEdit() ? hrefEdit()->text().trimmed() : QString());
        });
        connect(hrefEdit(), &QLineEdit::textChanged, this, [this](const QString &text) {
            if (m_openAttachmentButton) {
                m_openAttachmentButton->setEnabled(!text.trimmed().isEmpty());
            }
        });

        if (m_openAttachmentButton) {
            m_openAttachmentButton->setEnabled(false);
        }
    }

    QString Notes::searchPlaceholderText() const {
        return tr("Search notes by title, summary, or attached link");
    }

    QString Notes::namePlaceholderText() const {
        return tr("Note title");
    }

    QString Notes::descriptionPlaceholderText() const {
        return tr("Bridge note, crew reminder, maintenance detail, or passage memo");
    }

    QString Notes::importButtonText() const {
        return tr("Import notes");
    }

    QString Notes::exportButtonText() const {
        return tr("Export notes");
    }

    QString Notes::importFileFilter() const {
        return tr("Notes and attachments (*.geojson *.json *.txt *.md *.markdown *.html *.htm *.pdf *.jpg *.jpeg *.png);;All files (*)");
    }

    QString Notes::exportFileFilter() const {
        return tr("GeoJSON or JSON notes (*.geojson *.json);;Attachment copy (*)");
    }

    QString Notes::primaryRowActionToolTip() const {
        return tr("Open linked attachment or note details");
    }

    QIcon Notes::primaryRowActionIcon() const {
        return QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg"));
    }

    void Notes::triggerPrimaryAction(const QString &id, const QJsonObject &resource) {
        const QString href = resource["href"].toString().trimmed();
        if (!href.isEmpty()) {
            openAttachmentPath(href);
            return;
        }
        ResourceTab::triggerPrimaryAction(id, resource);
    }

    bool Notes::importResourcesFromPath(const QString &fileName,
                                        QList<QPair<QString, QJsonObject>> *resources,
                                        QString *message) const {
        const QFileInfo info(fileName);
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("geojson") || suffix == QStringLiteral("json")) {
            return ResourceTab::importResourcesFromPath(fileName, resources, message);
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

        return ResourceTab::exportResourcesToPath(fileName, resources, message);
    }

    void Notes::chooseAttachment() {
        const QString selectedPath = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Select attachment"),
            QString(),
            tr("All files (*)"));
        if (selectedPath.isEmpty()) {
            return;
        }

        const QFileInfo info(selectedPath);
        const QMimeDatabase mimeDatabase;
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(info, QMimeDatabase::MatchContent);

        if (hrefEdit()) {
            hrefEdit()->setText(info.absoluteFilePath());
        }
        if (mimeTypeEdit()) {
            mimeTypeEdit()->setText(mimeType.isValid() ? mimeType.name() : QStringLiteral("application/octet-stream"));
        }
        if (nameEdit() && nameEdit()->text().trimmed().isEmpty()) {
            nameEdit()->setText(info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName());
        }
        if (descriptionEdit() && descriptionEdit()->text().trimmed().isEmpty()) {
            descriptionEdit()->setText(tr("Attached file %1").arg(info.fileName()));
        }
    }

    void Notes::openAttachmentPath(const QString &path) const {
        if (path.trimmed().isEmpty()) {
            showWorkflowError(tr("This note does not reference an attachment or external link yet."));
            return;
        }

        const QFileInfo localInfo(path);
        const QUrl url = localInfo.exists() ? QUrl::fromLocalFile(localInfo.absoluteFilePath()) : QUrl::fromUserInput(path);
        if (!url.isValid() || !QDesktopServices::openUrl(url)) {
            showWorkflowError(tr("Unable to open the selected attachment or link."));
        }
    }
}
