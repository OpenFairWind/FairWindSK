//
// Created by Codex on 23/03/26.
//

#include "DrawerDialogHost.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
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

namespace fairwindsk::ui::drawer {
    namespace {
        enum class FileBrowserMode {
            OpenFile,
            SaveFile
        };

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

        QString drawerButtonStyle(const DrawerBrowserColors &colors, const bool accent = false) {
            const QColor top = accent ? colors.accentTop : colors.accentTrack.lighter(120);
            const QColor mid = accent ? colors.accentMid : colors.accentTrack;
            const QColor bottom = accent ? colors.accentBottom : colors.accentTrack.darker(118);
            const QColor border = accent ? colors.accentBottom : colors.border;

            return QStringLiteral(
                "QPushButton {"
                " min-width: 64px;"
                " min-height: 64px;"
                " padding: 0px;"
                " border: 1px solid %1;"
                " border-radius: 14px;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.42 %3, stop:1 %4);"
                " }"
                "QPushButton:hover {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %5, stop:0.42 %6, stop:1 %7);"
                " }"
                "QPushButton:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %8, stop:0.42 %9, stop:1 %10);"
                " }"
                "QPushButton:disabled {"
                " background: %11;"
                " border-color: %12;"
                " }")
                .arg(border.name(),
                     top.name(), mid.name(), bottom.name(),
                     top.lighter(108).name(), mid.lighter(104).name(), bottom.lighter(106).name(),
                     mid.darker(112).name(), bottom.darker(108).name(), bottom.darker(118).name(),
                     colors.fieldBackground.name(), colors.border.name());
        }

        QString drawerLineEditStyle(const DrawerBrowserColors &colors) {
            return QStringLiteral(
                "QLineEdit {"
                " min-height: 62px;"
                " padding: 0 18px;"
                " border: 1px solid %1;"
                " border-radius: 14px;"
                " background: %2;"
                " color: %3;"
                " font-size: 20px;"
                " }"
                "QLineEdit:focus {"
                " border-color: %4;"
                " }")
                .arg(colors.border.name(),
                     colors.fieldBackground.name(),
                     colors.text.name(),
                     colors.accentMid.name());
        }

        QString drawerLabelStyle(const DrawerBrowserColors &colors,
                                 const int pointSize,
                                 const int weight,
                                 const QColor &color = QColor()) {
            const QColor effectiveColor = color.isValid() ? color : colors.text;
            return QStringLiteral("color: %1; font-size: %2px; font-weight: %3;")
                .arg(effectiveColor.name())
                .arg(pointSize)
                .arg(weight);
        }

        QString drawerCardStyle(const DrawerBrowserColors &colors) {
            return QStringLiteral(
                "QFrame {"
                " border: 1px solid %1;"
                " border-radius: 18px;"
                " background: %2;"
                " }")
                .arg(colors.border.name(), colors.cardBackground.name());
        }

        QString drawerTreeStyle(const DrawerBrowserColors &colors) {
            return QStringLiteral(
                "QTreeView {"
                " background: transparent;"
                " color: %1;"
                " font-size: 20px;"
                " alternate-background-color: rgba(255, 255, 255, 0.03);"
                " border: none;"
                " outline: none;"
                " show-decoration-selected: 1;"
                " }"
                "QTreeView::item {"
                " min-height: 60px;"
                " padding: 8px 14px;"
                " border-radius: 10px;"
                " }"
                "QTreeView::item:selected {"
                " background: %2;"
                " color: %3;"
                " }"
                "QHeaderView::section {"
                " min-height: 48px;"
                " padding: 0 14px;"
                " background: %4;"
                " color: %5;"
                " border: none;"
                " border-bottom: 1px solid %6;"
                " font-size: 16px;"
                " font-weight: 700;"
                " }")
                .arg(colors.text.name(),
                     colors.accentTrack.name(),
                     colors.icon.name(),
                     colors.fieldBackground.name(),
                     colors.mutedText.name(),
                     colors.border.name());
        }

