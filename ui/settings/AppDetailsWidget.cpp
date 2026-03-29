//
// Created by Codex on 28/03/26.
//

#include "AppDetailsWidget.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QMouseEvent>
#include <QSet>
#include <QTouchEvent>

#include "FairWindSK.hpp"
#include "PageDetailsWidget.hpp"
#include "ui_AppDetailsWidget.h"

namespace fairwindsk::ui::settings {
    namespace {
        struct IconEntry {
            QString path;
            QString label;
        };

        int iconPickerIconSize() {
            const auto *configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
            const QString preset = configuration->getUiScaleMode() == QStringLiteral("auto")
                                       ? QStringLiteral("normal")
                                       : configuration->getUiScalePreset();
            if (preset == QStringLiteral("xlarge")) {
                return 96;
            }
            if (preset == QStringLiteral("large")) {
                return 80;
            }
            if (preset == QStringLiteral("small")) {
                return 56;
            }
            return 64;
        }

        void appendIconEntry(QList<IconEntry> &entries, QSet<QString> &seen, const QString &path, const QString &label = QString()) {
            if (path.trimmed().isEmpty() || seen.contains(path)) {
                return;
            }

            seen.insert(path);
            entries.append({path, label.isEmpty() ? QFileInfo(path).completeBaseName() : label});
        }

        QList<IconEntry> availableIconEntries() {
            QList<IconEntry> entries;
            QSet<QString> seen;

            const QStringList resourceIcons = {
                QStringLiteral(":/resources/svg/OpenBridge/home.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/applications.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/settings-iec.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/navigation-route.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/database.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/anchor-iec.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/alerts.svg"),
                QStringLiteral(":/resources/svg/OpenBridge/command-autopilot.svg"),
                QStringLiteral(":/resources/images/icons/apps_icon.png"),
                QStringLiteral(":/resources/images/icons/settings_icon.png"),
                QStringLiteral(":/resources/images/icons/signalkserver_icon.png"),
                QStringLiteral(":/resources/images/icons/youtube_icon.png"),
                QStringLiteral(":/resources/images/icons/web_icon.png"),
                QStringLiteral(":/resources/images/icons/webapp-256x256.png")
            };
            for (const QString &path : resourceIcons) {
                appendIconEntry(entries, seen, path);
            }

            const QStringList directories = {
                QDir::currentPath() + QStringLiteral("/icons"),
                QCoreApplication::applicationDirPath() + QStringLiteral("/icons"),
                QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../Resources/icons"))
            };

            for (const QString &directoryPath : directories) {
                const QDir dir(directoryPath);
                if (!dir.exists()) {
                    continue;
                }

                const QFileInfoList iconFiles = dir.entryInfoList(
                    QStringList() << QStringLiteral("*.png")
                                  << QStringLiteral("*.svg")
                                  << QStringLiteral("*.jpg")
                                  << QStringLiteral("*.jpeg"),
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDir::Name);
                for (const QFileInfo &fileInfo : iconFiles) {
                    appendIconEntry(entries,
                                    seen,
                                    PageDetailsWidget::normalizedIconStoragePath(QStringLiteral("file://") + fileInfo.absoluteFilePath()),
                                    fileInfo.completeBaseName());
                }
            }

            return entries;
        }
    }

    AppDetailsWidget::AppDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::AppDetailsWidget) {
        ui->setupUi(this);

        populateIconPicker();
        hideIconPicker();

        connect(ui->pushButton_Apps_AppIcon_Browse, &QPushButton::clicked, this, &AppDetailsWidget::showIconPicker);
        connect(ui->listWidget_Apps_Icons, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { applySelectedIcon(); });
    }

    AppDetailsWidget::~AppDetailsWidget() {
        delete ui;
        ui = nullptr;
    }

    void AppDetailsWidget::setAppIconPath(const QString &path) {
        m_currentIconPath = PageDetailsWidget::normalizedIconStoragePath(path.trimmed());
        ensureIconEntry(m_currentIconPath);

        if (ui->listWidget_Apps_Icons) {
            for (int i = 0; i < ui->listWidget_Apps_Icons->count(); ++i) {
                auto *item = ui->listWidget_Apps_Icons->item(i);
                if (item && item->data(Qt::UserRole).toString() == m_currentIconPath) {
                    ui->listWidget_Apps_Icons->setCurrentItem(item);
                    break;
                }
            }
        }

        updateIconPreview(m_currentIconPath);
    }

    QString AppDetailsWidget::appIconPath() const {
        return m_currentIconPath;
    }

    void AppDetailsWidget::showIconPicker() {
        ensureIconEntry(m_currentIconPath);
        ui->frame_AppsIconPicker->setVisible(true);
        ui->listWidget_Apps_Icons->setFocus();
        qApp->installEventFilter(this);
    }

    void AppDetailsWidget::hideIconPicker() {
        ui->frame_AppsIconPicker->setVisible(false);
        qApp->removeEventFilter(this);
    }

    bool AppDetailsWidget::eventFilter(QObject *watched, QEvent *event) {
        Q_UNUSED(watched);
        if (!ui->frame_AppsIconPicker->isVisible()) {
            return QWidget::eventFilter(watched, event);
        }

        QPoint globalPos;
        bool hasGlobalPos = false;
        if (event->type() == QEvent::MouseButtonPress) {
            if (const auto *mouseEvent = static_cast<QMouseEvent *>(event)) {
                globalPos = mouseEvent->globalPosition().toPoint();
                hasGlobalPos = true;
            }
        } else if (event->type() == QEvent::TouchBegin) {
            if (const auto *touchEvent = static_cast<QTouchEvent *>(event); !touchEvent->points().isEmpty()) {
                globalPos = touchEvent->points().constFirst().globalPosition().toPoint();
                hasGlobalPos = true;
            }
        }

        if (!hasGlobalPos) {
            return QWidget::eventFilter(watched, event);
        }

        const QRect pickerRect(ui->frame_AppsIconPicker->mapToGlobal(QPoint(0, 0)), ui->frame_AppsIconPicker->size());
        const QRect browseRect(ui->pushButton_Apps_AppIcon_Browse->mapToGlobal(QPoint(0, 0)), ui->pushButton_Apps_AppIcon_Browse->size());
        if (!pickerRect.contains(globalPos) && !browseRect.contains(globalPos)) {
            hideIconPicker();
        }

        return QWidget::eventFilter(watched, event);
    }

    void AppDetailsWidget::applySelectedIcon() {
        auto *item = ui->listWidget_Apps_Icons ? ui->listWidget_Apps_Icons->currentItem() : nullptr;
        if (!item) {
            hideIconPicker();
            return;
        }

        const QString iconPath = PageDetailsWidget::normalizedIconStoragePath(item->data(Qt::UserRole).toString());
        setAppIconPath(iconPath);
        hideIconPicker();
        emit iconPathSelected(iconPath);
    }

    void AppDetailsWidget::populateIconPicker() {
        if (!ui->listWidget_Apps_Icons) {
            return;
        }

        const int iconSize = iconPickerIconSize();
        ui->listWidget_Apps_Icons->setIconSize(QSize(iconSize, iconSize));
        ui->listWidget_Apps_Icons->setGridSize(QSize(iconSize + 20, iconSize + 26));

        const QList<IconEntry> entries = availableIconEntries();
        for (const IconEntry &entry : entries) {
            auto *item = new QListWidgetItem(QIcon(PageDetailsWidget::iconPixmapForPath(entry.path, iconSize)), entry.label);
            item->setData(Qt::UserRole, entry.path);
            item->setToolTip(entry.path);
            ui->listWidget_Apps_Icons->addItem(item);
        }
    }

    void AppDetailsWidget::ensureIconEntry(const QString &path) {
        if (path.isEmpty() || !ui->listWidget_Apps_Icons) {
            return;
        }

        for (int i = 0; i < ui->listWidget_Apps_Icons->count(); ++i) {
            auto *item = ui->listWidget_Apps_Icons->item(i);
            if (item && item->data(Qt::UserRole).toString() == path) {
                return;
            }
        }

        const int iconSize = iconPickerIconSize();
        auto *item = new QListWidgetItem(QIcon(PageDetailsWidget::iconPixmapForPath(path, iconSize)), QFileInfo(path).completeBaseName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        ui->listWidget_Apps_Icons->insertItem(0, item);
    }

    void AppDetailsWidget::updateIconPreview(const QString &path) {
        ui->label_Apps_Icon->setPixmap(PageDetailsWidget::iconPixmapForPath(path, 128));
    }
}
