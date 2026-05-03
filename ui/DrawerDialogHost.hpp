//
// Created by Codex on 23/03/26.
//

#ifndef FAIRWINDSK_UI_DRAWERDIALOGHOST_HPP
#define FAIRWINDSK_UI_DRAWERDIALOGHOST_HPP

#include <QLineEdit>
#include <QMessageBox>
#include <QWidget>

namespace fairwindsk::ui::drawer {

    struct ButtonSpec {
        QString text;
        int result = 0;
        bool isDefault = false;
    };

    int execDrawer(QWidget *parent, const QString &title, QWidget *content, const QList<ButtonSpec> &buttons, int defaultResult = 0);
    QMessageBox::StandardButton message(QWidget *parent,
                                        QMessageBox::Icon icon,
                                        const QString &title,
                                        const QString &text,
                                        QMessageBox::StandardButtons buttons,
                                        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    inline QMessageBox::StandardButton question(QWidget *parent,
                                                const QString &title,
                                                const QString &text,
                                                const QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
                                                const QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return message(parent, QMessageBox::Question, title, text, buttons, defaultButton);
    }

    inline QMessageBox::StandardButton warning(QWidget *parent,
                                               const QString &title,
                                               const QString &text,
                                               const QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                               const QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return message(parent, QMessageBox::Warning, title, text, buttons, defaultButton);
    }

    inline QMessageBox::StandardButton information(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   const QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                   const QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return message(parent, QMessageBox::Information, title, text, buttons, defaultButton);
    }

    QString getText(QWidget *parent,
                    const QString &title,
                    const QString &label,
                    const QString &text = QString(),
                    bool *ok = nullptr,
                    QLineEdit::EchoMode echo = QLineEdit::Normal);
    QString getOpenFilePath(QWidget *parent,
                            const QString &title,
                            const QString &directory = QString(),
                            const QString &filter = QString());
    QString getIconPath(QWidget *parent,
                        const QString &title,
                        const QString &currentPath = QString());
    QString getSaveFilePath(QWidget *parent,
                            const QString &title,
                            const QString &path = QString(),
                            const QString &filter = QString());
    void exploreLogs(QWidget *parent,
                     const QString &title,
                     const QString &directory);
    bool editGeoCoordinate(QWidget *parent,
                           const QString &title,
                           double *latitude,
                           double *longitude,
                           double *altitude = nullptr,
                           QString *formatId = nullptr);
}

#endif // FAIRWINDSK_UI_DRAWERDIALOGHOST_HPP