        class DrawerFileBrowserWidget final : public QWidget {
        public:
            DrawerFileBrowserWidget(const FileBrowserMode mode,
                                    const QString &directory,
                                    const QString &fileName,
                                    const QStringList &nameFilters,
                                    QWidget *parent = nullptr)
                : QWidget(parent), m_mode(mode), m_nameFilters(nameFilters) {
                setObjectName(QStringLiteral("drawerFileBrowserWidget"));
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                auto *layout = new QVBoxLayout(this);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(14);

                auto *titleRow = new QHBoxLayout();
                titleRow->setContentsMargins(0, 0, 0, 0);
                titleRow->setSpacing(12);

                m_titleLabel = new QLabel(m_mode == FileBrowserMode::SaveFile ? tr("Save file") : tr("Select file"), this);
                titleRow->addWidget(m_titleLabel);

                m_statusLabel = new QLabel(this);
                m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                titleRow->addWidget(m_statusLabel, 1);

                layout->addLayout(titleRow);

                auto *toolbarLayout = new QHBoxLayout();
                toolbarLayout->setContentsMargins(0, 0, 0, 0);
                toolbarLayout->setSpacing(12);

                m_backButton = new QPushButton(this);
                m_backButton->setObjectName(QStringLiteral("drawerNavButton"));
                m_backButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-left-google.svg")));
                m_backButton->setIconSize(QSize(32, 32));
                m_backButton->setToolTip(tr("Close"));
                toolbarLayout->addWidget(m_backButton);

                m_homeButton = new QPushButton(this);
                m_homeButton->setObjectName(QStringLiteral("drawerNavButton"));
                m_homeButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/home.svg")));
                m_homeButton->setIconSize(QSize(32, 32));
                m_homeButton->setToolTip(tr("Home"));
                toolbarLayout->addWidget(m_homeButton);

                m_upButton = new QPushButton(this);
                m_upButton->setObjectName(QStringLiteral("drawerNavButton"));
                m_upButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-up-google.svg")));
                m_upButton->setIconSize(QSize(32, 32));
                m_upButton->setToolTip(tr("Up"));
                toolbarLayout->addWidget(m_upButton);

                m_pathEdit = new QLineEdit(this);
                m_pathEdit->setObjectName(QStringLiteral("drawerPathEdit"));
                m_pathEdit->setPlaceholderText(tr("Enter a folder path"));
                toolbarLayout->addWidget(m_pathEdit, 1);

                m_acceptButton = new QPushButton(this);
                m_acceptButton->setObjectName(QStringLiteral("drawerActionButton"));
                m_acceptButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-right-google.svg")));
                m_acceptButton->setIconSize(QSize(32, 32));
                m_acceptButton->setToolTip(m_mode == FileBrowserMode::SaveFile ? tr("Save") : tr("Open"));
                toolbarLayout->addWidget(m_acceptButton);

                layout->addLayout(toolbarLayout);

                m_selectionLabel = new QLabel(this);
                layout->addWidget(m_selectionLabel);

                auto *browserFrame = new QFrame(this);
                auto *browserLayout = new QVBoxLayout(browserFrame);
                browserLayout->setContentsMargins(12, 12, 12, 12);
                browserLayout->setSpacing(10);

                m_view = new QTreeView(this);
                m_view->setAlternatingRowColors(true);
                m_view->setRootIsDecorated(false);
                m_view->setItemsExpandable(false);
                m_view->setSortingEnabled(true);
                m_view->setUniformRowHeights(true);
                m_view->setIndentation(0);
                m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
                m_view->setSelectionMode(QAbstractItemView::SingleSelection);
                m_view->setAllColumnsShowFocus(true);
                m_view->setIconSize(QSize(36, 36));
                m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                m_view->setTextElideMode(Qt::ElideMiddle);
                m_view->header()->setMinimumHeight(48);
                m_view->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_view->header()->setStretchLastSection(true);
                m_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
                m_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
                m_view->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
                m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                m_view->sortByColumn(0, Qt::AscendingOrder);
                browserLayout->addWidget(m_view, 1);
                layout->addWidget(browserFrame, 1);

                m_browserFrame = browserFrame;

                if (m_mode == FileBrowserMode::SaveFile) {
                    auto *nameLabel = new QLabel(tr("File name"), this);
                    nameLabel->setObjectName(QStringLiteral("drawerFileNameLabel"));
                    layout->addWidget(nameLabel);

                    m_nameEdit = new QLineEdit(this);
                    m_nameEdit->setObjectName(QStringLiteral("drawerFileNameEdit"));
                    m_nameEdit->setText(fileName);
                    layout->addWidget(m_nameEdit);
                }

                setMinimumHeight(m_mode == FileBrowserMode::SaveFile ? 780 : 740);

                m_model = new QFileSystemModel(this);
                m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
                if (!m_nameFilters.isEmpty()) {
                    m_model->setNameFilters(m_nameFilters);
                    m_model->setNameFilterDisables(false);
                }

                m_view->setModel(m_model);
                m_view->hideColumn(2);
                const int minimumVisibleRows = (m_mode == FileBrowserMode::SaveFile) ? 11 : 10;
                const int rowHeight = 60;
                const int headerHeight = m_view->header()->sizeHint().height();
                const int frameHeight = m_view->frameWidth() * 2;
                m_view->setMinimumHeight(headerHeight + frameHeight + (rowHeight * minimumVisibleRows));

                const QString initialDirectory = QFileInfo(directory).isDir() ? directory : defaultBrowserDirectory();
                navigateTo(initialDirectory);
                updateSelectionSummary();
                applyComfortChrome();

                connect(m_backButton, &QPushButton::clicked, this, [this]() { finishDrawer(int(QMessageBox::Cancel)); });
                connect(m_homeButton, &QPushButton::clicked, this, [this]() { navigateTo(defaultBrowserDirectory()); });
                connect(m_upButton, &QPushButton::clicked, this, [this]() {
                    QDir dir(currentDirectory());
                    if (dir.cdUp()) {
                        navigateTo(dir.absolutePath());
                    }
                });
                connect(m_acceptButton, &QPushButton::clicked, this, [this]() {
                    finishDrawer(m_mode == FileBrowserMode::SaveFile ? int(QMessageBox::Save) : int(QMessageBox::Open));
                });
                connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
                    const QFileInfo info(m_pathEdit->text().trimmed());
                    if (info.exists() && info.isDir()) {
                        navigateTo(info.absoluteFilePath());
                    }
                });
                if (m_nameEdit) {
                    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { updateSelectionSummary(); });
                }
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

