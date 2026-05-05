//
// Created by Codex on 23/03/26.
//

#include "DrawerDialogHost.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSet>
#include <QShowEvent>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "GeoCoordinateEditorWidget.hpp"
#include "IconUtils.hpp"
#include "MainWindow.hpp"
#include "ui/widgets/TouchColorPicker.hpp"
#include "ui/widgets/TouchFileBrowser.hpp"
#include "ui/widgets/TouchIconBrowser.hpp"

namespace fairwindsk::ui::drawer {
    namespace {
        struct DrawerBrowserColors {
            QColor panelBackground;
            QColor cardBackground;
            QColor fieldBackground;
            QColor border;
            QColor text;
            QColor mutedText;
            QColor icon;
            QColor accentTrack;
            QColor accentTop;
            QColor accentMid;
            QColor accentBottom;
        };

        MainWindow *resolveMainWindow(QWidget *parent) {
            if (parent) {
                if (auto *mainWindow = qobject_cast<MainWindow *>(parent->window())) {
                    return mainWindow;
                }
            }
            return MainWindow::instance(parent);
        }

        int runDrawerDialog(QWidget *parent,
                            const QString &title,
                            QWidget *content,
                            const QList<ButtonSpec> &buttons,
                            const int defaultResult) {
            if (!content) {
                return defaultResult;
            }

            auto *mainWindow = resolveMainWindow(parent);
            if (!mainWindow) {
                content->deleteLater();
                return defaultResult;
            }

            QEventLoop loop;
            int result = defaultResult;
            bool completed = false;
            QList<DrawerButtonSpec> drawerButtons;
            drawerButtons.reserve(buttons.size());
            for (const auto &button : buttons) {
                drawerButtons.append({button.text, button.result, button.isDefault});
            }

            mainWindow->showDrawerAsync(
                title,
                content,
                drawerButtons,
                [&loop, &result, &completed](const int drawerResult) {
                    result = drawerResult;
                    completed = true;
                    loop.quit();
                },
                defaultResult);

            if (!completed) {
                loop.exec();
            }

            return result;
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

        DrawerBrowserColors effectiveDrawerBrowserColors(const QPalette &palette) {
            DrawerBrowserColors colors;
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const auto scrollPalette = fairWindSK
                ? fairWindSK->getActiveComfortScrollPalette(configuration)
                : fairwindsk::UiScrollPalette{};
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");

            colors.panelBackground = palette.color(QPalette::Window);
            colors.cardBackground = palette.color(QPalette::Base);
            colors.fieldBackground = palette.color(QPalette::AlternateBase);
            colors.border = palette.color(QPalette::Mid);
            colors.text = palette.color(QPalette::WindowText);
            colors.mutedText = palette.color(QPalette::Midlight);
            colors.accentTrack = scrollPalette.track.isValid() ? scrollPalette.track : palette.color(QPalette::Button);
            colors.accentTop = scrollPalette.handleTop.isValid() ? scrollPalette.handleTop : palette.color(QPalette::Button).lighter(145);
            colors.accentMid = scrollPalette.handleMid.isValid() ? scrollPalette.handleMid : palette.color(QPalette::Button).lighter(118);
            colors.accentBottom = scrollPalette.handleBottom.isValid() ? scrollPalette.handleBottom : palette.color(QPalette::Button).darker(116);
            colors.icon = fairwindsk::ui::comfortIconColor(
                configuration,
                preset,
                fairwindsk::ui::bestContrastingColor(
                    colors.accentMid,
                    {palette.color(QPalette::Text),
                     palette.color(QPalette::ButtonText),
                     palette.color(QPalette::WindowText)}));
            return colors;
        }

        class DrawerLogExplorerWidget final : public QWidget {
            Q_DECLARE_TR_FUNCTIONS(DrawerLogExplorerWidget)
        public:
            explicit DrawerLogExplorerWidget(const QString &directory, QWidget *parent = nullptr)
                : QWidget(parent),
                  m_directory(directory) {
                auto *layout = new QVBoxLayout(this);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(10);

                auto *directoryLabel = new QLabel(tr("Persistent log directory"), this);
                layout->addWidget(directoryLabel);

                m_directoryValue = new QLabel(this);
                m_directoryValue->setWordWrap(true);
                m_directoryValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
                m_directoryValue->setText(m_directory);
                layout->addWidget(m_directoryValue);

                auto *toolbarLayout = new QHBoxLayout();
                toolbarLayout->setContentsMargins(0, 0, 0, 0);
                toolbarLayout->setSpacing(8);
                layout->addLayout(toolbarLayout);

                m_refreshButton = new QPushButton(tr("Refresh"), this);
                m_refreshButton->setMinimumHeight(42);
                toolbarLayout->addWidget(m_refreshButton, 0);
                toolbarLayout->addStretch(1);

                auto *hintLabel = new QLabel(tr("Select a log file to inspect it inside the drawer."), this);
                hintLabel->setWordWrap(true);
                layout->addWidget(hintLabel);

                auto *splitter = new QSplitter(Qt::Horizontal, this);
                splitter->setChildrenCollapsible(false);
                layout->addWidget(splitter, 1);

                m_logList = new QListWidget(splitter);
                m_logList->setMinimumWidth(260);
                m_logList->setAlternatingRowColors(true);
                m_logList->setSelectionMode(QAbstractItemView::SingleSelection);
                m_logList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

                auto *detailsWidget = new QWidget(splitter);
                auto *detailsLayout = new QVBoxLayout(detailsWidget);
                detailsLayout->setContentsMargins(0, 0, 0, 0);
                detailsLayout->setSpacing(6);

                m_selectedLogLabel = new QLabel(tr("No log selected"), detailsWidget);
                m_selectedLogLabel->setWordWrap(true);
                m_selectedLogLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                detailsLayout->addWidget(m_selectedLogLabel);

                m_logSummaryLabel = new QLabel(tr("Choose a log file from the left pane."), detailsWidget);
                m_logSummaryLabel->setWordWrap(true);
                m_logSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                detailsLayout->addWidget(m_logSummaryLabel);

                m_logGrid = new QTableWidget(detailsWidget);
                m_logGrid->setColumnCount(4);
                m_logGrid->setHorizontalHeaderLabels({tr("Time"), tr("Level"), tr("Message"), tr("Source")});
                m_logGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
                m_logGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
                m_logGrid->setSelectionMode(QAbstractItemView::SingleSelection);
                m_logGrid->setAlternatingRowColors(true);
                m_logGrid->setWordWrap(true);
                m_logGrid->verticalHeader()->setVisible(false);
                m_logGrid->verticalHeader()->setDefaultSectionSize(28);
                m_logGrid->horizontalHeader()->setStretchLastSection(false);
                m_logGrid->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
                m_logGrid->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
                m_logGrid->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
                m_logGrid->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
                detailsLayout->addWidget(m_logGrid, 1);

                splitter->setStretchFactor(0, 0);
                splitter->setStretchFactor(1, 1);
                splitter->setSizes({280, 760});

                connect(m_refreshButton, &QPushButton::clicked, this, [this]() { reloadLogs(); });
                connect(m_logList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
                    loadLog(current ? current->data(Qt::UserRole).toString() : QString());
                });

                reloadLogs();
            }

        private:
            void reloadLogs() {
                const QString previouslySelected = m_logList->currentItem()
                                                      ? m_logList->currentItem()->data(Qt::UserRole).toString()
                                                      : QString();
                QSignalBlocker blocker(m_logList);
                m_logList->clear();

                QDir directory(m_directory);
                const QFileInfoList files = directory.entryInfoList(
                    QStringList() << QStringLiteral("*.log") << QStringLiteral("*.txt"),
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDir::Time | QDir::Reversed);

                for (const QFileInfo &fileInfo : files) {
                    const QString title = QStringLiteral("%1\n%2  •  %3 KB")
                                              .arg(fileInfo.fileName(),
                                                   QLocale().toString(fileInfo.lastModified(), QLocale::ShortFormat),
                                                   QString::number(qMax<qint64>(1, fileInfo.size() / 1024)));
                    auto *item = new QListWidgetItem(title, m_logList);
                    item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
                    item->setToolTip(fileInfo.absoluteFilePath());
                    if (fileInfo.absoluteFilePath() == previouslySelected) {
                        m_logList->setCurrentItem(item);
                    }
                }

                if (m_logList->count() == 0) {
                    m_selectedLogLabel->setText(tr("No log selected"));
                    m_logSummaryLabel->setText(tr("No log files are available in the persistent diagnostics directory yet."));
                    m_logGrid->setRowCount(0);
                    return;
                }

                if (!m_logList->currentItem()) {
                    m_logList->setCurrentRow(0);
                } else {
                    loadLog(m_logList->currentItem()->data(Qt::UserRole).toString());
                }
            }

            void loadLog(const QString &path) {
                struct ParsedLogLine {
                    QString timestamp;
                    QString level;
                    QString message;
                    QString source;
                };

                if (path.trimmed().isEmpty()) {
                    m_selectedLogLabel->setText(tr("No log selected"));
                    m_logSummaryLabel->setText(tr("Choose a log file from the left pane."));
                    m_logGrid->setRowCount(0);
                    return;
                }

                QFile file(path);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    m_selectedLogLabel->setText(path);
                    m_logSummaryLabel->setText(tr("Unable to read the selected log file."));
                    m_logGrid->setRowCount(1);
                    for (int column = 0; column < 4; ++column) {
                        m_logGrid->setItem(0, column, new QTableWidgetItem());
                    }
                    m_logGrid->item(0, 2)->setText(tr("Unable to read: %1").arg(path));
                    return;
                }

                const QString content = QString::fromUtf8(file.readAll());
                const QStringList lines = content.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
                const QRegularExpression pattern(
                    QStringLiteral(R"(^(\S+)\s+\[([^\]]+)\]\s+(.*?)(?:\s+\(([^()]+)\))?$)"));

                QList<ParsedLogLine> parsedLines;
                parsedLines.reserve(lines.size());
                for (const QString &rawLine : lines) {
                    const QString trimmedLine = rawLine.trimmed();
                    if (trimmedLine.isEmpty()) {
                        continue;
                    }

                    ParsedLogLine parsed;
                    const auto match = pattern.match(trimmedLine);
                    if (match.hasMatch()) {
                        parsed.timestamp = match.captured(1);
                        parsed.level = match.captured(2);
                        parsed.message = match.captured(3).trimmed();
                        parsed.source = match.captured(4).trimmed();
                    } else {
                        parsed.message = trimmedLine;
                    }
                    parsedLines.append(parsed);
                }

                m_selectedLogLabel->setText(path);
                m_logSummaryLabel->setText(
                    tr("%1 entries  •  %2")
                        .arg(parsedLines.size())
                        .arg(QLocale().formattedDataSize(QFileInfo(path).size())));

                m_logGrid->setRowCount(parsedLines.size());
                for (int row = 0; row < parsedLines.size(); ++row) {
                    const ParsedLogLine &parsed = parsedLines.at(row);
                    auto *timeItem = new QTableWidgetItem(parsed.timestamp);
                    auto *levelItem = new QTableWidgetItem(parsed.level);
                    auto *messageItem = new QTableWidgetItem(parsed.message);
                    auto *sourceItem = new QTableWidgetItem(parsed.source);
                    levelItem->setTextAlignment(Qt::AlignCenter);
                    m_logGrid->setItem(row, 0, timeItem);
                    m_logGrid->setItem(row, 1, levelItem);
                    m_logGrid->setItem(row, 2, messageItem);
                    m_logGrid->setItem(row, 3, sourceItem);
                }
                m_logGrid->resizeRowsToContents();
                m_logGrid->scrollToTop();
            }

