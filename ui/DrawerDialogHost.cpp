//
// Created by Codex on 23/03/26.
//

#include "DrawerDialogHost.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "MainWindow.hpp"

namespace fairwindsk::ui::drawer {
    namespace {
        MainWindow *resolveMainWindow(QWidget *parent) {
            if (parent) {
                if (auto *mainWindow = qobject_cast<MainWindow *>(parent->window())) {
                    return mainWindow;
                }
            }
            return MainWindow::instance(parent);
        }

        QString buttonText(const QMessageBox::StandardButton button) {
            QMessageBox box;
            box.setStandardButtons(button);
            if (auto *pushButton = box.button(button)) {
                return pushButton->text();
            }

            switch (button) {
                case QMessageBox::Ok: return QObject::tr("OK");
                case QMessageBox::Cancel: return QObject::tr("Cancel");
                case QMessageBox::Yes: return QObject::tr("Yes");
                case QMessageBox::No: return QObject::tr("No");
                case QMessageBox::Save: return QObject::tr("Save");
                case QMessageBox::Discard: return QObject::tr("Discard");
                default: return QObject::tr("Close");
            }
        }

        QString iconGlyph(const QMessageBox::Icon icon) {
            switch (icon) {
                case QMessageBox::Warning: return QStringLiteral("!");
                case QMessageBox::Information: return QStringLiteral("i");
                case QMessageBox::Question: return QStringLiteral("?");
                case QMessageBox::Critical: return QStringLiteral("!");
                case QMessageBox::NoIcon: break;
            }
            return QString();
        }
    }

    int execDrawer(QWidget *parent, const QString &title, QWidget *content, const QList<ButtonSpec> &buttons, const int defaultResult) {
        auto *mainWindow = resolveMainWindow(parent);
        if (!mainWindow || !content) {
            return defaultResult;
        }
        QList<DrawerButtonSpec> specs;
        specs.reserve(buttons.size());
        for (const auto &button : buttons) {
            specs.append({button.text, button.result, button.isDefault});
        }
        return mainWindow->execDrawer(title, content, specs, defaultResult);
    }

    QMessageBox::StandardButton message(QWidget *parent,
                                        const QMessageBox::Icon icon,
                                        const QString &title,
                                        const QString &text,
                                        const QMessageBox::StandardButtons buttons,
                                        const QMessageBox::StandardButton defaultButton) {
        auto *mainWindow = resolveMainWindow(parent);
        if (!mainWindow) {
            QMessageBox box(icon, title, text, buttons, parent);
            box.setDefaultButton(defaultButton);
            return static_cast<QMessageBox::StandardButton>(box.exec());
        }

        auto *content = new QWidget();
        auto *layout = new QHBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        const QString glyph = iconGlyph(icon);
        if (!glyph.isEmpty()) {
            auto *iconLabel = new QLabel(glyph, content);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setFixedSize(30, 30);
            iconLabel->setStyleSheet(
                "QLabel { background: #f3f4f6; color: #111827; border: 1px solid #d1d5db; border-radius: 15px; font-size: 18px; font-weight: 700; }");
            layout->addWidget(iconLabel, 0, Qt::AlignTop);
        }

        auto *textLabel = new QLabel(text, content);
        textLabel->setWordWrap(true);
        textLabel->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(textLabel, 1);

        QList<ButtonSpec> specs;
        const QList<QMessageBox::StandardButton> orderedButtons = {
            QMessageBox::Yes, QMessageBox::No, QMessageBox::Ok, QMessageBox::Cancel,
            QMessageBox::Save, QMessageBox::Discard
        };
        for (const auto standardButton : orderedButtons) {
            if (buttons.testFlag(standardButton)) {
                specs.append({buttonText(standardButton), int(standardButton), standardButton == defaultButton});
            }
        }

        QList<DrawerButtonSpec> drawerSpecs;
        drawerSpecs.reserve(specs.size());
        for (const auto &spec : specs) {
            drawerSpecs.append({spec.text, spec.result, spec.isDefault});
        }
        const int result = mainWindow->execDrawer(title, content, drawerSpecs, int(defaultButton));
        return QMessageBox::StandardButton(result);
    }

    QString getText(QWidget *parent,
                    const QString &title,
                    const QString &label,
                    const QString &text,
                    bool *ok,
                    const QLineEdit::EchoMode echo) {
        auto *mainWindow = resolveMainWindow(parent);
        if (!mainWindow) {
            return QInputDialog::getText(parent, title, label, echo, text, ok);
        }

        auto *content = new QWidget();
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *labelWidget = new QLabel(label, content);
        labelWidget->setWordWrap(true);
        labelWidget->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(labelWidget);

        auto *lineEdit = new QLineEdit(content);
        lineEdit->setEchoMode(echo);
        lineEdit->setText(text);
        lineEdit->setStyleSheet(
            "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        layout->addWidget(lineEdit);

        const QList<DrawerButtonSpec> drawerSpecs = {
            {QObject::tr("OK"), int(QMessageBox::Ok), true},
            {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
        };
        const int result = mainWindow->execDrawer(title, content, drawerSpecs, int(QMessageBox::Cancel));
        if (ok) {
            *ok = result == QMessageBox::Ok;
        }
        return result == QMessageBox::Ok ? lineEdit->text() : QString();
    }
}
