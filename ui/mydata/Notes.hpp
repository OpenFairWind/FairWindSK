//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_NOTES_HPP
#define FAIRWINDSK_UI_MYDATA_NOTES_HPP

#include <QJsonObject>

#include "ResourceCollectionPageBase.hpp"

namespace Ui { class Notes; }

namespace fairwindsk::ui::mydata {

    class Notes final : public ResourceCollectionPageBase {
        Q_OBJECT

    public:
        explicit Notes(QWidget *parent = nullptr);
        ~Notes() override;

    protected:
        QString pageTitle() const override;
        QString searchPlaceholderText() const override;
        QString importFileFilter() const override;
        QString exportFileFilter() const override;
        QString importSuccessMessage(int importedCount) const override;
        void triggerPrimaryAction(const QString &id, const QJsonObject &resource) override;
        bool importResourcesFromPath(const QString &fileName,
                                     QList<QPair<QString, QJsonObject>> *resources,
                                     QString *message) const override;
        bool exportResourcesToPath(const QString &fileName,
                                   const QList<QPair<QString, QJsonObject>> &resources,
                                   QString *message) const override;

    private:
        void openAttachmentPath(const QString &path) const;

        ::Ui::Notes *ui = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_NOTES_HPP
