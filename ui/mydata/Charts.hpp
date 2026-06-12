//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_CHARTS_HPP
#define FAIRWINDSK_UI_MYDATA_CHARTS_HPP

#include <QJsonObject>

#include "ResourceCollectionPageBase.hpp"

namespace Ui { class Charts; }

namespace fairwindsk::ui::mydata {

    class Charts final : public ResourceCollectionPageBase {
        Q_OBJECT

    public:
        explicit Charts(QWidget *parent = nullptr);
        ~Charts() override;

    protected:
        QString pageTitle() const override;
        QString searchPlaceholderText() const override;
        QString importFileFilter() const override;
        QString exportFileFilter() const override;
        QString exportDefaultFileName() const override;
        void triggerPrimaryAction(const QString &id, const QJsonObject &resource) override;
        bool importResourcesFromPath(const QString &fileName,
                                     QList<QPair<QString, QJsonObject>> *resources,
                                     QString *message) const override;
        bool exportResourcesToPath(const QString &fileName,
                                   const QList<QPair<QString, QJsonObject>> &resources,
                                   QString *message) const override;

    private:
        void openChartSourcePath(const QString &path) const;

        ::Ui::Charts *ui = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_CHARTS_HPP
