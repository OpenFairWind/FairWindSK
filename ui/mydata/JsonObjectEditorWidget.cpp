//
// Created by Codex on 22/03/26.
//

#include "JsonObjectEditorWidget.hpp"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
    constexpr int KeyColumn = 0;
    constexpr int ValueColumn = 1;
    constexpr int TypeColumn = 2;

    QString defaultKeyForParent(const QTreeWidgetItem *parent) {
        if (!parent) {
            return QStringLiteral("property");
        }
        return parent->text(TypeColumn) == "array"
                ? QStringLiteral("[%1]").arg(parent->childCount())
                : QStringLiteral("property%1").arg(parent->childCount() + 1);
    }
}

namespace fairwindsk::ui::mydata {
    JsonObjectEditorWidget::JsonObjectEditorWidget(QWidget *parent)
        : QWidget(parent),
          m_tabWidget(new QTabWidget(this)),
          m_treeWidget(new QTreeWidget(this)),
          m_jsonEdit(new QPlainTextEdit(this)),
          m_addChildButton(new QToolButton(this)),
          m_addSiblingButton(new QToolButton(this)),
          m_removeButton(new QToolButton(this)) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        auto *toolbarLayout = new QHBoxLayout();
        rootLayout->addLayout(toolbarLayout);

        m_addChildButton->setText(tr("Add Child"));
        connect(m_addChildButton, &QToolButton::clicked, this, &JsonObjectEditorWidget::onAddChildClicked);
        toolbarLayout->addWidget(m_addChildButton);

        m_addSiblingButton->setText(tr("Add Sibling"));
        connect(m_addSiblingButton, &QToolButton::clicked, this, &JsonObjectEditorWidget::onAddSiblingClicked);
        toolbarLayout->addWidget(m_addSiblingButton);

        m_removeButton->setText(tr("Remove"));
        connect(m_removeButton, &QToolButton::clicked, this, &JsonObjectEditorWidget::onRemoveClicked);
        toolbarLayout->addWidget(m_removeButton);
        toolbarLayout->addStretch(1);

        rootLayout->addWidget(m_tabWidget, 1);

