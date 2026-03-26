//
// Created by Codex on 23/03/26.
//

#include "DrawerDialogHost.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "GeoCoordinateEditorWidget.hpp"
#include "MainWindow.hpp"

namespace fairwindsk::ui::drawer {
    namespace {
        enum class FileBrowserMode {
            OpenFile,
            SaveFile
        };

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

        QString defaultBrowserDirectory() {
            const QString documentsDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (!documentsDirectory.isEmpty()) {
                return documentsDirectory;
            }
            return QDir::homePath();
        }

        QStringList wildcardFilters(const QString &filter) {
            QStringList result;
            const QStringList groups = filter.split(QStringLiteral(";;"), Qt::SkipEmptyParts);
            const QRegularExpression matcher(QStringLiteral("\\(([^\\)]*)\\)"));

            for (const auto &group : groups) {
                const auto match = matcher.match(group);
                if (!match.hasMatch()) {
                    continue;
                }

                const QStringList parts = match.captured(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
                for (const auto &part : parts) {
                    if (!result.contains(part)) {
                        result.append(part);
                    }
                }
            }

            return result;
        }

        QString preferredSuffix(const QStringList &filters) {
            for (const auto &filter : filters) {
                if (!filter.startsWith(QStringLiteral("*."))) {
                    continue;
                }

                const QString suffix = filter.mid(2);
                if (!suffix.isEmpty() && suffix != QStringLiteral("*")) {
                    return suffix;
                }
            }

            return {};
        }

        class DrawerFileBrowserWidget final : public QWidget {
        public:
            DrawerFileBrowserWidget(const FileBrowserMode mode,
                                    const QString &directory,
                                    const QString &fileName,
                                    const QStringList &nameFilters,
                                    QWidget *parent = nullptr)
                : QWidget(parent), m_mode(mode), m_nameFilters(nameFilters) {
                auto *layout = new QVBoxLayout(this);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(8);

                auto *toolbarLayout = new QHBoxLayout();
                toolbarLayout->setContentsMargins(0, 0, 0, 0);
                toolbarLayout->setSpacing(8);

                m_homeButton = new QToolButton(this);
                m_homeButton->setText(tr("Home"));
                toolbarLayout->addWidget(m_homeButton);

                m_upButton = new QToolButton(this);
                m_upButton->setText(tr("Up"));
                toolbarLayout->addWidget(m_upButton);

                m_pathEdit = new QLineEdit(this);
                m_pathEdit->setStyleSheet(
                    "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
                toolbarLayout->addWidget(m_pathEdit, 1);

                layout->addLayout(toolbarLayout);

                m_view = new QTreeView(this);
                m_view->setAlternatingRowColors(true);
                m_view->setRootIsDecorated(false);
                m_view->setItemsExpandable(false);
                m_view->setSortingEnabled(true);
                m_view->setUniformRowHeights(true);
                m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                m_view->sortByColumn(0, Qt::AscendingOrder);
                m_view->setStyleSheet(
                    "QTreeView { background: #111827; color: white; alternate-background-color: #0f172a; }"
                    "QTreeView::item:selected { background: #1d4ed8; color: white; }");
                layout->addWidget(m_view, 1);

                if (m_mode == FileBrowserMode::SaveFile) {
                    auto *nameLabel = new QLabel(tr("File name"), this);
                    nameLabel->setStyleSheet("QLabel { color: white; }");
                    layout->addWidget(nameLabel);

                    m_nameEdit = new QLineEdit(this);
                    m_nameEdit->setStyleSheet(
                        "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
                    m_nameEdit->setText(fileName);
                    layout->addWidget(m_nameEdit);
                }

                setMinimumHeight(m_mode == FileBrowserMode::SaveFile ? 430 : 400);

                m_model = new QFileSystemModel(this);
                m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
                if (!m_nameFilters.isEmpty()) {
                    m_model->setNameFilters(m_nameFilters);
                    m_model->setNameFilterDisables(false);
                }

                m_view->setModel(m_model);
                m_view->hideColumn(1);
                m_view->hideColumn(2);
                m_view->hideColumn(3);
                const int minimumVisibleRows = 10;
                const int rowHeight = qMax(24, m_view->sizeHintForRow(0));
                const int headerHeight = m_view->header()->sizeHint().height();
                const int frameHeight = m_view->frameWidth() * 2;
                m_view->setMinimumHeight(headerHeight + frameHeight + (rowHeight * minimumVisibleRows));

                const QString initialDirectory = QFileInfo(directory).isDir() ? directory : defaultBrowserDirectory();
                navigateTo(initialDirectory);

                connect(m_homeButton, &QToolButton::clicked, this, [this]() { navigateTo(defaultBrowserDirectory()); });
                connect(m_upButton, &QToolButton::clicked, this, [this]() {
                    QDir dir(currentDirectory());
                    if (dir.cdUp()) {
                        navigateTo(dir.absolutePath());
                    }
                });
                connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
                    const QFileInfo info(m_pathEdit->text().trimmed());
                    if (info.exists() && info.isDir()) {
                        navigateTo(info.absoluteFilePath());
                    }
                });
                connect(m_view, &QTreeView::clicked, this, [this](const QModelIndex &index) { handleSelection(index, false); });
                connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) { handleSelection(index, true); });
            }

