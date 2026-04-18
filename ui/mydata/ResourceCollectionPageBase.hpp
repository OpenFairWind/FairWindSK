//
// Created by Codex on 18/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCECOLLECTIONPAGEBASE_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCECOLLECTIONPAGEBASE_HPP

#include <QJsonObject>
#include <QList>
#include <QWidget>

#include "ResourceModel.hpp"

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace fairwindsk::ui::mydata {

    class ResourceCollectionPageBase : public QWidget {
        Q_OBJECT

    public:
        explicit ResourceCollectionPageBase(ResourceKind kind, QWidget *parent = nullptr);
        ~ResourceCollectionPageBase() override = default;

    protected:
        void bindPageUi(QLabel *titleLabel,
                        QLineEdit *searchEdit,
                        QTableWidget *tableWidget,
                        QPushButton *openButton,
                        QPushButton *editButton,
                        QPushButton *addButton,
                        QPushButton *deleteButton,
                        QPushButton *importButton,
                        QPushButton *exportButton,
                        QPushButton *refreshButton);

        ResourceModel *model() const;
        ResourceKind kind() const;
        QString selectedResourceId() const;
        QJsonObject selectedResource() const;
        void showPageError(const QString &message) const;

        virtual QString pageTitle() const = 0;
        virtual QString searchPlaceholderText() const = 0;
        virtual QString importFileFilter() const;
        virtual QString exportFileFilter() const;
        virtual QString exportDefaultFileName() const;
        virtual QString importSuccessMessage(int importedCount) const;
        virtual bool importResourcesFromPath(const QString &fileName,
                                             QList<QPair<QString, QJsonObject>> *resources,
                                             QString *message) const;
        virtual bool exportResourcesToPath(const QString &fileName,
                                           const QList<QPair<QString, QJsonObject>> &resources,
                                           QString *message) const;
        virtual void triggerPrimaryAction(const QString &id, const QJsonObject &resource);

        void changeEvent(QEvent *event) override;

    private slots:
        void rebuildTable();
        void onSearchTextChanged(const QString &text);
        void onOpenClicked();
        void onEditClicked();
        void onAddClicked();
        void onDeleteClicked();
        void onImportClicked();
        void onExportClicked();
        void onRefreshClicked();
        void onTableDoubleClicked(int row, int column);
        void updateActionState();

    private:
        void applyTouchFriendlyStyling();
        void configureTable();
        void openEditor(const QString &resourceId, const QJsonObject &resource, bool creating);
        QList<QPair<QString, QJsonObject>> collectExportResources() const;
        QString resourceIdForVisibleRow(int row) const;

        ResourceKind m_kind;
        ResourceModel *m_model = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLineEdit *m_searchEdit = nullptr;
        QTableWidget *m_tableWidget = nullptr;
        QPushButton *m_openButton = nullptr;
        QPushButton *m_editButton = nullptr;
        QPushButton *m_addButton = nullptr;
        QPushButton *m_deleteButton = nullptr;
        QPushButton *m_importButton = nullptr;
        QPushButton *m_exportButton = nullptr;
        QPushButton *m_refreshButton = nullptr;
        QStringList m_visibleResourceIds;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCECOLLECTIONPAGEBASE_HPP
