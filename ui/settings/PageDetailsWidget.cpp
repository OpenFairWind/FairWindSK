//
// Created by Codex on 28/03/26.
//

#include "PageDetailsWidget.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QMouseEvent>
#include <QSet>
#include <QTouchEvent>
#include <algorithm>

#include "FairWindSK.hpp"
#include "ui_PageDetailsWidget.h"

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

    PageDetailsWidget::PageDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PageDetailsWidget) {
        ui->setupUi(this);

        populateIconPicker();
        hideIconPicker();

        connect(ui->pushButton_Page_Icon_Browse, &QPushButton::clicked, this, &PageDetailsWidget::showIconPicker);
        connect(ui->listWidget_Page_Icons, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { applySelectedIcon(); });
    }

    PageDetailsWidget::~PageDetailsWidget() {
        delete ui;
        ui = nullptr;
    }

    void PageDetailsWidget::setPageName(const QString &name) const {
        ui->lineEdit_Page_Name->setText(name);
    }

    QString PageDetailsWidget::pageName() const {
        return ui->lineEdit_Page_Name->text().trimmed();
    }

    void PageDetailsWidget::setPageIconPath(const QString &path) {
        m_currentIconPath = normalizedIconStoragePath(path.trimmed());
        ensureIconEntry(m_currentIconPath);

        if (ui->listWidget_Page_Icons) {
            for (int i = 0; i < ui->listWidget_Page_Icons->count(); ++i) {
                auto *item = ui->listWidget_Page_Icons->item(i);
                if (item && item->data(Qt::UserRole).toString() == m_currentIconPath) {
                    ui->listWidget_Page_Icons->setCurrentItem(item);
                    break;
                }
            }
        }

        updateIconPreview(m_currentIconPath);
    }

    QString PageDetailsWidget::pageIconPath() const {
        return m_currentIconPath;
    }

    void PageDetailsWidget::showIconPicker() {
        ensureIconEntry(m_currentIconPath);
        ui->frame_PageIconPicker->setVisible(true);
        ui->listWidget_Page_Icons->setFocus();
        qApp->installEventFilter(this);
    }

    void PageDetailsWidget::hideIconPicker() {
        ui->frame_PageIconPicker->setVisible(false);
        qApp->removeEventFilter(this);
    }

    bool PageDetailsWidget::eventFilter(QObject *watched, QEvent *event) {
        Q_UNUSED(watched);
        if (!ui->frame_PageIconPicker->isVisible()) {
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

        const QRect pickerRect(ui->frame_PageIconPicker->mapToGlobal(QPoint(0, 0)), ui->frame_PageIconPicker->size());
        const QRect browseRect(ui->pushButton_Page_Icon_Browse->mapToGlobal(QPoint(0, 0)), ui->pushButton_Page_Icon_Browse->size());
        if (!pickerRect.contains(globalPos) && !browseRect.contains(globalPos)) {
            hideIconPicker();
        }

        return QWidget::eventFilter(watched, event);
    }

    void PageDetailsWidget::applySelectedIcon() {
        auto *item = ui->listWidget_Page_Icons ? ui->listWidget_Page_Icons->currentItem() : nullptr;
        if (!item) {
            hideIconPicker();
            return;
        }

        const QString iconPath = normalizedIconStoragePath(item->data(Qt::UserRole).toString());
        setPageIconPath(iconPath);
        hideIconPicker();
        emit iconPathSelected(iconPath);
    }

    void PageDetailsWidget::populateIconPicker() {
        if (!ui->listWidget_Page_Icons) {
            return;
        }

        const int iconSize = iconPickerIconSize();
        ui->listWidget_Page_Icons->setIconSize(QSize(iconSize, iconSize));
        ui->listWidget_Page_Icons->setGridSize(QSize(iconSize + 20, iconSize + 26));

        const QList<IconEntry> entries = availableIconEntries();
        for (const IconEntry &entry : entries) {
            auto *item = new QListWidgetItem(QIcon(iconPixmapForPath(entry.path, iconSize)), entry.label);
            item->setData(Qt::UserRole, entry.path);
            item->setToolTip(entry.path);
            ui->listWidget_Page_Icons->addItem(item);
        }
    }

    void PageDetailsWidget::ensureIconEntry(const QString &path) {
        if (path.isEmpty() || !ui->listWidget_Page_Icons) {
            return;
        }

        for (int i = 0; i < ui->listWidget_Page_Icons->count(); ++i) {
            auto *item = ui->listWidget_Page_Icons->item(i);
            if (item && item->data(Qt::UserRole).toString() == path) {
                return;
            }
        }

        const int iconSize = iconPickerIconSize();
        auto *item = new QListWidgetItem(QIcon(iconPixmapForPath(path, iconSize)), QFileInfo(path).completeBaseName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        ui->listWidget_Page_Icons->insertItem(0, item);
    }

    void PageDetailsWidget::updateIconPreview(const QString &path) {
        const int previewSize = 128;
        const QPixmap pixmap = iconPixmapForPath(path, previewSize);
        ui->label_Page_Icon->setPixmap(pixmap);
    }

    QString PageDetailsWidget::normalizedIconStoragePath(const QString &path) {
        const QString trimmed = path.trimmed();
        if (!trimmed.startsWith(QStringLiteral("file://"))) {
            return trimmed;
        }

        const QString localPath = trimmed.mid(QStringLiteral("file://").size());
        const QFileInfo info(localPath);
        if (!info.isAbsolute()) {
            return QStringLiteral("file://") + QDir::cleanPath(localPath);
        }

        const QString absolutePath = info.absoluteFilePath();
        const QStringList baseDirectories = {
            QCoreApplication::applicationDirPath(),
            QDir::currentPath()
        };

        for (const QString &baseDirectory : baseDirectories) {
            const QDir baseDir(baseDirectory);
            const QString relativePath = QDir::cleanPath(baseDir.relativeFilePath(absolutePath));
            if (!relativePath.startsWith(QStringLiteral("../")) && relativePath != QStringLiteral("..")) {
                return QStringLiteral("file://") + relativePath;
            }
        }

        return trimmed;
    }

    QString PageDetailsWidget::resolvedLocalIconPath(const QString &path) {
        const QString trimmed = path.trimmed();
        if (trimmed.isEmpty()) {
            return {};
        }

        const QString localPath = trimmed.startsWith(QStringLiteral("file://"))
                                      ? trimmed.mid(QStringLiteral("file://").size())
                                      : trimmed;
        const QFileInfo info(localPath);
        if (info.isAbsolute()) {
            return info.absoluteFilePath();
        }

        const QString executableRelativePath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(localPath);
        if (QFileInfo::exists(executableRelativePath)) {
            return executableRelativePath;
        }

        return QDir::current().absoluteFilePath(localPath);
    }

    QPixmap PageDetailsWidget::iconPixmapForPath(const QString &path, const int iconSize) {
        QPixmap pixmap;
        if (path.startsWith(QStringLiteral("file://"))) {
            pixmap.load(resolvedLocalIconPath(path));
        } else {
            pixmap.load(path);
        }

        if (pixmap.isNull()) {
            pixmap = QPixmap(QStringLiteral(":/resources/images/icons/webapp-256x256.png"));
        }

        return pixmap.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}
