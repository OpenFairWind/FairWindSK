//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_NOTES_HPP
#define FAIRWINDSK_UI_MYDATA_NOTES_HPP

#include <QJsonObject>

#include "ResourceTab.hpp"

class QPushButton;

namespace fairwindsk::ui::mydata {

    class Notes final : public ResourceTab {
        Q_OBJECT

    public:
        explicit Notes(QWidget *parent = nullptr);
        ~Notes() override = default;

    protected:
        QString searchPlaceholderText() const override;
        QString namePlaceholderText() const override;
        QString descriptionPlaceholderText() const override;
        QString importButtonText() const override;
        QString exportButtonText() const override;
        QString importFileFilter() const override;
        QString exportFileFilter() const override;
        QString primaryRowActionToolTip() const override;
        QIcon primaryRowActionIcon() const override;
        void triggerPrimaryAction(const QString &id, const QJsonObject &resource) override;
        bool importResourcesFromPath(const QString &fileName,
                                     QList<QPair<QString, QJsonObject>> *resources,
                                     QString *message) const override;
        bool exportResourcesToPath(const QString &fileName,
                                   const QList<QPair<QString, QJsonObject>> &resources,
                                   QString *message) const override;

    private:
        void chooseAttachment();
        void openAttachmentPath(const QString &path) const;

        QPushButton *m_browseAttachmentButton = nullptr;
        QPushButton *m_openAttachmentButton = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_NOTES_HPP
