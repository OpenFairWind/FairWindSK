//
// Created by Codex on 15/04/26.
//

#include "TouchIconBrowser.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {
    namespace {
        struct IconEntry {
            QString path;
            QString label;
        };

        int iconBrowserIconSize() {
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

        void appendIconEntry(QList<IconEntry> &entries,
                             QSet<QString> &seen,
                             const QString &path,
                             const QString &label = QString()) {
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
                                    TouchIconBrowser::normalizedIconStoragePath(QStringLiteral("file://") + fileInfo.absoluteFilePath()),
                                    fileInfo.completeBaseName());
                }
            }

            return entries;
        }
    }

    TouchIconBrowser::TouchIconBrowser(QWidget *parent)
        : QWidget(parent) {
        setObjectName(QStringLiteral("touchIconBrowser"));
        setProperty("drawerFillCenterArea", true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumHeight(0);

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(12);

        auto *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(12);
        rootLayout->addLayout(headerLayout);

        m_cancelButton = new QPushButton(this);
        m_cancelButton->setObjectName(QStringLiteral("touchIconBrowserBackButton"));
        m_cancelButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg")));
        m_cancelButton->setIconSize(QSize(32, 32));
        m_cancelButton->setMinimumSize(64, 64);
        m_cancelButton->setToolTip(tr("Cancel"));
        headerLayout->addWidget(m_cancelButton, 0, Qt::AlignTop);

        auto *headerLabel = new QLabel(tr("Choose an icon. Tap once to preview it, then double tap or double click an icon to select it."), this);
        headerLabel->setWordWrap(true);
        headerLayout->addWidget(headerLabel, 1);

        auto *previewFrame = new QFrame(this);
        m_previewFrame = previewFrame;
        previewFrame->setFrameShape(QFrame::StyledPanel);
        auto *previewLayout = new QHBoxLayout(previewFrame);
        previewLayout->setContentsMargins(12, 12, 12, 12);
        previewLayout->setSpacing(12);

        m_previewLabel = new QLabel(previewFrame);
        m_previewLabel->setMinimumSize(112, 112);
        m_previewLabel->setMaximumSize(112, 112);
        m_previewLabel->setAlignment(Qt::AlignCenter);
        previewLayout->addWidget(m_previewLabel, 0, Qt::AlignTop);

        m_selectionLabel = new QLabel(previewFrame);
        m_selectionLabel->setWordWrap(true);
        previewLayout->addWidget(m_selectionLabel, 1);

        rootLayout->addWidget(previewFrame);

        m_listWidget = new QListWidget(this);
        m_listWidget->setViewMode(QListView::IconMode);
        m_listWidget->setMovement(QListView::Static);
        m_listWidget->setFlow(QListView::LeftToRight);
        m_listWidget->setWrapping(true);
        m_listWidget->setResizeMode(QListView::Adjust);
        m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setSpacing(10);
        m_listWidget->setMinimumHeight(0);
        rootLayout->addWidget(m_listWidget, 1);

        populate();

        connect(m_listWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
            const QString path = current ? current->data(Qt::UserRole).toString() : QString();
            updatePreview(path);
            emit pathSelected(path);
        });
        connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            if (!item) {
                return;
            }
            const QString path = item->data(Qt::UserRole).toString();
            emit pathActivated(path);
        });
        connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
            emit canceled();
        });

        applyComfortChrome();
    }

    void TouchIconBrowser::setCurrentPath(const QString &path) {
        m_currentPath = normalizedIconStoragePath(path.trimmed());
        ensureIconEntry(m_currentPath);

        if (!m_listWidget) {
            updatePreview(m_currentPath);
            return;
        }

        for (int i = 0; i < m_listWidget->count(); ++i) {
            auto *item = m_listWidget->item(i);
            if (item && item->data(Qt::UserRole).toString() == m_currentPath) {
                m_listWidget->setCurrentItem(item);
                updatePreview(m_currentPath);
                return;
            }
        }

        updatePreview(m_currentPath);
    }

    QString TouchIconBrowser::currentPath() const {
        return m_currentPath;
    }

    QString TouchIconBrowser::selectedPath() const {
        auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
        return item ? normalizedIconStoragePath(item->data(Qt::UserRole).toString()) : m_currentPath;
    }

    void TouchIconBrowser::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            applyComfortChrome();
        }
    }

    QString TouchIconBrowser::normalizedIconStoragePath(const QString &path) {
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

    QString TouchIconBrowser::resolvedLocalIconPath(const QString &path) {
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

    QPixmap TouchIconBrowser::iconPixmapForPath(const QString &path, const int iconSize) {
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

    void TouchIconBrowser::populate() {
        if (!m_listWidget) {
            return;
        }

        const int iconSize = iconBrowserIconSize();
        m_listWidget->setIconSize(QSize(iconSize, iconSize));
        m_listWidget->setGridSize(QSize(iconSize + 52, iconSize + 76));

        const QList<IconEntry> entries = availableIconEntries();
        for (const IconEntry &entry : entries) {
            auto *item = new QListWidgetItem(QIcon(iconPixmapForPath(entry.path, iconSize)), entry.label);
            item->setData(Qt::UserRole, entry.path);
            item->setToolTip(entry.path);
            m_listWidget->addItem(item);
        }
    }

    void TouchIconBrowser::ensureIconEntry(const QString &path) {
        if (path.isEmpty() || !m_listWidget) {
            return;
        }

        for (int i = 0; i < m_listWidget->count(); ++i) {
            auto *item = m_listWidget->item(i);
            if (item && item->data(Qt::UserRole).toString() == path) {
                return;
            }
        }

        const int iconSize = iconBrowserIconSize();
        auto *item = new QListWidgetItem(QIcon(iconPixmapForPath(path, iconSize)), QFileInfo(path).completeBaseName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        m_listWidget->insertItem(0, item);
    }

    void TouchIconBrowser::updatePreview(const QString &path) {
        m_currentPath = normalizedIconStoragePath(path);
        if (m_previewLabel) {
            m_previewLabel->setPixmap(iconPixmapForPath(m_currentPath, 112));
        }
        if (m_selectionLabel) {
            if (m_currentPath.isEmpty()) {
                m_selectionLabel->setText(tr("No icon selected."));
            } else {
                m_selectionLabel->setText(tr("Selected icon:\n%1").arg(m_currentPath));
            }
        }
    }

    void TouchIconBrowser::applyComfortChrome() {
        if (m_isApplyingComfortChrome) {
            return;
        }
        m_isApplyingComfortChrome = true;

        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const auto accentChrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), true);

        setStyleSheet(QStringLiteral(
            "QWidget#touchIconBrowser { background: %1; color: %2; }"
            "QLabel { color: %2; font-size: 20px; }"
            "QFrame { background: %3; border: 1px solid %4; border-radius: 18px; }"
            "QListWidget {"
            " background: %3;"
            " color: %2;"
            " border: 1px solid %4;"
            " border-radius: 18px;"
            " padding: 12px;"
            " outline: none;"
            " }"
            "QListWidget::item {"
            " min-width: 124px;"
            " min-height: 144px;"
            " padding: 12px 10px;"
            " border-radius: 16px;"
            " }"
            "QListWidget::item:selected {"
            " background: %5;"
            " color: %6;"
            " }"
            "QPushButton {"
            " min-height: 64px;"
            " min-width: 64px;"
            " border-radius: 16px;"
            " border: 1px solid %4;"
            " background: %7;"
            " color: %8;"
            " font-size: 20px;"
            " font-weight: 700;"
            " }"
            "QPushButton:hover { background: %9; }"
            "QPushButton:pressed { background: %10; }")
            .arg(chrome.window.name(),
                 chrome.text.name(),
                 palette().color(QPalette::Base).name(),
                 chrome.border.name(),
                 chrome.accentTop.name(),
                 chrome.accentText.name(),
                 chrome.buttonBackground.name(),
                 chrome.buttonText.name(),
                 chrome.hoverBackground.name(),
                 chrome.pressedBackground.name()));

        fairwindsk::ui::applyTintedButtonIcon(m_cancelButton, accentChrome.icon, QSize(30, 30));
        m_isApplyingComfortChrome = false;
    }
}
