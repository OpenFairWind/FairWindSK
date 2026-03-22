//
// Created by Codex on 22/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_JSONOBJECTEDITORWIDGET_HPP
#define FAIRWINDSK_UI_MYDATA_JSONOBJECTEDITORWIDGET_HPP

#include <QJsonObject>
#include <QWidget>

class QPlainTextEdit;
class QTabWidget;
class QToolButton;
class QLabel;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace fairwindsk::ui::mydata {

    class JsonObjectEditorWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit JsonObjectEditorWidget(QWidget *parent = nullptr);

        void setJsonObject(const QJsonObject &object);
        QJsonObject jsonObject(bool *ok = nullptr, QString *message = nullptr) const;
        void setEditMode(bool editMode);
        void setLabels(const QString &treeTitle, const QString &jsonTitle);

    private slots:
        void onCurrentTabChanged(int index);
        void onAddChildClicked();
        void onAddSiblingClicked();
        void onRemoveClicked();

    private:
        void populateTree(const QJsonObject &object);
        void addValueItem(QTreeWidgetItem *parent, const QString &key, const QJsonValue &value);
        QJsonValue valueFromItem(const QTreeWidgetItem *item) const;
        QJsonObject objectFromTree() const;
        void syncTextFromTree();
        bool syncTreeFromText(QString *message = nullptr);
        void updateButtonState();
        void updateTreePlaceholder();
        static QString typeName(const QJsonValue &value);
        static QJsonValue defaultValueForType(const QString &typeName);
        static QString scalarToString(const QJsonValue &value);
        static QJsonValue scalarFromString(const QString &typeName, const QString &text);
        static void renumberArrayChildren(QTreeWidgetItem *item);

        QTabWidget *m_tabWidget = nullptr;
        QStackedWidget *m_treeStack = nullptr;
        QTreeWidget *m_treeWidget = nullptr;
        QLabel *m_emptyTreeLabel = nullptr;
        QPlainTextEdit *m_jsonEdit = nullptr;
        QToolButton *m_addChildButton = nullptr;
        QToolButton *m_addSiblingButton = nullptr;
        QToolButton *m_removeButton = nullptr;
        bool m_isEditing = false;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_JSONOBJECTEDITORWIDGET_HPP