        protected:
            void showEvent(QShowEvent *event) override {
                QWidget::showEvent(event);
                updateDrawerGeometry();
            }

            void changeEvent(QEvent *event) override {
                QWidget::changeEvent(event);
                if (event && (event->type() == QEvent::PaletteChange
                              || event->type() == QEvent::ApplicationPaletteChange
                              || event->type() == QEvent::StyleChange)) {
                    applyComfortChrome();
                }
            }

            bool eventFilter(QObject *watched, QEvent *event) override {
                if (watched == m_geometryHost && event && event->type() == QEvent::Resize) {
                    updateDrawerGeometry();
                }
                return QWidget::eventFilter(watched, event);
            }

        private:
            void updateDrawerGeometry() {
                auto *mainWindow = resolveMainWindow(this);
                if (!mainWindow || !mainWindow->getUi() || !mainWindow->getUi()->stackedWidget_Center) {
                    return;
                }

                const int availableHeight = mainWindow->getUi()->stackedWidget_Center->height();
                if (availableHeight <= 0) {
                    return;
                }

                if (m_geometryHost != mainWindow->getUi()->stackedWidget_Center) {
                    if (m_geometryHost) {
                        m_geometryHost->removeEventFilter(this);
                    }
                    m_geometryHost = mainWindow->getUi()->stackedWidget_Center;
                    m_geometryHost->installEventFilter(this);
                }

                const int targetHeight = qMax(availableHeight - 8, m_mode == FileBrowserMode::SaveFile ? 780 : 740);
                setMinimumHeight(targetHeight);
                setMaximumHeight(targetHeight);
            }

            void finishDrawer(const int result) const {
                if (auto *mainWindow = resolveMainWindow(const_cast<DrawerFileBrowserWidget *>(this))) {
                    mainWindow->finishActiveDrawer(result);
                }
            }