            QString currentDirectory() const {
                return m_currentDirectory;
            }

            QString fileName() const {
                if (m_nameEdit) {
                    return m_nameEdit->text().trimmed();
                }

                const QFileInfo info(m_selectedPath);
                return info.isFile() ? info.fileName() : QString();
            }

            QString selectedPath() const {
                if (m_mode == FileBrowserMode::SaveFile) {
                    QString name = fileName();
                    if (name.isEmpty()) {
                        return {};
                    }

                    const QString suffix = preferredSuffix(m_nameFilters);
                    if (!suffix.isEmpty() && QFileInfo(name).suffix().isEmpty()) {
                        name += QStringLiteral(".") + suffix;
                    }

                    return QDir(m_currentDirectory).filePath(name);
                }

                return m_selectedPath;
            }

            bool canAccept(QString *message) const {
                const QString path = selectedPath();
                if (m_mode == FileBrowserMode::OpenFile) {
                    const QFileInfo info(path);
                    if (path.isEmpty() || !info.exists() || !info.isFile()) {
                        if (message) {
                            *message = tr("Select an existing file.");
                        }
                        return false;
                    }
                    return true;
                }

                if (path.isEmpty()) {
                    if (message) {
                        *message = tr("Enter a file name.");
                    }
                    return false;
                }

                const QFileInfo parentInfo(QFileInfo(path).absolutePath());
                if (!parentInfo.exists() || !parentInfo.isDir()) {
                    if (message) {
                        *message = tr("Select a valid destination folder.");
                    }
                    return false;
                }
                return true;
            }

        private:
            void navigateTo(const QString &path) {
                const QFileInfo info(path);
                if (!info.exists() || !info.isDir()) {
                    return;
                }

                m_currentDirectory = info.absoluteFilePath();
                const QModelIndex rootIndex = m_model->setRootPath(m_currentDirectory);
                m_view->setRootIndex(rootIndex);
                m_pathEdit->setText(m_currentDirectory);
                m_selectedPath.clear();
                m_upButton->setEnabled(QDir(m_currentDirectory).cdUp());
            }

            void handleSelection(const QModelIndex &index, const bool activate) {
                const QFileInfo info = m_model->fileInfo(index);
                if (!info.exists()) {
                    return;
                }

                if (info.isDir()) {
                    if (activate) {
                        navigateTo(info.absoluteFilePath());
                    }
                    return;
                }

                m_selectedPath = info.absoluteFilePath();
                if (m_nameEdit) {
                    m_nameEdit->setText(info.fileName());
                }
            }

            FileBrowserMode m_mode;
            QStringList m_nameFilters;
            QString m_currentDirectory;
            QString m_selectedPath;
            QFileSystemModel *m_model = nullptr;
            QTreeView *m_view = nullptr;
            QLineEdit *m_pathEdit = nullptr;
            QLineEdit *m_nameEdit = nullptr;
            QToolButton *m_homeButton = nullptr;
            QToolButton *m_upButton = nullptr;
        };
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