        m_treeWidget->setColumnCount(3);
        m_treeWidget->setHeaderLabels({tr("Key"), tr("Value"), tr("Type")});
        m_treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_treeWidget->header()->setStretchLastSection(false);
        m_treeWidget->setAlternatingRowColors(true);
        connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &JsonObjectEditorWidget::updateButtonState);
        m_tabWidget->addTab(m_treeWidget, tr("Tree"));

        m_jsonEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_jsonEdit->setStyleSheet("QPlainTextEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_tabWidget->addTab(m_jsonEdit, tr("JSON"));
        connect(m_tabWidget, &QTabWidget::currentChanged, this, &JsonObjectEditorWidget::onCurrentTabChanged);

        setJsonObject(QJsonObject{});
        setEditMode(false);
    }

    void JsonObjectEditorWidget::setLabels(const QString &treeTitle, const QString &jsonTitle) {
        m_tabWidget->setTabText(0, treeTitle);
        m_tabWidget->setTabText(1, jsonTitle);
    }

    void JsonObjectEditorWidget::setJsonObject(const QJsonObject &object) {
        populateTree(object);
        syncTextFromTree();
        updateButtonState();
    }

    QJsonObject JsonObjectEditorWidget::jsonObject(bool *ok, QString *message) const {
        if (m_tabWidget->currentIndex() == 1) {
            const auto text = m_jsonEdit->toPlainText().trimmed().toUtf8();
            if (text.isEmpty()) {
                if (ok) {
                    *ok = true;
                }
                return {};
            }

            const auto document = QJsonDocument::fromJson(text);
            const bool valid = document.isObject();
            if (ok) {
                *ok = valid;
            }
            if (!valid && message) {
                *message = tr("The JSON properties editor must contain a valid JSON object.");
            }
            return valid ? document.object() : QJsonObject{};
        }

        if (ok) {
            *ok = true;
        }
        return objectFromTree();
    }

    void JsonObjectEditorWidget::setEditMode(const bool editMode) {
        m_isEditing = editMode;

        const auto flags = editMode
                ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable
                : Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        std::function<void(QTreeWidgetItem *)> applyFlags = [&](QTreeWidgetItem *item) {
            if (!item) {
                return;
            }
            item->setFlags(flags);
            for (int i = 0; i < item->childCount(); ++i) {
                applyFlags(item->child(i));
            }
        };

        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            applyFlags(m_treeWidget->topLevelItem(i));
        }

        m_jsonEdit->setReadOnly(!editMode);
        updateButtonState();
    }

    void JsonObjectEditorWidget::onCurrentTabChanged(const int index) {
        if (index == 1) {
            syncTextFromTree();
        } else {
            syncTreeFromText();
        }
        updateButtonState();
    }

    void JsonObjectEditorWidget::onAddChildClicked() {
        auto *selected = m_treeWidget->currentItem();
        if (!selected) {
            auto *item = new QTreeWidgetItem(QStringList{QStringLiteral("property"), QString(), QStringLiteral("string")});
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            m_treeWidget->addTopLevelItem(item);
            m_treeWidget->setCurrentItem(item);
            syncTextFromTree();
            return;
        }

        if (selected->text(TypeColumn) != "object" && selected->text(TypeColumn) != "array") {
            selected = selected->parent();
        }

        auto *parent = selected;
        auto *child = new QTreeWidgetItem(QStringList{defaultKeyForParent(parent), QString(), QStringLiteral("string")});
        child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        if (parent) {
            parent->addChild(child);
            parent->setExpanded(true);
            renumberArrayChildren(parent);
        } else {
            m_treeWidget->addTopLevelItem(child);
        }
        m_treeWidget->setCurrentItem(child);
        syncTextFromTree();
    }

    void JsonObjectEditorWidget::onAddSiblingClicked() {
        auto *selected = m_treeWidget->currentItem();
        if (!selected) {
            onAddChildClicked();
            return;
        }

        auto *parent = selected->parent();
        auto *sibling = new QTreeWidgetItem(QStringList{defaultKeyForParent(parent), QString(), QStringLiteral("string")});
        sibling->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        if (parent) {
            parent->addChild(sibling);
            parent->setExpanded(true);
            renumberArrayChildren(parent);
        } else {
            m_treeWidget->addTopLevelItem(sibling);
        }
        m_treeWidget->setCurrentItem(sibling);
        syncTextFromTree();
    }

    void JsonObjectEditorWidget::onRemoveClicked() {
        auto *selected = m_treeWidget->currentItem();
        if (!selected) {
            return;
        }

        auto *parent = selected->parent();
        if (parent) {
            delete parent->takeChild(parent->indexOfChild(selected));
            renumberArrayChildren(parent);
        } else {
            delete m_treeWidget->takeTopLevelItem(m_treeWidget->indexOfTopLevelItem(selected));
        }
        syncTextFromTree();
        updateButtonState();
    }

    void JsonObjectEditorWidget::populateTree(const QJsonObject &object) {
        m_treeWidget->clear();
        for (auto it = object.begin(); it != object.end(); ++it) {
            addValueItem(nullptr, it.key(), it.value());
        }
        m_treeWidget->expandAll();
    }

    void JsonObjectEditorWidget::addValueItem(QTreeWidgetItem *parent, const QString &key, const QJsonValue &value) {
        auto *item = new QTreeWidgetItem();
        item->setText(KeyColumn, key);
        item->setText(TypeColumn, typeName(value));
        item->setFlags((m_isEditing ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        if (value.isObject()) {
            item->setText(ValueColumn, QStringLiteral("{...}"));
            const auto object = value.toObject();
            for (auto it = object.begin(); it != object.end(); ++it) {
                addValueItem(item, it.key(), it.value());
            }
        } else if (value.isArray()) {
            item->setText(ValueColumn, QStringLiteral("[%1]").arg(value.toArray().size()));
            const auto array = value.toArray();
            for (int index = 0; index < array.size(); ++index) {
                addValueItem(item, QStringLiteral("[%1]").arg(index), array.at(index));
            }
        } else {
            item->setText(ValueColumn, scalarToString(value));
        }

        if (parent) {
            parent->addChild(item);
        } else {
            m_treeWidget->addTopLevelItem(item);
        }
    }

    QJsonValue JsonObjectEditorWidget::valueFromItem(const QTreeWidgetItem *item) const {
        const QString type = item->text(TypeColumn);
        if (type == "object") {
            QJsonObject object;
            for (int i = 0; i < item->childCount(); ++i) {
                const auto *child = item->child(i);
                object[child->text(KeyColumn)] = valueFromItem(child);
            }
            return object;
        }
        if (type == "array") {
            QJsonArray array;
            for (int i = 0; i < item->childCount(); ++i) {
                array.append(valueFromItem(item->child(i)));
            }
            return array;
        }
        return scalarFromString(type, item->text(ValueColumn));
    }

    QJsonObject JsonObjectEditorWidget::objectFromTree() const {
        QJsonObject object;
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            const auto *item = m_treeWidget->topLevelItem(i);
            object[item->text(KeyColumn)] = valueFromItem(item);
        }
        return object;
    }

    void JsonObjectEditorWidget::syncTextFromTree() {
        m_jsonEdit->setPlainText(QString::fromUtf8(QJsonDocument(objectFromTree()).toJson(QJsonDocument::Indented)));
    }

    bool JsonObjectEditorWidget::syncTreeFromText(QString *message) {
        const auto text = m_jsonEdit->toPlainText().trimmed().toUtf8();
        if (text.isEmpty()) {
            populateTree(QJsonObject{});
            return true;
        }

        const auto document = QJsonDocument::fromJson(text);
        if (!document.isObject()) {
            if (message) {
                *message = tr("The JSON properties editor must contain a valid JSON object.");
            }
            return false;
        }

        populateTree(document.object());
        return true;
    }

    void JsonObjectEditorWidget::updateButtonState() {
        const bool treeMode = m_tabWidget->currentIndex() == 0;
        const bool hasSelection = m_treeWidget->currentItem() != nullptr;

        m_addChildButton->setVisible(m_isEditing && treeMode);
        m_addSiblingButton->setVisible(m_isEditing && treeMode);
        m_removeButton->setVisible(m_isEditing && treeMode);

        m_addChildButton->setEnabled(m_isEditing);
        m_addSiblingButton->setEnabled(m_isEditing && hasSelection);
        m_removeButton->setEnabled(m_isEditing && hasSelection);
    }

    QString JsonObjectEditorWidget::typeName(const QJsonValue &value) {
        if (value.isObject()) {
            return QStringLiteral("object");
        }
        if (value.isArray()) {
            return QStringLiteral("array");
        }
        if (value.isBool()) {
            return QStringLiteral("bool");
        }
        if (value.isDouble()) {
            return QStringLiteral("number");
        }
        if (value.isNull() || value.isUndefined()) {
            return QStringLiteral("null");
        }
        return QStringLiteral("string");
    }

    QJsonValue JsonObjectEditorWidget::defaultValueForType(const QString &typeName) {
        if (typeName == "object") {
            return QJsonObject{};
        }
        if (typeName == "array") {
            return QJsonArray{};
        }
        if (typeName == "bool") {
            return false;
        }
        if (typeName == "number") {
            return 0;
        }
        if (typeName == "null") {
            return QJsonValue::Null;
        }
        return QString{};
    }

    QString JsonObjectEditorWidget::scalarToString(const QJsonValue &value) {
        if (value.isBool()) {
            return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        }
        if (value.isDouble()) {
            return QString::number(value.toDouble());
        }
        if (value.isNull() || value.isUndefined()) {
            return QStringLiteral("null");
        }
        return value.toString();
    }

    QJsonValue JsonObjectEditorWidget::scalarFromString(const QString &typeName, const QString &text) {
        if (typeName == "bool") {
            return text.trimmed().compare("true", Qt::CaseInsensitive) == 0;
        }
        if (typeName == "number") {
            return text.toDouble();
        }
        if (typeName == "null") {
            return QJsonValue::Null;
        }
        return text;
    }

    void JsonObjectEditorWidget::renumberArrayChildren(QTreeWidgetItem *item) {
        if (!item || item->text(TypeColumn) != "array") {
            return;
        }

        for (int i = 0; i < item->childCount(); ++i) {
            item->child(i)->setText(KeyColumn, QStringLiteral("[%1]").arg(i));
        }
    }
}