            void applyComfortChrome() {
                const DrawerBrowserColors colors = effectiveDrawerBrowserColors(palette());

                setStyleSheet(QStringLiteral("QWidget#drawerFileBrowserWidget { background: %1; }").arg(colors.panelBackground.name()));
                if (m_titleLabel) {
                    m_titleLabel->setStyleSheet(drawerLabelStyle(colors, 26, 700));
                }
                if (m_statusLabel) {
                    m_statusLabel->setStyleSheet(drawerLabelStyle(colors, 16, 600, colors.mutedText));
                }
                if (m_selectionLabel) {
                    m_selectionLabel->setStyleSheet(drawerLabelStyle(colors, 18, 600));
                }
                if (m_browserFrame) {
                    m_browserFrame->setStyleSheet(drawerCardStyle(colors));
                }
                if (m_nameEdit) {
                    m_nameEdit->setStyleSheet(drawerLineEditStyle(colors));
                }
                if (m_pathEdit) {
                    m_pathEdit->setStyleSheet(drawerLineEditStyle(colors));
                }
                if (m_backButton) {
                    m_backButton->setStyleSheet(drawerButtonStyle(colors));
                }
                if (m_homeButton) {
                    m_homeButton->setStyleSheet(drawerButtonStyle(colors));
                }
                if (m_upButton) {
                    m_upButton->setStyleSheet(drawerButtonStyle(colors));
                }
                if (m_acceptButton) {
                    m_acceptButton->setStyleSheet(drawerButtonStyle(colors, true));
                }
                if (m_view) {
                    m_view->setStyleSheet(drawerTreeStyle(colors) + fairwindsk::ui::widgets::TouchScrollArea::scrollBarStyleSheet());
                }

                auto *fairWindSK = FairWindSK::getInstance();
                const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
                const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
                const QColor fallbackIconColor = fairwindsk::ui::bestContrastingColor(
                    palette().color(QPalette::Button),
                    {palette().color(QPalette::Text),
                     palette().color(QPalette::ButtonText),
                     palette().color(QPalette::WindowText)});
                const QColor iconColor = fairwindsk::ui::comfortIconColor(configuration, preset, fallbackIconColor);

                fairwindsk::ui::applyTintedButtonIcon(m_backButton, iconColor, QSize(28, 28));
                fairwindsk::ui::applyTintedButtonIcon(m_homeButton, iconColor, QSize(28, 28));
                fairwindsk::ui::applyTintedButtonIcon(m_upButton, iconColor, QSize(28, 28));
                fairwindsk::ui::applyTintedButtonIcon(m_acceptButton, iconColor, QSize(28, 28));
            }

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
                if (m_view->selectionModel()) {
                    m_view->selectionModel()->clearSelection();
                }
                updateSelectionSummary();
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
                updateSelectionSummary();
                if (activate) {
                    finishDrawer(m_mode == FileBrowserMode::SaveFile ? int(QMessageBox::Save) : int(QMessageBox::Open));
                }
            }

            void updateSelectionSummary() {
                const QString currentName = fileName();
                if (m_statusLabel) {
                    const QFileInfo directoryInfo(m_currentDirectory);
                    m_statusLabel->setText(directoryInfo.fileName().isEmpty() ? m_currentDirectory : directoryInfo.fileName());
                }

                if (!m_selectionLabel) {
                    return;
                }

                if (m_mode == FileBrowserMode::SaveFile) {
                    if (currentName.isEmpty()) {
                        m_selectionLabel->setText(tr("Choose a destination folder and enter a file name."));
                    } else {
                        m_selectionLabel->setText(tr("Destination: %1").arg(QDir(m_currentDirectory).filePath(currentName)));
                    }
                    return;
                }

                if (m_selectedPath.isEmpty()) {
                    m_selectionLabel->setText(tr("Tap a file once to select it, or double tap to open it."));
                } else {
                    m_selectionLabel->setText(tr("Selected file: %1").arg(QFileInfo(m_selectedPath).fileName()));
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
            QLabel *m_titleLabel = nullptr;
            QLabel *m_statusLabel = nullptr;
            QLabel *m_selectionLabel = nullptr;
            QFrame *m_browserFrame = nullptr;
            QPushButton *m_backButton = nullptr;
            QPushButton *m_homeButton = nullptr;
            QPushButton *m_upButton = nullptr;
            QPushButton *m_acceptButton = nullptr;
            QWidget *m_geometryHost = nullptr;
        };

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

        QString normalizedIconStoragePath(const QString &path) {
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

        QString resolvedLocalIconPath(const QString &path) {
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
                QDir dir(directoryPath);
                if (!dir.exists()) {
                    continue;
                }
                const QFileInfoList iconFiles = dir.entryInfoList(
                    QStringList() << QStringLiteral("*.png") << QStringLiteral("*.svg") << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg"),
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDir::Name);
                for (const QFileInfo &fileInfo : iconFiles) {
                    appendIconEntry(entries,
                                    seen,
                                    normalizedIconStoragePath(QStringLiteral("file://") + fileInfo.absoluteFilePath()),
                                    fileInfo.completeBaseName());
                }
            }

            return entries;
        }

        QPixmap iconPixmapForPath(const QString &path, const int iconSize) {
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

        class DrawerIconPickerWidget final : public QWidget {
        public:
            explicit DrawerIconPickerWidget(const QString &currentPath, QWidget *parent = nullptr)
                : QWidget(parent), m_currentPath(currentPath.trimmed()) {
                auto *layout = new QVBoxLayout(this);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(8);

                m_previewLabel = new QLabel(this);
                m_previewLabel->setAlignment(Qt::AlignCenter);
                m_previewLabel->setMinimumHeight(110);
                layout->addWidget(m_previewLabel);

                m_listWidget = new QListWidget(this);
                m_listWidget->setViewMode(QListView::IconMode);
                m_listWidget->setFlow(QListView::LeftToRight);
                m_listWidget->setWrapping(true);
                m_listWidget->setResizeMode(QListView::Adjust);
                m_listWidget->setMovement(QListView::Static);
                m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
                m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                const int iconSize = iconPickerIconSize();
                m_listWidget->setIconSize(QSize(iconSize, iconSize));
                m_listWidget->setGridSize(QSize(iconSize + 36, iconSize + 44));
                const int visibleRows = 4;
                m_listWidget->setMinimumHeight((iconSize + 44) * visibleRows + 18);
                layout->addWidget(m_listWidget, 1);

                const QList<IconEntry> entries = availableIconEntries();
                for (const IconEntry &entry : entries) {
                    auto *item = new QListWidgetItem(QIcon(iconPixmapForPath(entry.path, iconSize)), entry.label);
                    item->setData(Qt::UserRole, entry.path);
                    item->setToolTip(entry.path);
                    m_listWidget->addItem(item);
                    if (entry.path == m_currentPath) {
                        m_listWidget->setCurrentItem(item);
                    }
                }

                if (!m_currentPath.isEmpty() && !m_listWidget->currentItem()) {
                    const QPixmap pixmap = iconPixmapForPath(m_currentPath, iconSize);
                    auto *item = new QListWidgetItem(QIcon(pixmap), QFileInfo(m_currentPath).completeBaseName());
                    item->setData(Qt::UserRole, m_currentPath);
                    item->setToolTip(m_currentPath);
                    m_listWidget->insertItem(0, item);
                    m_listWidget->setCurrentItem(item);
                }

                if (!m_listWidget->currentItem() && m_listWidget->count() > 0) {
                    m_listWidget->setCurrentRow(0);
                }

                updatePreview();
                QObject::connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() { updatePreview(); });
                QObject::connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this]() {
                    if (auto *mainWindow = resolveMainWindow(this)) {
                        mainWindow->finishActiveDrawer(int(QMessageBox::Ok));
                    }
                });
            }

            QString selectedPath() const {
                auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
                return item ? item->data(Qt::UserRole).toString() : QString();
            }

        private:
            void updatePreview() {
                auto *item = m_listWidget ? m_listWidget->currentItem() : nullptr;
                if (!item) {
                    m_previewLabel->clear();
                    return;
                }

                const int previewSize = std::max(96, iconPickerIconSize() + 24);
                const QPixmap pixmap = iconPixmapForPath(item->data(Qt::UserRole).toString(), previewSize);
                m_previewLabel->setPixmap(pixmap);
            }

            QString m_currentPath;
            QLabel *m_previewLabel = nullptr;
            QListWidget *m_listWidget = nullptr;
        };

        class DrawerLogExplorerWidget final : public QWidget {
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
                "QLabel { border: 1px solid rgba(127, 127, 127, 0.35); border-radius: 15px; font-size: 18px; font-weight: 700; }");
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
        layout->addWidget(labelWidget);

        auto *lineEdit = new QLineEdit(content);
        lineEdit->setEchoMode(echo);
        lineEdit->setText(text);
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
        auto *picker = new DrawerIconPickerWidget(currentPath);
        QPointer<DrawerIconPickerWidget> pickerGuard(picker);
        const int result = execDrawer(parent, title, picker, {
            {QObject::tr("Select"), int(QMessageBox::Ok), true},
            {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
        }, int(QMessageBox::Cancel));

        if (!pickerGuard || result != QMessageBox::Ok) {
            return {};
        }

        return normalizedIconStoragePath(pickerGuard->selectedPath());
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
            auto *mainWindow = resolveMainWindow(parent);
            QList<ButtonSpec> buttons;
            if (mainWindow) {
                QObject::connect(editor->applyButton(), &QPushButton::clicked, mainWindow, [mainWindow]() {
                    mainWindow->finishActiveDrawer(int(QMessageBox::Ok));
                });
                QObject::connect(editor->cancelButton(), &QPushButton::clicked, mainWindow, [mainWindow]() {
                    mainWindow->finishActiveDrawer(int(QMessageBox::Cancel));
                });
            } else {
                buttons = {
                    {QObject::tr("Apply"), int(QMessageBox::Ok), true},
                    {QObject::tr("Cancel"), int(QMessageBox::Cancel), false}
                };
                editor->applyButton()->setVisible(false);
                editor->cancelButton()->setVisible(false);
            }
            const int result = execDrawer(parent, title, editor, buttons, int(QMessageBox::Cancel));

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