    QString getOpenFilePath(QWidget *parent,
                            const QString &title,
                            const QString &directory,
                            const QString &filter) {
        const QStringList filters = wildcardFilters(filter);
        QString currentDirectory = directory;

        while (true) {
            auto *browser = new DrawerFileBrowserWidget(FileBrowserMode::OpenFile, currentDirectory, {}, filters);
            QPointer<DrawerFileBrowserWidget> browserGuard(browser);
            const QList<DrawerButtonSpec> drawerSpecs = {
                {QObject::tr("Open"), int(QMessageBox::Open), true},
                {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
            };
            const int result = execDrawer(parent, title, browser, {
                {QObject::tr("Open"), int(QMessageBox::Open), true},
                {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
            }, int(QMessageBox::Cancel));

            if (!browserGuard) {
                return {};
            }

            currentDirectory = browserGuard->currentDirectory();
            if (result != QMessageBox::Open) {
                return {};
            }

            QString message;
            if (!browserGuard->canAccept(&message)) {
                warning(parent, title, message);
                continue;
            }

            return browserGuard->selectedPath();
        }
    }

    QString getSaveFilePath(QWidget *parent,
                            const QString &title,
                            const QString &path,
                            const QString &filter) {
        const QStringList filters = wildcardFilters(filter);
        QFileInfo initialInfo(path);
        QString currentDirectory = initialInfo.absolutePath();
        QString currentFileName = initialInfo.fileName();

        if (currentDirectory.isEmpty() || !QFileInfo(currentDirectory).isDir()) {
            currentDirectory = defaultBrowserDirectory();
        }

        while (true) {
            auto *browser = new DrawerFileBrowserWidget(FileBrowserMode::SaveFile, currentDirectory, currentFileName, filters);
            QPointer<DrawerFileBrowserWidget> browserGuard(browser);
            const int result = execDrawer(parent, title, browser, {
                {QObject::tr("Save"), int(QMessageBox::Save), true},
                {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
            }, int(QMessageBox::Cancel));

            if (!browserGuard) {
                return {};
            }

            currentDirectory = browserGuard->currentDirectory();
            currentFileName = browserGuard->fileName();
            if (result != QMessageBox::Save) {
                return {};
            }

            QString message;
            if (!browserGuard->canAccept(&message)) {
                warning(parent, title, message);
                continue;
            }

            const QString selectedPath = browserGuard->selectedPath();
            if (QFileInfo::exists(selectedPath)) {
                if (question(parent,
                             title,
                             QObject::tr("Replace existing file \"%1\"?").arg(QFileInfo(selectedPath).fileName()),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) {
                    continue;
                }
            }

            return selectedPath;
        }
    }

    bool editGeoCoordinate(QWidget *parent,
                           const QString &title,
                           double *latitude,
                           double *longitude,
                           QString *formatId) {
        if (!latitude || !longitude) {
            return false;
        }

        QString currentFormat = formatId && !formatId->isEmpty()
                                ? *formatId
                                : fairwindsk::FairWindSK::getInstance()->getConfiguration()->getCoordinateFormat();

        while (true) {
            auto *editor = new fairwindsk::ui::GeoCoordinateEditorWidget();
            editor->setCoordinate(*latitude, *longitude, currentFormat);
            QPointer<fairwindsk::ui::GeoCoordinateEditorWidget> editorGuard(editor);
            const int result = execDrawer(parent, title, editor, {
                {QObject::tr("Apply"), int(QMessageBox::Ok), true},
                {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
            }, int(QMessageBox::Cancel));

            if (!editorGuard || result != QMessageBox::Ok) {
                return false;
            }

            QString message;
            double parsedLatitude = *latitude;
            double parsedLongitude = *longitude;
            if (!editorGuard->coordinate(&parsedLatitude, &parsedLongitude, &message)) {
                warning(parent, title, message);
                currentFormat = editorGuard->formatId();
                continue;
            }

            *latitude = parsedLatitude;
            *longitude = parsedLongitude;
            if (formatId) {
                *formatId = editorGuard->formatId();
            }
            return true;
        }
    }
}
