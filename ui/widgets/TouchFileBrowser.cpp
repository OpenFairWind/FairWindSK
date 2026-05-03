//
// Created by Codex on 14/04/26.
//

#include "TouchFileBrowser.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QScopedValueRollback>
#include <QShowEvent>
#include <QSortFilterProxyModel>

#include "FairWindSK.hpp"
#include "ui/MainWindow.hpp"
#include "ui/widgets/TouchScrollArea.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::widgets {
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

        fairwindsk::ui::MainWindow *resolveMainWindow(QWidget *context) {
            QWidget *candidate = context;
            while (candidate) {
                if (auto *mainWindow = qobject_cast<fairwindsk::ui::MainWindow *>(candidate)) {
                    return mainWindow;
                }
                candidate = candidate->parentWidget();
            }
            return fairwindsk::ui::MainWindow::instance(context);
        }

        DrawerBrowserColors effectiveDrawerBrowserColors(const QPalette &palette) {
            DrawerBrowserColors colors;

            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const auto scrollPalette = fairWindSK
                ? fairWindSK->getActiveComfortScrollPalette(configuration)
                : fairwindsk::UiScrollPalette{};
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");

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
                " min-height: 64px;"
                " min-width: 64px;"
                " padding: 8px 14px;"
                " border-radius: 16px;"
                " border: 1px solid %1;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.55 %3, stop:1 %4);"
                " color: %5;"
                " font-size: 20px;"
                " font-weight: 700;"
                " }"
                "QPushButton:hover {"
                " border-color: %6;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %7, stop:0.55 %8, stop:1 %9);"
                " }"
                "QPushButton:pressed {"
                " border-color: %10;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %11, stop:0.55 %3, stop:1 %12);"
                " }")
                .arg(border.name(),
                     top.name(),
                     mid.name(),
                     bottom.name(),
                     colors.icon.name(),
                     colors.icon.lighter(120).name(),
                     top.lighter(108).name(),
                     mid.lighter(112).name(),
                     bottom.lighter(112).name(),
                     colors.icon.darker(125).name(),
                     top.darker(118).name(),
                     bottom.darker(125).name());
        }

        QString drawerLineEditStyle(const DrawerBrowserColors &colors) {
            return QStringLiteral(
                "QLineEdit {"
                " min-height: 66px;"
                " border: 1px solid %1;"
                " border-radius: 16px;"
                " padding: 8px 18px;"
                " background: %2;"
                " color: %3;"
                " font-size: 34px;"
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
                " }"
                "QHeaderView::section:first {"
                " border-top-left-radius: 10px;"
                " }"
                "QHeaderView::section:last {"
                " border-top-right-radius: 10px;"
                " }"
                "QHeaderView::up-arrow, QHeaderView::down-arrow {"
                " width: 0px;"
                " height: 0px;"
                " }")
                .arg(colors.text.name(),
                     colors.accentTrack.name(),
                     colors.icon.name(),
                     colors.fieldBackground.name(),
                     colors.mutedText.name(),
                     colors.border.name());
        }
    }

    TouchFileBrowser::TouchFileBrowser(const Mode mode,
                                       const QString &directory,
                                       const QString &fileName,
                                       const QStringList &nameFilters,
                                       QWidget *parent)
        : QWidget(parent), m_mode(mode), m_nameFilters(nameFilters) {
        setObjectName(QStringLiteral("touchFileBrowser"));
        setProperty("drawerFillCenterArea", true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(14);

        auto *titleRow = new QHBoxLayout();
        titleRow->setContentsMargins(0, 0, 0, 0);
        titleRow->setSpacing(12);

        m_titleLabel = new QLabel(m_mode == Mode::SaveFile ? tr("Save file") : tr("Select file"), this);
        if (m_mode == Mode::OpenFile) {
            m_titleLabel->hide();
        }
        titleRow->addWidget(m_titleLabel);
        titleRow->addStretch(1);
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
        m_acceptButton->setToolTip(m_mode == Mode::SaveFile ? tr("Save") : tr("Open"));
        toolbarLayout->addWidget(m_acceptButton);

        layout->addLayout(toolbarLayout);

        if (m_mode == Mode::SaveFile) {
            m_selectionLabel = new QLabel(this);
            layout->addWidget(m_selectionLabel);
        }

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
        // Column 0 (Name) stretches; columns 1 (Size) and 3 (Date) size to content.
        // stretchLastSection=false is required so it does not override the column 0
        // Stretch mode and collapse the name column to icon-only width.
        m_view->header()->setStretchLastSection(false);
        m_view->header()->setSortIndicatorShown(false);
        m_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_view->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_view->setMinimumHeight(0);
        m_view->sortByColumn(0, Qt::AscendingOrder);
        browserLayout->addWidget(m_view, 1);
        layout->addWidget(browserFrame, 1);

        m_browserFrame = browserFrame;

        if (m_mode == Mode::SaveFile) {
            auto *nameLabel = new QLabel(tr("File name"), this);
            nameLabel->setObjectName(QStringLiteral("drawerFileNameLabel"));
            layout->addWidget(nameLabel);

            m_nameEdit = new QLineEdit(this);
            m_nameEdit->setObjectName(QStringLiteral("drawerFileNameEdit"));
            m_nameEdit->setText(fileName);
            layout->addWidget(m_nameEdit);
        }

        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);

        m_model = new QFileSystemModel(this);
        m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        if (!m_nameFilters.isEmpty()) {
            m_model->setNameFilters(m_nameFilters);
            m_model->setNameFilterDisables(false);
        }

        m_view->setModel(m_model);
        m_view->hideColumn(2);
        browserFrame->setMinimumHeight(0);

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
            finishDrawer(m_mode == Mode::SaveFile ? int(QMessageBox::Save) : int(QMessageBox::Open));
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

    QString TouchFileBrowser::currentDirectory() const {
        return m_currentDirectory;
    }

    QString TouchFileBrowser::fileName() const {
        if (m_nameEdit) {
            return m_nameEdit->text().trimmed();
        }

        const QFileInfo info(m_selectedPath);
        return info.isFile() ? info.fileName() : QString();
    }

    QString TouchFileBrowser::selectedPath() const {
        if (m_mode == Mode::SaveFile) {
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

    bool TouchFileBrowser::canAccept(QString *message) const {
        const QString path = selectedPath();
        if (m_mode == Mode::OpenFile) {
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

    void TouchFileBrowser::showEvent(QShowEvent *event) {
        QWidget::showEvent(event);
        updateGeometryToHost();
    }

    void TouchFileBrowser::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            applyComfortChrome();
        }
    }

    bool TouchFileBrowser::eventFilter(QObject *watched, QEvent *event) {
        if (watched == m_geometryHost && event && event->type() == QEvent::Resize) {
            updateGeometryToHost();
        }
        return QWidget::eventFilter(watched, event);
    }

    void TouchFileBrowser::updateGeometryToHost() {
        QWidget *geometryHost = parentWidget();
        if (!geometryHost) {
            auto *mainWindow = resolveMainWindow(this);
            if (mainWindow && mainWindow->getUi()) {
                geometryHost = mainWindow->getUi()->widgetDialogDrawerContentHost;
            }
        }
        if (!geometryHost) {
            return;
        }

        const int availableHeight = geometryHost->height();
        if (availableHeight <= 0) {
            return;
        }

        if (m_geometryHost != geometryHost) {
            if (m_geometryHost) {
                m_geometryHost->removeEventFilter(this);
            }
            m_geometryHost = geometryHost;
            m_geometryHost->installEventFilter(this);
        }

        setMinimumHeight(availableHeight);
        setMaximumHeight(availableHeight);
    }

    void TouchFileBrowser::finishDrawer(const int result) {
        emit finished(result);

        if (auto *mainWindow = resolveMainWindow(this)) {
            mainWindow->finishActiveDrawer(result);
        }
    }

    void TouchFileBrowser::applyComfortChrome() {
        if (m_isApplyingComfortChrome) {
            return;
        }
        QScopedValueRollback<bool> applyingGuard(m_isApplyingComfortChrome, true);

        const DrawerBrowserColors colors = effectiveDrawerBrowserColors(palette());
        const auto applyStyleIfChanged = [](QWidget *widget, const QString &style) {
            if (widget && widget->styleSheet() != style) {
                widget->setStyleSheet(style);
            }
        };

        applyStyleIfChanged(this, QStringLiteral("QWidget#touchFileBrowser { background: %1; }").arg(colors.panelBackground.name()));
        applyStyleIfChanged(m_titleLabel, drawerLabelStyle(colors, 26, 700));
        applyStyleIfChanged(m_selectionLabel, drawerLabelStyle(colors, 18, 600));
        applyStyleIfChanged(m_browserFrame, drawerCardStyle(colors));
        applyStyleIfChanged(m_nameEdit, drawerLineEditStyle(colors));
        applyStyleIfChanged(m_pathEdit, drawerLineEditStyle(colors));
        applyStyleIfChanged(m_backButton, drawerButtonStyle(colors));
        applyStyleIfChanged(m_homeButton, drawerButtonStyle(colors));
        applyStyleIfChanged(m_upButton, drawerButtonStyle(colors));
        applyStyleIfChanged(m_acceptButton, drawerButtonStyle(colors, true));
        applyStyleIfChanged(m_view, drawerTreeStyle(colors) + TouchScrollArea::scrollBarStyleSheet());

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

    void TouchFileBrowser::navigateTo(const QString &path) {
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

    void TouchFileBrowser::handleSelection(const QModelIndex &index, const bool activate) {
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
            finishDrawer(m_mode == Mode::SaveFile ? int(QMessageBox::Save) : int(QMessageBox::Open));
        }
    }

    void TouchFileBrowser::updateSelectionSummary() {
        const QString currentName = fileName();
        if (!m_selectionLabel || m_mode != Mode::SaveFile) {
            return;
        }

        if (currentName.isEmpty()) {
            m_selectionLabel->setText(tr("Choose a destination folder and enter a file name."));
        } else {
            m_selectionLabel->setText(tr("Destination: %1").arg(QDir(m_currentDirectory).filePath(currentName)));
        }
    }

    QString TouchFileBrowser::defaultBrowserDirectory() {
        const QString homePath = QDir::homePath();
        return homePath.isEmpty() ? QDir::currentPath() : homePath;
    }

    QString TouchFileBrowser::preferredSuffix(const QStringList &nameFilters) {
        for (const QString &entry : nameFilters) {
            const int wildcardIndex = entry.indexOf(QStringLiteral("*."));
            if (wildcardIndex < 0) {
                continue;
            }

            int start = wildcardIndex + 2;
            int end = start;
            while (end < entry.size() && entry.at(end).isLetterOrNumber()) {
                ++end;
            }
            if (end > start) {
                return entry.mid(start, end - start);
            }
        }
        return {};
    }
}