            QString m_directory;
            QLabel *m_directoryValue = nullptr;
            QPushButton *m_refreshButton = nullptr;
            QListWidget *m_logList = nullptr;
            QLabel *m_selectedLogLabel = nullptr;
            QLabel *m_logSummaryLabel = nullptr;
            QTableWidget *m_logGrid = nullptr;
        };
    }

    int execDrawer(QWidget *parent, const QString &title, QWidget *content, const QList<ButtonSpec> &buttons, const int defaultResult) {
        return runDrawerDialog(parent, title, content, buttons, defaultResult);
    }

    QMessageBox::StandardButton message(QWidget *parent,
                                        const QMessageBox::Icon icon,
                                        const QString &title,
                                        const QString &text,
                                        const QMessageBox::StandardButtons buttons,
                                        const QMessageBox::StandardButton defaultButton) {
        auto *content = new QWidget();
        auto *layout = new QHBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);
        const auto colors = effectiveDrawerBrowserColors(content->palette());

        const QString glyph = iconGlyph(icon);
        if (!glyph.isEmpty()) {
            auto *iconLabel = new QLabel(glyph, content);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setFixedSize(30, 30);
            iconLabel->setStyleSheet(QStringLiteral(
                "QLabel {"
                " border: 1px solid %1;"
                " color: %2;"
                " border-radius: 15px;"
                " font-size: 18px;"
                " font-weight: 700;"
                " background: %3;"
                " }")
                .arg(fairwindsk::ui::comfortAlpha(colors.border, 144).name(QColor::HexArgb),
                     colors.text.name(),
                     fairwindsk::ui::comfortAlpha(colors.fieldBackground, 112).name(QColor::HexArgb)));
            layout->addWidget(iconLabel, 0, Qt::AlignTop);
        }

        auto *textLabel = new QLabel(text, content);
        textLabel->setWordWrap(true);
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

        const int result = runDrawerDialog(parent, title, content, specs, int(defaultButton));
        return QMessageBox::StandardButton(result);
    }

    QString getText(QWidget *parent,
                    const QString &title,
                    const QString &label,
                    const QString &text,
                    bool *ok,
                    const QLineEdit::EchoMode echo) {
        auto *content = new QWidget();
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *labelWidget = new QLabel(label, content);
        labelWidget->setWordWrap(true);
        layout->addWidget(labelWidget);

        auto *lineEdit = new QLineEdit(content);
        lineEdit->setEchoMode(echo);
        lineEdit->setText(text);
        layout->addWidget(lineEdit);

        const QList<ButtonSpec> drawerSpecs = {
            {QObject::tr("OK"), int(QMessageBox::Ok), true},
            {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
        };
        const int result = runDrawerDialog(parent, title, content, drawerSpecs, int(QMessageBox::Cancel));
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
            auto *browser = new fairwindsk::ui::widgets::TouchFileBrowser(
                fairwindsk::ui::widgets::TouchFileBrowser::Mode::OpenFile,
                currentDirectory,
                {},
                filters);
            QPointer<fairwindsk::ui::widgets::TouchFileBrowser> browserGuard(browser);
            const int result = execDrawer(parent, title, browser, {}, int(QMessageBox::Cancel));

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

    QString getIconPath(QWidget *parent,
                        const QString &title,
                        const QString &currentPath) {
        // Derive a sensible starting directory from the current icon path.
        QString directory;
        if (!currentPath.isEmpty()) {
            const QString resolved = fairwindsk::ui::widgets::TouchIconBrowser::resolvedLocalIconPath(currentPath);
            if (!resolved.isEmpty()) {
                directory = QFileInfo(resolved).absolutePath();
            }
        }
        if (directory.isEmpty() || !QFileInfo(directory).isDir()) {
            directory = defaultBrowserDirectory();
        }

        const QStringList imageFilters = {
            QStringLiteral("*.png"),
            QStringLiteral("*.jpg"),
            QStringLiteral("*.jpeg"),
            QStringLiteral("*.svg")
        };

        while (true) {
            auto *browser = new fairwindsk::ui::widgets::TouchFileBrowser(
                fairwindsk::ui::widgets::TouchFileBrowser::Mode::OpenFile,
                directory,
                {},
                imageFilters);
            QPointer<fairwindsk::ui::widgets::TouchFileBrowser> browserGuard(browser);
            const int result = execDrawer(parent, title, browser, {}, int(QMessageBox::Cancel));

            if (!browserGuard) {
                return {};
            }

            directory = browserGuard->currentDirectory();
            if (result != QMessageBox::Open) {
                return {};
            }

            QString message;
            if (!browserGuard->canAccept(&message)) {
                warning(parent, title, message);
                continue;
            }

            // Normalize to a relative file:// path when the file lives under
            // the application directory; store absolute file:// otherwise.
            return fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(
                QStringLiteral("file://") + browserGuard->selectedPath());
        }
    }

    QColor getColor(QWidget *parent,
                    const QString &title,
                    const QColor &initialColor,
                    bool *accepted,
                    const bool alphaEnabled) {
        return fairwindsk::ui::widgets::TouchColorPickerDialog::getColor(
            parent, title, initialColor, accepted, alphaEnabled);
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
            auto *browser = new fairwindsk::ui::widgets::TouchFileBrowser(
                fairwindsk::ui::widgets::TouchFileBrowser::Mode::SaveFile,
                currentDirectory,
                currentFileName,
                filters);
            QPointer<fairwindsk::ui::widgets::TouchFileBrowser> browserGuard(browser);
            const int result = execDrawer(parent, title, browser, {}, int(QMessageBox::Cancel));

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

    void exploreLogs(QWidget *parent,
                     const QString &title,
                     const QString &directory) {
        auto *content = new DrawerLogExplorerWidget(directory, parent);
        execDrawer(parent,
                   title,
                   content,
                   {
                       {QObject::tr("Close"), int(QMessageBox::Close), true}
                   },
                   int(QMessageBox::Close));
    }

    bool editGeoCoordinate(QWidget *parent,
                           const QString &title,
                           double *latitude,
                           double *longitude,
                           double *altitude,
                           QString *formatId) {
        if (!latitude || !longitude) {
            return false;
        }

        QString currentFormat = formatId && !formatId->isEmpty()
                                ? *formatId
                                : fairwindsk::FairWindSK::getInstance()->getConfiguration()->getCoordinateFormat();

        while (true) {
            auto *editor = new fairwindsk::ui::GeoCoordinateEditorWidget();
            editor->setCoordinate(*latitude, *longitude, altitude ? *altitude : 0.0, currentFormat);
            QPointer<fairwindsk::ui::GeoCoordinateEditorWidget> editorGuard(editor);
            QObject::connect(editor->applyButton(), &QPushButton::clicked, editor, [editor]() {
                if (auto *mainWindow = resolveMainWindow(editor)) {
                    mainWindow->finishActiveDrawer(int(QMessageBox::Ok));
                }
            });
            QObject::connect(editor->cancelButton(), &QPushButton::clicked, editor, [editor]() {
                if (auto *mainWindow = resolveMainWindow(editor)) {
                    mainWindow->finishActiveDrawer(int(QMessageBox::Cancel));
                }
            });
            const int result = execDrawer(parent, title, editor, {}, int(QMessageBox::Cancel));

            if (!editorGuard || result != QMessageBox::Ok) {
                return false;
            }

            QString message;
            double parsedLatitude = *latitude;
            double parsedLongitude = *longitude;
            double parsedAltitude = altitude ? *altitude : 0.0;
            if (!editorGuard->coordinate(&parsedLatitude, &parsedLongitude, altitude ? &parsedAltitude : nullptr, &message)) {
                warning(parent, title, message);
                currentFormat = editorGuard->formatId();
                continue;
            }

            *latitude = parsedLatitude;
            *longitude = parsedLongitude;
            if (altitude) {
                *altitude = parsedAltitude;
            }
            if (formatId) {
                *formatId = editorGuard->formatId();
            }
            return true;
        }
    }
}
