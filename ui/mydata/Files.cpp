//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Files.h" resolved

#include "Files.hpp"
#include "ui_Files.h"

#include <QFileSystemModel>
#include <QClipboard>
#include <QMessageBox>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QHeaderView>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScroller>
#include <QStorageInfo>
#include <QTemporaryDir>
#include <QTreeView>

#include "FairWindSK.hpp"
#include "FileInfoListModel.hpp"
#include "ui/IconUtils.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/widgets/TouchScrollArea.hpp"



#ifdef Q_OS_WIN
	// See https://doc.qt.io/qt-6/qfileinfo.html#details
	extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#else
int qt_ntfs_permission_lookup = 0; //dummy
#endif

namespace fairwindsk::ui::mydata {
    namespace {
        QString normalizedAbsolutePath(const QString &path) {
            if (path.trimmed().isEmpty()) {
                return {};
            }

            const QFileInfo info(path);
            const QString canonicalPath = info.canonicalFilePath();
            if (!canonicalPath.isEmpty()) {
                return QDir::cleanPath(canonicalPath);
            }

            return QDir::cleanPath(info.absoluteFilePath());
        }

        bool isSameOrDescendantPath(const QString &candidatePath, const QString &basePath) {
            const QString normalizedCandidate = normalizedAbsolutePath(candidatePath);
            const QString normalizedBase = normalizedAbsolutePath(basePath);
            if (normalizedCandidate.isEmpty() || normalizedBase.isEmpty()) {
                return false;
            }

            if (normalizedCandidate == normalizedBase) {
                return true;
            }

            return normalizedCandidate.startsWith(normalizedBase + QDir::separator());
        }

        QString touchToolbarButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors, const bool accent = false) {
            const QColor top = accent ? colors.accentTop : colors.buttonBackground.lighter(112);
            const QColor mid = accent ? colors.accentTop.darker(103) : colors.buttonBackground;
            const QColor bottom = accent ? colors.accentBottom : colors.buttonBackground.darker(118);
            const QColor border = accent ? colors.accentBottom : colors.border;
            const QColor checkedTop = colors.accentTop.lighter(accent ? 108 : 102);
            const QColor checkedBottom = colors.accentBottom;
            return QStringLiteral(
                "QPushButton {"
                " min-width: 58px;"
                " max-width: 58px;"
                " min-height: 58px;"
                " max-height: 58px;"
                " padding: 0px;"
                " border-radius: 14px;"
                " border: 1px solid %1;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %2, stop:0.52 %3, stop:1 %4);"
                " }"
                "QPushButton:hover { border-color: %5; }"
                "QPushButton:checked {"
                " border-color: %6;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %7, stop:0.52 %3, stop:1 %8);"
                " }"
                "QPushButton:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %9, stop:0.52 %3, stop:1 %10);"
                " }"
                "QPushButton:disabled {"
                " border-color: %11;"
                " background: %12;"
                " }")
                .arg(border.name(),
                     top.name(),
                     mid.name(),
                     bottom.name(),
                     colors.icon.lighter(115).name(),
                     colors.accentTop.name(),
                     checkedTop.name(),
                     checkedBottom.name(),
                     top.darker(118).name(),
                     bottom.darker(118).name(),
                     colors.border.darker(135).name(),
                     colors.window.darker(105).name());
        }

        QString touchTextButtonStyle(const fairwindsk::ui::ComfortChromeColors &colors, const bool accent = false) {
            const QColor top = accent ? colors.accentTop : colors.buttonBackground.lighter(112);
            const QColor mid = accent ? colors.accentTop.darker(103) : colors.buttonBackground;
            const QColor bottom = accent ? colors.accentBottom : colors.buttonBackground.darker(118);
            const QColor border = accent ? colors.accentBottom : colors.border;
            return QStringLiteral(
                "QPushButton {"
                " min-height: 50px;"
                " padding: 0 16px;"
                " border-radius: 14px;"
                " border: 1px solid %1;"
                " color: %2;"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %3, stop:0.52 %4, stop:1 %5);"
                " }"
                "QPushButton:hover { border-color: %6; }"
                "QPushButton:pressed {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %7, stop:0.52 %4, stop:1 %8);"
                " }"
                "QPushButton:disabled {"
                " border-color: %9;"
                " color: %10;"
                " background: %11;"
                " }")
                .arg(border.name(),
                     colors.text.name(),
                     top.name(),
                     mid.name(),
                     bottom.name(),
                     colors.icon.lighter(115).name(),
                     top.darker(118).name(),
                     bottom.darker(118).name(),
                     colors.border.darker(135).name(),
                     colors.text.darker(150).name(),
                     colors.window.darker(105).name());
        }

        QString touchLineEditStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &baseColor) {
            return QStringLiteral(
                "QLineEdit {"
                " min-height: 50px;"
                " border: 1px solid %1;"
                " border-radius: 12px;"
                " padding: 4px 12px;"
                " background: %2;"
                " color: %3;"
                " font-size: 18px;"
                " }"
                "QLineEdit:focus { border-color: %4; }")
                .arg(colors.border.name(), baseColor.name(), colors.text.name(), colors.accentTop.name());
        }

        QString touchBrowserTreeStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &baseColor, const QColor &panelColor) {
            return QStringLiteral(
                "QTreeView {"
                " background: %1;"
                " color: %2;"
                " border: 1px solid %3;"
                " border-radius: 14px;"
                " outline: none;"
                " font-size: 18px;"
                " alternate-background-color: %4;"
                " show-decoration-selected: 1;"
                " }"
                "QTreeView::item {"
                " min-height: 58px;"
                " padding: 8px 12px;"
                " border-radius: 10px;"
                " }"
                "QTreeView::item:selected {"
                " background: %5;"
                " color: %6;"
                " }"
                "QHeaderView::section {"
                " min-height: 46px;"
                " padding: 0 12px;"
                " background: %7;"
                " color: %2;"
                " border: none;"
                " border-bottom: 1px solid %3;"
                " font-size: 16px;"
                " font-weight: 700;"
                " }")
                .arg(baseColor.name(),
                     colors.text.name(),
                     colors.border.name(),
                     panelColor.name(),
                     colors.accentTop.name(),
                     colors.accentText.name(),
                     panelColor.darker(104).name());
        }

        QString touchTableStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &baseColor, const QColor &panelColor) {
            return QStringLiteral(
                "QTableView {"
                " background: %1;"
                " color: %2;"
                " alternate-background-color: %3;"
                " border: 1px solid %4;"
                " border-radius: 14px;"
                " gridline-color: transparent;"
                " outline: none;"
                " font-size: 18px;"
                " }"
                "QTableView::item { padding: 10px; }"
                "QTableView::item:selected { background: %5; color: %6; }"
                "QHeaderView::section {"
                " min-height: 46px;"
                " padding: 0 12px;"
                " background: %7;"
                " color: %8;"
                " border: none;"
                " border-bottom: 1px solid %4;"
                " font-size: 16px;"
                " font-weight: 700;"
                " }")
                .arg(baseColor.name(),
                     colors.text.name(),
                     panelColor.name(),
                     colors.border.name(),
                     colors.accentTop.name(),
                     colors.accentText.name(),
                     panelColor.darker(104).name(),
                     colors.text.name());
        }

        QString touchInfoStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &panelColor) {
            return QStringLiteral(
                "QGroupBox {"
                " margin-top: 12px;"
                " padding-top: 18px;"
                " border: 1px solid %1;"
                " border-radius: 14px;"
                " background: %2;"
                " color: %3;"
                " font-size: 16px;"
                " font-weight: 700;"
                " }"
                "QGroupBox::title {"
                " subcontrol-origin: margin;"
                " left: 14px;"
                " padding: 0 6px;"
                " }"
                "QLabel {"
                " color: %3;"
                " font-size: 16px;"
                " }")
                .arg(colors.border.name(), panelColor.name(), colors.text.name());
        }

        QString toolRowStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &panelColor) {
            return QStringLiteral(
                "QFrame {"
                " background: %1;"
                " border: 1px solid %2;"
                " border-radius: 14px;"
                " }")
                .arg(panelColor.name(), colors.border.name());
        }

        QString statusPanelStyle(const fairwindsk::ui::ComfortChromeColors &colors, const QColor &panelColor) {
            return QStringLiteral(
                "QWidget {"
                " background: %1;"
                " border: 1px solid %2;"
                " border-radius: 14px;"
                " }"
                "QLabel {"
                " color: %3;"
                " font-size: 16px;"
                " font-weight: 600;"
                " padding: 0 12px;"
                " }")
                .arg(panelColor.name(), colors.border.name(), colors.text.name());
        }
	    }

    bool Files::wouldPasteDirectoryIntoOwnTree(const QString &sourcePath, const QString &destinationPath) {
        return isSameOrDescendantPath(destinationPath, sourcePath);
    }

    bool Files::runDirectorySafetySelfTest(QString *failureReason) {
        auto fail = [failureReason](const QString &message) {
            if (failureReason) {
                *failureReason = message;
            }
            return false;
        };

        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            return fail(QStringLiteral("Unable to create a temporary directory for the directory safety self-test"));
        }

        QDir root(tempDir.path());
        const QString sourcePath = root.filePath(QStringLiteral("source"));
        const QString childPath = QDir(sourcePath).filePath(QStringLiteral("nested"));
        const QString siblingDestination = root.filePath(QStringLiteral("destination"));

        if (!root.mkpath(QStringLiteral("source/nested")) || !root.mkpath(QStringLiteral("destination"))) {
            return fail(QStringLiteral("Unable to create the directory safety self-test fixtures"));
        }

        if (!wouldPasteDirectoryIntoOwnTree(sourcePath, sourcePath)) {
            return fail(QStringLiteral("Self-paste detection failed for identical source and destination directories"));
        }

        if (!wouldPasteDirectoryIntoOwnTree(sourcePath, childPath)) {
            return fail(QStringLiteral("Self-paste detection failed for a descendant destination directory"));
        }

        if (wouldPasteDirectoryIntoOwnTree(sourcePath, siblingDestination)) {
            return fail(QStringLiteral("Self-paste detection incorrectly rejected a sibling destination directory"));
        }

        QFile sampleFile(QDir(sourcePath).filePath(QStringLiteral("sample.txt")));
        if (!sampleFile.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
            sampleFile.write("payload") != 7) {
            return fail(QStringLiteral("Unable to create the directory copy self-test fixture"));
        }
        sampleFile.close();

        QString errorMessage;
        if (!copyOrMoveDirectorySubtree(sourcePath, siblingDestination, false, false, &errorMessage)) {
            return fail(QStringLiteral("Copying to a sibling directory failed unexpectedly: %1").arg(errorMessage));
        }

        const QString copiedFilePath = QDir(siblingDestination).filePath(QStringLiteral("source/sample.txt"));
        if (!QFileInfo::exists(copiedFilePath)) {
            return fail(QStringLiteral("The copied directory subtree did not contain the expected file"));
        }

        if (failureReason) {
            failureReason->clear();
        }
        return true;
    }

	Files::Files(QWidget *parent) : QWidget(parent), ui(new Ui::Files) {
	    ui->setupUi(this);

		ui->tableView_Search->hide();
		ui->listView_Files->show();
		ui->widget_Searching->hide();
		ui->groupBox_ItemInfo->hide();
        ui->groupBox_ItemInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		m_currentDir = nullptr;



		m_fileSystemModel = new QFileSystemModel(this);
        m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
	    m_fileSystemModel->setRootPath(QDir::homePath());

		ui->listView_Files->setModel(m_fileSystemModel);
        ui->listView_Files->setSelectionMode(QAbstractItemView::ExtendedSelection);
        ui->listView_Files->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->listView_Files->setRootIsDecorated(false);
        ui->listView_Files->setItemsExpandable(false);
        ui->listView_Files->setUniformRowHeights(true);
        ui->listView_Files->setIndentation(0);
        ui->listView_Files->setAllColumnsShowFocus(true);
        ui->listView_Files->setAlternatingRowColors(true);
        ui->listView_Files->setSortingEnabled(true);
        ui->listView_Files->setIconSize(QSize(36, 36));
        ui->listView_Files->header()->setMinimumHeight(48);
        ui->listView_Files->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->listView_Files->header()->setStretchLastSection(true);
        ui->listView_Files->header()->setSortIndicatorShown(false);
        ui->listView_Files->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        ui->listView_Files->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        ui->listView_Files->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        ui->listView_Files->sortByColumn(0, Qt::AscendingOrder);
        ui->listView_Files->hideColumn(2);
        ui->tableView_Search->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView_Search->setSelectionMode(QAbstractItemView::ExtendedSelection);
        ui->tableView_Search->horizontalHeader()->setStretchLastSection(true);
        ui->tableView_Search->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->tableView_Search->verticalHeader()->setVisible(false);

		const auto index = m_fileSystemModel->index(QDir::currentPath());

		ui->listView_Files->setRootIndex(index);

		connect(ui->listView_Files, &QAbstractItemView::doubleClicked, this, &Files::onFileViewItemDoubleClicked);
		connect(ui->listView_Files, &QAbstractItemView::clicked, this, &Files::onFileViewItemClicked);

		connect(ui->toolButton_Up, &QPushButton::clicked, this, &Files::onUpClicked);
		connect(ui->lineEdit_Path, &QLineEdit::returnPressed, this, &Files::onPathReturnPressed);

		connect(ui->toolButton_Filters, &QPushButton::clicked, this, &Files::onFiltersClicked);
		connect(ui->toolButton_Search, &QPushButton::clicked, this, &Files::onSearchClicked);
		connect(ui->lineEdit_Search,&QLineEdit::returnPressed, this, &Files::onSearchClicked);

		connect(ui->toolButton_Cut, &QPushButton::clicked, this, &Files::onCutClicked);
		connect(ui->toolButton_Copy, &QPushButton::clicked, this, &Files::onCopyClicked);
		connect(ui->toolButton_Paste, &QPushButton::clicked, this, &Files::onPasteClicked);

		connect(ui->toolButton_Open, &QPushButton::clicked, this, &Files::onOpenClicked);

		connect(ui->toolButton_NewFolder, &QPushButton::clicked, this, &Files::onNewFolderClicked);

		connect(ui->toolButton_Delete, &QPushButton::clicked, this, &Files::onDeleteClicked);
		connect(ui->toolButton_Rename, &QPushButton::clicked, this, &Files::onRenameClicked);

		connect(ui->toolButton_Home, &QPushButton::clicked, this, &Files::onHome);

        m_searchOptionsButton = new QPushButton(tr("Options"), ui->frame_FilesSearchRow);
        m_searchOptionsButton->setObjectName(QStringLiteral("toolButton_SearchOptions"));
        m_searchOptionsButton->setCheckable(true);
        ui->horizontalLayout_SearchRow->insertWidget(5, m_searchOptionsButton);
        connect(m_searchOptionsButton, &QPushButton::toggled, this, [this](const bool visible) {
            ui->toolButton_CaseSensitive->setVisible(visible);
            ui->toolButton_SearchHidden->setVisible(visible);
            ui->toolButton_SearchSystem->setVisible(visible);
            updateSearchOptionsButton();
        });
        connect(ui->toolButton_CaseSensitive, &QPushButton::toggled, this, &Files::updateSearchOptionsButton);
        connect(ui->toolButton_SearchHidden, &QPushButton::toggled, this, &Files::updateSearchOptionsButton);
        connect(ui->toolButton_SearchSystem, &QPushButton::toggled, this, &Files::updateSearchOptionsButton);
        ui->toolButton_CaseSensitive->hide();
        ui->toolButton_SearchHidden->hide();
        ui->toolButton_SearchSystem->hide();

        m_actionsRow = new QFrame(ui->group_ToolBar);
        m_actionsRow->setObjectName(QStringLiteral("frame_FilesActionsRow"));
        m_actionsLayout = new QGridLayout(m_actionsRow);
        m_actionsLayout->setContentsMargins(8, 6, 8, 6);
        m_actionsLayout->setHorizontalSpacing(4);
        m_actionsLayout->setVerticalSpacing(6);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_NewFolder);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_Rename);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_Delete);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_Cut);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_Copy);
        ui->horizontalLayout_SearchRow->removeWidget(ui->toolButton_Paste);
        ui->verticalLayout_Toolbar->insertWidget(2, m_actionsRow);

        auto *clipboardStatusRow = new QWidget(ui->group_ToolBar);
        clipboardStatusRow->setObjectName(QStringLiteral("widget_ClipboardStatus"));
        auto *clipboardStatusLayout = new QHBoxLayout(clipboardStatusRow);
        clipboardStatusLayout->setContentsMargins(0, 0, 0, 0);
        clipboardStatusLayout->setSpacing(8);
        m_clipboardStatusLabel = new QLabel(clipboardStatusRow);
        m_clipboardStatusLabel->setObjectName(QStringLiteral("label_ClipboardStatus"));
        m_clipboardStatusLabel->setWordWrap(true);
        m_clearClipboardButton = new QPushButton(tr("Clear"), clipboardStatusRow);
        m_clearClipboardButton->setObjectName(QStringLiteral("pushButton_ClearClipboard"));
        clipboardStatusLayout->addWidget(m_clipboardStatusLabel, 1);
        clipboardStatusLayout->addWidget(m_clearClipboardButton, 0, Qt::AlignRight);
        ui->verticalLayout_Toolbar->insertWidget(3, clipboardStatusRow);
        connect(m_clearClipboardButton, &QPushButton::clicked, this, &Files::clearClipboardState);

		m_fileListModel = new FileInfoListModel(this);
		ui->tableView_Search->setModel(m_fileListModel);
        ui->lineEdit_Path->setPlaceholderText(tr("Enter a path"));
        ui->lineEdit_Search->setPlaceholderText(tr("Search current folder"));
        ui->toolButton_Open->setToolTip(tr("Open selected item"));
        ui->toolButton_NewFolder->setToolTip(tr("Create folder"));
        ui->toolButton_Delete->setToolTip(tr("Delete selected items"));
        ui->toolButton_Rename->setToolTip(tr("Rename selected item"));
        ui->toolButton_Copy->setToolTip(tr("Copy selection"));
        ui->toolButton_Cut->setToolTip(tr("Cut selection"));
        ui->toolButton_Paste->setToolTip(tr("Paste into current folder"));
        ui->toolButton_Search->setToolTip(tr("Search in current folder"));
        ui->toolButton_Filters->setToolTip(tr("Clear search results"));
        if (m_searchOptionsButton) {
            m_searchOptionsButton->setToolTip(tr("Search options"));
        }

		connect(&m_searchingWatcher, &QFutureWatcher<QFileInfo>::finished, this, &Files::searchFinished);
		connect(&m_searchingWatcher, &QFutureWatcher<QFileInfo>::progressValueChanged, this, &Files::searchProgressValueChanged);

        connect(ui->tableView_Search, &QAbstractItemView::doubleClicked, this, &Files::onSearchViewItemDoubleClicked);
		connect(ui->tableView_Search, &QAbstractItemView::clicked, this, &Files::onSearchViewItemClicked);
        connect(ui->listView_Files->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            updateActionStates();
        });
        connect(ui->tableView_Search->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            updateActionStates();
        });

	    ui->listView_Files->setAttribute(Qt::WA_AcceptTouchEvents,true);
		ui->tableView_Search->setAttribute(Qt::WA_AcceptTouchEvents,true);
        QScroller::grabGesture(ui->listView_Files->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(ui->listView_Files->viewport(), QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(ui->tableView_Search->viewport(), QScroller::TouchGesture);
        QScroller::grabGesture(ui->tableView_Search->viewport(), QScroller::LeftMouseButtonGesture);

        configureTouchFriendlyUi();
        retintToolButtons();
        applyComfortChrome();
        clearItemInfo();
        updateClipboardStatus();
        updateSearchOptionsButton();
        updateActionRowLayout();
        updateActionStates();
        updateTitleLabel();
		
		onHome();
	}

    void Files::changeEvent(QEvent *event) {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            retintToolButtons();
            applyComfortChrome();
            updateSearchOptionsButton();
            updateActionRowLayout();
            updateActionStates();
        }
    }

    void Files::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        updateActionRowLayout();
    }

	    void Files::configureTouchFriendlyUi() {
        const auto configureIconButton = [](QPushButton *button, const QString &toolTip, const bool accent = false) {
            if (!button) {
                return;
            }
            button->setText(QString());
            button->setToolTip(toolTip);
            button->setFlat(false);
            button->setMinimumSize(accent ? QSize(62, 58) : QSize(58, 58));
            button->setIconSize(QSize(28, 28));
        };

        configureIconButton(ui->toolButton_Home, tr("Home"));
        configureIconButton(ui->toolButton_Up, tr("Up"));
        configureIconButton(ui->toolButton_Filters, tr("Clear search"));
        configureIconButton(ui->toolButton_Search, tr("Search in current folder"), true);
        configureIconButton(ui->toolButton_Cut, tr("Cut selection"));
        configureIconButton(ui->toolButton_Copy, tr("Copy selection"));
	        configureIconButton(ui->toolButton_Paste, tr("Paste into current folder"));
        configureIconButton(ui->toolButton_Rename, tr("Rename selected item"));
        configureIconButton(ui->toolButton_Delete, tr("Delete selected items"));
        configureIconButton(ui->toolButton_NewFolder, tr("Create folder"), true);
        configureIconButton(ui->toolButton_Open, tr("Open selected item"), true);
        configureIconButton(m_searchOptionsButton, tr("Search options"));

        ui->labelTitle->setTextFormat(Qt::RichText);
        ui->labelTitle->setWordWrap(true);
        ui->lineEdit_Search->setClearButtonEnabled(true);
        ui->lineEdit_Path->setClearButtonEnabled(true);
        ui->lineEdit_Search->setMinimumHeight(58);
        ui->lineEdit_Path->setMinimumHeight(0);
        ui->lineEdit_Path->setMaximumHeight(0);

        ui->listView_Files->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        ui->listView_Files->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        ui->listView_Files->setTextElideMode(Qt::ElideMiddle);

        ui->tableView_Search->verticalHeader()->setDefaultSectionSize(58);
        ui->tableView_Search->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        ui->tableView_Search->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

	        ui->progressBar_Searching->setMinimumHeight(16);
	        ui->label_Searching->setMinimumHeight(44);
	        ui->groupBox_ItemInfo->setMinimumHeight(88);
	        ui->groupBox_ItemInfo->setTitle(tr("Selection"));
            if (m_clearClipboardButton) {
                m_clearClipboardButton->setMinimumHeight(50);
                m_clearClipboardButton->setMinimumWidth(96);
            }
            if (m_clipboardStatusLabel) {
                m_clipboardStatusLabel->setMinimumHeight(50);
            }
	        ui->verticalLayout_Main->setStretch(0, 1);
        ui->verticalLayout_Main->setStretch(1, 1);
        ui->verticalLayout_Main->setStretch(2, 0);
        ui->verticalLayout_Main->setStretch(3, 0);

        ui->toolButton_CaseSensitive->setIcon(QIcon(":/resources/svg/OpenBridge/sort-google.svg"));
        ui->toolButton_SearchHidden->setIcon(QIcon(":/resources/svg/OpenBridge/show-page-details.svg"));
        ui->toolButton_SearchSystem->setIcon(QIcon(":/resources/svg/OpenBridge/database.svg"));
        if (m_searchOptionsButton) {
            m_searchOptionsButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/configure.svg")));
        }
    }

    void Files::applyComfortChrome() {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QColor panelColor = fairwindsk::ui::comfortThemeColor(configuration, preset, QStringLiteral("panel"), palette().color(QPalette::AlternateBase));
        const QColor baseColor = fairwindsk::ui::comfortThemeColor(configuration, preset, QStringLiteral("base"), palette().color(QPalette::Base));

        const auto applyStyleIfChanged = [](QWidget *widget, const QString &style) {
            if (widget && widget->styleSheet() != style) {
                widget->setStyleSheet(style);
            }
        };

        applyStyleIfChanged(this, QStringLiteral("QWidget { background: %1; }").arg(chrome.window.name()));
        applyStyleIfChanged(ui->labelTitle, QStringLiteral("color: %1; font-size: 26px; font-weight: 700;")
            .arg(chrome.text.name()));
        applyStyleIfChanged(ui->lineEdit_Path, touchLineEditStyle(chrome, baseColor));
        applyStyleIfChanged(ui->lineEdit_Search, touchLineEditStyle(chrome, baseColor));
        for (auto *button : findChildren<QPushButton *>()) {
            const bool accent = button == ui->toolButton_Search || button == ui->toolButton_Open;
            applyStyleIfChanged(button, touchToolbarButtonStyle(chrome, accent));
        }
	        for (auto *rowFrame : ui->group_ToolBar->findChildren<QFrame *>()) {
	            if (rowFrame->objectName().startsWith(QStringLiteral("frame_Files"))) {
	                applyStyleIfChanged(rowFrame, toolRowStyle(chrome, panelColor));
	            }
	        }
            if (auto *statusWidget = ui->group_ToolBar->findChild<QWidget *>(QStringLiteral("widget_ClipboardStatus"))) {
                applyStyleIfChanged(statusWidget, statusPanelStyle(chrome, panelColor));
            }
            if (m_clearClipboardButton) {
                applyStyleIfChanged(m_clearClipboardButton, touchTextButtonStyle(chrome));
                m_clearClipboardButton->setText(tr("Clear"));
                m_clearClipboardButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/close-google.svg")));
                m_clearClipboardButton->setIconSize(QSize(22, 22));
            }
            if (m_searchOptionsButton) {
                applyStyleIfChanged(m_searchOptionsButton, touchTextButtonStyle(chrome));
                m_searchOptionsButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/configure.svg")));
                m_searchOptionsButton->setIconSize(QSize(22, 22));
            }
	        applyStyleIfChanged(ui->listView_Files, touchBrowserTreeStyle(chrome, baseColor, panelColor) + fairwindsk::ui::widgets::TouchScrollArea::scrollBarStyleSheet());
        applyStyleIfChanged(ui->tableView_Search, touchTableStyle(chrome, baseColor, panelColor) + fairwindsk::ui::widgets::TouchScrollArea::scrollBarStyleSheet());
        applyStyleIfChanged(ui->groupBox_ItemInfo, touchInfoStyle(chrome, panelColor));
        applyStyleIfChanged(ui->widget_Searching, QStringLiteral("QWidget { background: %1; border: 1px solid %2; border-radius: 12px; } QLabel { color: %3; font-size: 17px; font-weight: 600; }").arg(panelColor.name(), chrome.border.name(), chrome.text.name()));
    }

    void Files::retintToolButtons() const {
        auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
        auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("day");
        const QColor fallbackIconColor = fairwindsk::ui::bestContrastingColor(
            palette().color(QPalette::Button),
            {palette().color(QPalette::Text),
             palette().color(QPalette::ButtonText),
             palette().color(QPalette::WindowText)});
        const QColor buttonIconColor = fairwindsk::ui::comfortIconColor(configuration, preset, fallbackIconColor);
        for (auto *button : findChildren<QPushButton *>()) {
            fairwindsk::ui::applyTintedButtonIcon(button, buttonIconColor, QSize(32, 32));
        }
    }



	QString Files::getCurrentDir() const {
		if (!m_currentDir) {
			return {};
		}

		return m_currentDir->absolutePath();
	}

	void Files::setCurrentDir(const QString& path) {
        const QFileInfo pathInfo(path);
        const QString absolutePath = pathInfo.exists() && pathInfo.isDir()
            ? pathInfo.absoluteFilePath()
            : QDir(path).absolutePath();
        const QDir targetDir(absolutePath);
        if (!targetDir.exists()) {
            showWarning(tr("The selected directory does not exist."));
            return;
        }

		if (m_currentDir) {
			delete m_currentDir;
			m_currentDir = nullptr;
		}

		m_currentDir = new QDir(absolutePath);

		ui->lineEdit_Path->setText(m_currentDir->absolutePath());
		ui->lineEdit_Path->update();
        updateTitleLabel();

		const auto index = m_fileSystemModel->index(m_currentDir->absolutePath());


		ui->listView_Files->setRootIndex(index);
        ui->listView_Files->clearSelection();
        ui->tableView_Search->clearSelection();
        m_currentFilePath.clear();
        clearItemInfo();
        updateActionStates();
	}



	/***********************
	 * Open the selected
	 ***********************/
	void Files::onOpenClicked() {
        const QString path = selectedOrCurrentPath();
        if (path.isEmpty()) {
            showWarning(tr("Select a file or folder first."));
            return;
        }

        const QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            viewFile(path);
        } else if (fileInfo.isDir()) {
            setCurrentDir(path);
            setSearchResultsVisible(false);
            ui->widget_Searching->hide();
        }
	}

	bool Files::setCurrentFilePath(const QString& path) {
        m_currentFilePath = path;
        updateItemInfo(path);
        updateActionStates();
        return QFileInfo(path).isFile();
	}

	void Files::onFileViewItemClicked(const QModelIndex &index)  {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileSystemModel->filePath(index); !setCurrentFilePath(path)) {
            updateItemInfo(path);
		}
	}

	void Files::onFileViewItemDoubleClicked(const QModelIndex &index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileSystemModel->filePath(index); setCurrentFilePath(path)) {

			// View the file
			viewFile(path);

		} else {

			// Select the current path
			setCurrentDir(path);

		}

	}

	void Files::onSearchViewItemClicked(const QModelIndex& index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path =  m_fileListModel->getAbsolutePath(index); !setCurrentFilePath(path)) {

			// Reset the current file path
			m_currentFilePath = "";
            clearItemInfo();
            updateActionStates();
		}


	}

	void Files::onSearchViewItemDoubleClicked(const QModelIndex& index) {

		// Get the selected file, set is as current file path if it is a file
		if (const auto path = m_fileListModel->getAbsolutePath(index); setCurrentFilePath(path)) {

			// View the file
			viewFile(path);
        } else if (!path.isEmpty()) {
            setCurrentDir(path);
            setSearchResultsVisible(false);
            ui->widget_Searching->hide();
            ui->listView_Files->show();
		}

	}



	void Files::viewFile(const QString& path) {
		if (m_fileViewer) {
			onFileViewerCloseClicked();
		}

		qDebug() << "Files::viewFile " << path;

		const QMimeDatabase db;
		const auto type = db.mimeTypeForFile(path);
		qDebug() << "Mime type:" << type.name();

		ui->group_ToolBar->hide();
		ui->group_Main->hide();
		m_fileViewer = new FileViewer(path, this);
		connect(m_fileViewer, &FileViewer::askedToBeClosed, this, &Files::onFileViewerCloseClicked);
		ui->group_Content->layout()->addWidget(m_fileViewer);
        const QString fileLabel = QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName();
        ui->labelTitle->setText(QStringLiteral(
                                    "<html><head/><body>"
                                    "<p style=\"margin:0;\">"
                                    "<span style=\"font-size:26px; font-weight:700;\">%1</span><br/>"
                                    "<span style=\"font-size:16px; font-weight:500;\">%2</span>"
                                    "</p></body></html>")
                                    .arg(tr("Files"), fileLabel.toHtmlEscaped()));
        updateActionStates();
	}



	void Files::onFileViewerCloseClicked() {
		if (m_fileViewer) {
			m_fileViewer->close();
			delete m_fileViewer;
			m_fileViewer = nullptr;

			ui->group_ToolBar->show();
			ui->group_Main->show();
            updateTitleLabel();
            updateActionStates();
		}
	}



	QStringList Files::getSelection() const {
        const auto *selectionModel = activeSelectionModel();
        if (!selectionModel) {
            return {};
        }
		QModelIndexList list = selectionModel->selectedRows();
        if (list.isEmpty()) {
            list = selectionModel->selectedIndexes();
        }
		QStringList path_list;
		foreach (const QModelIndex &index, list) {
            if (ui->tableView_Search->isVisible()) {
                const QString path = m_fileListModel->getAbsolutePath(index);
                if (!path.isEmpty()) {
                    path_list.append(path);
                }
            } else {
			    path_list.append(m_fileSystemModel->filePath(index));
            }
		}
        path_list.removeDuplicates();
        return path_list;
}



	void Files::onFiltersClicked() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		ui->lineEdit_Search->clear();
		setSearchResultsVisible(false);
		ui->widget_Searching->hide();
		ui->listView_Files->show();
        updateActionStates();
	}

	void Files::onSearchClicked() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		if (const auto key = ui->lineEdit_Search->text(); key.isEmpty()) {
			setSearchResultsVisible(false);
			ui->listView_Files->show();
            updateActionStates();
		} else {

			ui->listView_Files->hide();
			setSearchResultsVisible(false);
			ui->label_Searching->setText(tr("Searching..."));
			ui->progressBar_Searching->setValue(0);
			ui->widget_Searching->show();

			auto searchPath = getCurrentDir();

			const Qt::CaseSensitivity caseSensitivity = ui->toolButton_CaseSensitive->isChecked() ? Qt::CaseSensitive:Qt::CaseInsensitive;


			QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;

			if (ui->toolButton_SearchHidden->isChecked()) {
				filters |= QDir::Hidden;
			}
			if (ui->toolButton_SearchSystem->isChecked())
				filters |= QDir::System;

			const auto future = QtConcurrent::run(Files::search, searchPath, key, caseSensitivity,filters);
			m_searchingWatcher.setFuture(future);

		}
	}

	void Files::search(QPromise<QFileInfo> &promise, const QString &searchPath, const QString &key, const Qt::CaseSensitivity caseSensitivity, const QDir::Filters filters)
	{
		qDebug() << "Files::search" << searchPath << " " << key;

		QDirIterator dirItems(searchPath, filters, QDirIterator::Subdirectories);
		int count=0;
		while (dirItems.hasNext()) {

			promise.suspendIfRequested();

			if (promise.isCanceled())
				return;

			if (const QFileInfo fileInfo = dirItems.nextFileInfo(); fileInfo.fileName().contains(key, caseSensitivity)) {
				promise.addResult(fileInfo);
			}

			count++;
			if (count % 1000 == 0) {

				promise.setProgressValue(count);
			}
		}
	}

	void Files::searchProgressValueChanged(const int progress) {
		ui->progressBar_Searching->setValue(std::min(progress, ui->progressBar_Searching->maximum()));
        ui->label_Searching->setText(tr("Searching... %1 item(s) scanned").arg(progress));
	}

	void Files::searchFinished() {
		if (const QList<QFileInfo> results = m_searchingWatcher.future().results<QFileInfo>(); !results.isEmpty()) {

			m_fileListModel->setQFileInfoList(results);
			ui->tableView_Search->resizeColumnsToContents();
			ui->widget_Searching->hide();
			setSearchResultsVisible(true);
            ui->label_Searching->setText(tr("%1 result(s)").arg(results.size()));
		} else {
			ui->label_Searching->setText(tr("Not found!"));
			ui->progressBar_Searching->setValue(100);
			setSearchResultsVisible(false);
		}
        updateActionStates();
	}

	void Files::setSearchResultsVisible(const bool visible) {
		ui->tableView_Search->setVisible(visible);
        ui->listView_Files->setVisible(!visible);
		if (!visible) {
			m_fileListModel->setQFileInfoList({});
		}
	}





	void Files::onHome() {
		setCurrentDir(QDir::homePath());
	}

	void Files::onPathReturnPressed() {
		setCurrentDir(ui->lineEdit_Path->text());
	}


	void Files::showWarning(const QString &message) const {
		drawer::warning(const_cast<Files *>(this), tr("Files"), message);
	}

	void Files::onUpClicked() {
		if (QDir dir(getCurrentDir()); dir.cdUp()) {
			setCurrentDir(dir.absolutePath());
		}
	}

	void Files::onNewFolderClicked() {
		bool ok = false;
		const QString folderName = drawer::getText(this,
		                               tr("Create a new folder"),
		                               tr("Enter folder name:"),
		                               tr("New folder"),
		                               &ok);

		if (ok && !folderName.isEmpty()) {
            const QString absolutePath = QDir(currentDestinationDir()).filePath(folderName);
			QDir dir;
			if (!dir.exists(absolutePath)) {
				if (!dir.mkpath(absolutePath)) {
                    showWarning(tr("Unable to create the selected folder."));
                }
			} else {
                showWarning(tr("A file or folder with the same name already exists."));
			}
            refreshCurrentView();
            setCurrentFilePath(absolutePath);
		}
	}

	void Files::onDeleteClicked()
	{

			QStringList paths_to_delete = getSelection();
			QMessageBox::StandardButton choice
				= drawer::warning(this,
				                  tr("Confirm delete"),
				                  tr("Are you sure you want to delete the selected items?"),
				                  QMessageBox::Ok | QMessageBox::Cancel,
				                  QMessageBox::Cancel);
			if (choice == QMessageBox::Ok) {
                    QStringList issues;
                    int deletedCount = 0;
					foreach (QString path, paths_to_delete) {
						if (QFileInfo fileInfo(path); fileInfo.isDir()) {
							QDir dir(path);
							if (!dir.removeRecursively()) {
								issues.append(tr("Could not delete folder %1.").arg(QDir::toNativeSeparators(path)));
                            } else {
                                ++deletedCount;
							}
						} else {
							if (QFile file(path); !file.remove()) {
								issues.append(tr("Could not delete file %1.").arg(QDir::toNativeSeparators(path)));
                            } else {
                                ++deletedCount;
							}
						}
					}
	                refreshCurrentView();
	                clearItemInfo();
	                updateActionStates();
                    const QString summary = issues.isEmpty()
                            ? tr("Removed %1 item(s).").arg(deletedCount)
                            : tr("Removed %1 of %2 item(s).").arg(deletedCount).arg(paths_to_delete.size());
                    showOperationSummary(tr("Delete completed"), summary, issues);
				}

	}

	void Files::onRenameClicked() {

			QStringList selectedItems =getSelection();
			foreach (QString selectedItem, selectedItems) {
				if (!selectedItem.isEmpty()) {
					bool ok = false;
					QFileInfo fileInfo(selectedItem);
					QString newName = drawer::getText(this,
					                                  tr("Rename"),
					                                  tr("New name:"),
					                                  fileInfo.fileName(),
					                                  &ok);
					if (ok && !newName.isEmpty()) {
                        const QString renamedPath = fileInfo.dir().filePath(newName);
                        if (QFileInfo::exists(renamedPath)) {
                            showWarning(tr("A file or folder with the same name already exists."));
                            continue;
                        }
                        bool renamed = false;
                        if (fileInfo.isDir()) {
                            renamed = fileInfo.dir().rename(fileInfo.fileName(), newName);
                        } else {
						    renamed = QFile::rename(selectedItem, renamedPath);
                        }
						if (!renamed) {
                            showWarning(tr("Unable to rename the selected item."));
                        } else {
                            m_currentFilePath = renamedPath;
						}
					}
				}
			}
            refreshCurrentView();
            if (!m_currentFilePath.isEmpty()) {
                setCurrentFilePath(m_currentFilePath);
            }

	}

		void Files::onCutClicked() {

				m_itemsToCopy.clear();
				m_itemsToMove = getSelection();
                updateClipboardStatus();
                updateActionStates();

		}

		void Files::onCopyClicked() {

				m_itemsToMove.clear();
				m_itemsToCopy = getSelection();
                updateClipboardStatus();
                updateActionStates();

		}

		void Files::onPasteClicked() {
	            const QString destinationDir = currentDestinationDir();
                QStringList issues;
                int copiedCount = 0;
                int movedCount = 0;
					foreach (QString path, m_itemsToCopy) {
						QFileInfo item(path);
						if (item.isFile()) {
							QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
	                    if (QFileInfo::exists(newPath)) {
	                        issues.append(tr("Skipped %1 because it already exists in the destination.").arg(item.fileName()));
	                        continue;
	                    }
							if (!QFile::copy(path, newPath)) {
		                        issues.append(tr("Unable to copy %1.").arg(item.fileName()));
                            } else {
                                ++copiedCount;
		                    }
						} else {
	                        const QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
	                        if (wouldPasteDirectoryIntoOwnTree(path, newPath)) {
	                            issues.append(tr("Unable to copy %1 into itself or one of its subfolders.").arg(item.fileName()));
	                            continue;
	                        }
	                        if (QFileInfo::exists(newPath)) {
	                            issues.append(tr("Skipped %1 because it already exists in the destination.").arg(item.fileName()));
	                            continue;
	                        }
	                        QString errorMessage;
							if (!copyOrMoveDirectorySubtree(path, destinationDir, false, false, &errorMessage)) {
                                issues.append(errorMessage.isEmpty()
	                                            ? tr("Unable to copy %1.").arg(item.fileName())
	                                            : errorMessage);
                            } else {
                                ++copiedCount;
	                        }
						}
					}
					foreach (QString path, m_itemsToMove) {
						QFileInfo item(path);
					if (item.isFile()) {
						QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
	                    if (QFileInfo::exists(newPath)) {
	                        issues.append(tr("Skipped %1 because it already exists in the destination.").arg(item.fileName()));
	                        continue;
	                    }
	                    bool moved = QFile::rename(path, newPath);
	                    if (!moved) {
						    moved = QFile::copy(path, newPath) && QFile::remove(path);
	                    }
							if (!moved) {
		                        issues.append(tr("Unable to move %1.").arg(item.fileName()));
                            } else {
                                ++movedCount;
		                    }
						} else {
	                        const QString newPath = QDir(destinationDir).absoluteFilePath(item.fileName());
	                        if (wouldPasteDirectoryIntoOwnTree(path, newPath)) {
	                            issues.append(tr("Unable to move %1 into itself or one of its subfolders.").arg(item.fileName()));
	                            continue;
	                        }
	                        if (QFileInfo::exists(newPath)) {
	                            issues.append(tr("Skipped %1 because it already exists in the destination.").arg(item.fileName()));
	                            continue;
	                        }
	                        QString errorMessage;
							if (!copyOrMoveDirectorySubtree(path, destinationDir, true, false, &errorMessage)) {
                                issues.append(errorMessage.isEmpty()
	                                            ? tr("Unable to move %1.").arg(item.fileName())
	                                            : errorMessage);
                            } else {
                                ++movedCount;
	                        }
						}
					}
                const QString summary = tr("Copied %1 item(s) and moved %2 item(s).")
                        .arg(copiedCount)
                        .arg(movedCount);
                clearClipboardState();
	            refreshCurrentView();
	            updateActionStates();
                showOperationSummary(tr("Paste completed"), summary, issues);

		}

    QString Files::format_bytes(qint64 bytes)
    {
	    qint64 higher = bytes;
        int index = 0;
        const QString units[] = {"B", "KB", "MB", "GB", "TB"};
        while (higher > 1000) {
            ++index;
            higher /= 1000;
        }
        QString result = QString::number(higher) + " " + units[index];
        return result;
    }

	    bool Files::copyOrMoveDirectorySubtree(const QString &from,
                                    const QString &to,
                                    const bool copyAndRemove,
                                    const bool overwriteExistingFiles,
                                    QString *errorMessage)
	    {
            if (errorMessage) {
                errorMessage->clear();
            }

	        QDirIterator diritems(from, QDirIterator::Subdirectories);
	        QDir fromDir(from);
	        QDir toDir(to);

            if (!fromDir.exists()) {
                if (errorMessage) {
                    *errorMessage = QObject::tr("The source folder %1 is no longer available.").arg(QDir::toNativeSeparators(from));
                }
                return false;
            }

	        const auto absSourcePathLength = fromDir.absolutePath().length();

	        if (!toDir.mkdir(fromDir.dirName()) && !toDir.cd(fromDir.dirName())) {
                if (errorMessage) {
                    *errorMessage = QObject::tr("Unable to create the destination folder %1.").arg(fromDir.dirName());
                }
                return false;
            }

	        if (!toDir.cd(fromDir.dirName())) {
                if (errorMessage) {
                    *errorMessage = QObject::tr("Unable to access the destination folder %1.").arg(fromDir.dirName());
                }
                return false;
            }

	        while (diritems.hasNext()) {
	            const QFileInfo fileInfo = diritems.nextFileInfo();

            if (fileInfo.fileName().compare(".") == 0
                || fileInfo.fileName().compare("..") == 0) //filters dot and dotdot
                    continue;
            const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
            qDebug() << "subPathStructure: " << subPathStructure;
            const QString constructedAbsolutePath = toDir.canonicalPath() + subPathStructure;
            qDebug() << "constructedAbsolutePath: " << constructedAbsolutePath;

	            if (fileInfo.isDir()) {
	                //Create directory in target folder
	                if (!toDir.mkpath(constructedAbsolutePath)) {
                        if (errorMessage) {
                            *errorMessage = QObject::tr("Unable to create the destination folder %1.")
                                    .arg(QDir::toNativeSeparators(constructedAbsolutePath));
                        }
                        return false;
                    }
	            } else if (fileInfo.isFile()) {
	                //Copy File to target directory

	                if (overwriteExistingFiles) {
	                    //Remove file at target location, if it exists, or QFile::copy will fail
	                    QFile::remove(constructedAbsolutePath);
	                }
	                if (!QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath)) {
                        if (errorMessage) {
                            *errorMessage = QObject::tr("Unable to copy %1.")
                                    .arg(QDir::toNativeSeparators(fileInfo.absoluteFilePath()));
                        }
                        return false;
                    }
	            }
	        }

	        if (copyAndRemove && !fromDir.removeRecursively()) {
                if (errorMessage) {
                    *errorMessage = QObject::tr("Unable to remove the original folder %1 after copying.")
                            .arg(QDir::toNativeSeparators(fromDir.absolutePath()));
                }
                return false;
            }

            return true;
	    }



	Files::~Files() {

		if (m_searchingWatcher.isRunning()) {
			m_searchingWatcher.cancel();
			m_searchingWatcher.waitForFinished();
		}

		if (m_fileViewer) {
			delete m_fileViewer;
			m_fileViewer = nullptr;
		}

		if (ui) {
			delete ui;
			ui = nullptr;
		}
	}

    QItemSelectionModel *Files::activeSelectionModel() const {
        if (ui->tableView_Search->isVisible()) {
            return ui->tableView_Search->selectionModel();
        }
        return ui->listView_Files->selectionModel();
    }

    QString Files::currentDestinationDir() const {
        const QFileInfo selectedInfo(m_currentFilePath);
        if (selectedInfo.exists() && selectedInfo.isDir()) {
            return selectedInfo.absoluteFilePath();
        }
        if (m_currentDir) {
            return m_currentDir->absolutePath();
        }
        return QDir::homePath();
    }

    void Files::refreshCurrentView() {
        if (m_fileSystemModel) {
            const QString rootPath = m_fileSystemModel->rootPath();
            m_fileSystemModel->setRootPath(QString());
            m_fileSystemModel->setRootPath(rootPath);
        }
        if (m_currentDir) {
            setCurrentDir(m_currentDir->absolutePath());
        }
        if (ui->tableView_Search->isVisible() && !ui->lineEdit_Search->text().trimmed().isEmpty()) {
            onSearchClicked();
        }
    }

	    void Files::updateActionStates() {
        const QStringList selection = getSelection();
        const bool hasSelection = !selection.isEmpty();
        const bool singleSelection = selection.size() == 1;
        const bool hasClipboardItems = !m_itemsToCopy.isEmpty() || !m_itemsToMove.isEmpty();
        const QString candidatePath = selectedOrCurrentPath();
        const QFileInfo selectedInfo(candidatePath);

        ui->toolButton_Open->setEnabled(hasSelection || !m_currentFilePath.isEmpty());
        ui->toolButton_Rename->setEnabled(singleSelection);
        ui->toolButton_Delete->setEnabled(hasSelection);
        ui->toolButton_Copy->setEnabled(hasSelection);
        ui->toolButton_Cut->setEnabled(hasSelection);
	        ui->toolButton_Paste->setEnabled(hasClipboardItems && m_fileViewer == nullptr);
        ui->toolButton_NewFolder->setEnabled(m_fileViewer == nullptr);
        ui->toolButton_Home->setEnabled(m_fileViewer == nullptr);
        ui->toolButton_Up->setEnabled(m_fileViewer == nullptr && !getCurrentDir().isEmpty() && QDir(getCurrentDir()).absolutePath() != QDir::rootPath());
        ui->toolButton_Search->setEnabled(m_fileViewer == nullptr && !getCurrentDir().isEmpty());
        if (m_searchOptionsButton) {
            m_searchOptionsButton->setEnabled(m_fileViewer == nullptr && !getCurrentDir().isEmpty());
        }
        ui->toolButton_Filters->setEnabled(m_fileViewer == nullptr && (ui->tableView_Search->isVisible() || !ui->lineEdit_Search->text().trimmed().isEmpty()));
        ui->lineEdit_Path->setEnabled(m_fileViewer == nullptr);
        ui->lineEdit_Search->setEnabled(m_fileViewer == nullptr);

	        if (singleSelection && selectedInfo.exists()) {
	            ui->toolButton_Open->setToolTip(selectedInfo.isDir() ? tr("Open selected folder") : tr("Open selected file"));
	        } else {
	            ui->toolButton_Open->setToolTip(tr("Open selected item"));
	        }
            if (!m_itemsToMove.isEmpty()) {
                ui->toolButton_Paste->setToolTip(tr("Move %1 staged item(s) into the current folder").arg(m_itemsToMove.size()));
            } else if (!m_itemsToCopy.isEmpty()) {
                ui->toolButton_Paste->setToolTip(tr("Copy %1 staged item(s) into the current folder").arg(m_itemsToCopy.size()));
            } else {
                ui->toolButton_Paste->setToolTip(tr("Paste into current folder"));
        }
    }

    void Files::updateClipboardStatus() {
            const int moveCount = m_itemsToMove.size();
            const int copyCount = m_itemsToCopy.size();
            const bool hasClipboardItems = moveCount > 0 || copyCount > 0;
            if (m_clipboardStatusLabel) {
                if (moveCount > 0) {
                    m_clipboardStatusLabel->setText(tr("Move %1 item(s) staged for paste").arg(moveCount));
                } else if (copyCount > 0) {
                    m_clipboardStatusLabel->setText(tr("Copy %1 item(s) staged for paste").arg(copyCount));
                } else {
                    m_clipboardStatusLabel->setText(tr("No staged copy or move action"));
                }
            }
            if (auto *statusWidget = ui->group_ToolBar->findChild<QWidget *>(QStringLiteral("widget_ClipboardStatus"))) {
                statusWidget->setVisible(hasClipboardItems);
            }
            if (m_clearClipboardButton) {
                m_clearClipboardButton->setVisible(hasClipboardItems);
                m_clearClipboardButton->setEnabled(hasClipboardItems && m_fileViewer == nullptr);
            }
        }

        void Files::updateSearchOptionsButton() {
            const bool caseSensitive = ui->toolButton_CaseSensitive->isChecked();
            const bool includeHidden = ui->toolButton_SearchHidden->isChecked();
            const bool includeSystem = ui->toolButton_SearchSystem->isChecked();
            int enabledCount = 0;
            enabledCount += caseSensitive ? 1 : 0;
            enabledCount += includeHidden ? 1 : 0;
            enabledCount += includeSystem ? 1 : 0;

            if (m_searchOptionsButton) {
                const QString buttonText = enabledCount > 0
                        ? tr("Options (%1)").arg(enabledCount)
                        : tr("Options");
                QStringList enabledLabels;
                if (caseSensitive) {
                    enabledLabels.append(tr("case-sensitive"));
                }
                if (includeHidden) {
                    enabledLabels.append(tr("hidden"));
                }
                if (includeSystem) {
                    enabledLabels.append(tr("system"));
                }
                const QString tooltip = enabledLabels.isEmpty()
                        ? tr("Search options")
                        : tr("Search options: %1").arg(enabledLabels.join(QStringLiteral(", ")));
                m_searchOptionsButton->setText(buttonText);
                m_searchOptionsButton->setToolTip(tooltip);
            }
        }

        void Files::updateActionRowLayout() {
            if (!m_actionsLayout || !m_actionsRow) {
                return;
            }

            while (QLayoutItem *item = m_actionsLayout->takeAt(0)) {
                delete item;
            }

            const QList<QPushButton *> actionButtons = {
                ui->toolButton_NewFolder,
                ui->toolButton_Rename,
                ui->toolButton_Delete,
                ui->toolButton_Cut,
                ui->toolButton_Copy,
                ui->toolButton_Paste
            };

            const bool narrowLayout = width() <= 900;
            const int columnCount = narrowLayout ? 3 : actionButtons.size();
            const int rowCount = narrowLayout ? 2 : 1;

            for (int row = 0; row < rowCount; ++row) {
                m_actionsLayout->setRowStretch(row, 0);
            }
            for (int column = 0; column < columnCount; ++column) {
                m_actionsLayout->setColumnStretch(column, 1);
            }

            for (int index = 0; index < actionButtons.size(); ++index) {
                auto *button = actionButtons.at(index);
                if (!button) {
                    continue;
                }

                int row = 0;
                int column = index;
                if (narrowLayout) {
                    row = index / 3;
                    column = index % 3;
                }
                m_actionsLayout->addWidget(button, row, column);
            }
        }

	    void Files::updateTitleLabel() const {
        const QString path = ui->lineEdit_Path->text().trimmed();
        if (path.isEmpty()) {
            ui->labelTitle->setText(tr("Files"));
            return;
        }

        ui->labelTitle->setText(QStringLiteral(
                                    "<html><head/><body>"
                                    "<p style=\"margin:0;\">"
                                    "<span style=\"font-size:26px; font-weight:700;\">%1</span><br/>"
                                    "<span style=\"font-size:16px; font-weight:500;\">%2</span>"
                                    "</p></body></html>")
                                    .arg(tr("Files"), path.toHtmlEscaped()));
    }

    void Files::updateItemInfo(const QString &path) {
        const QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            clearItemInfo();
            return;
        }

        QString permissions;
        const QStorageInfo storageInfo(fileInfo.absolutePath());
        const bool isNTFS = storageInfo.fileSystemType().compare("NTFS") == 0;
        if (isNTFS) {
            qt_ntfs_permission_lookup++;
        }
        if (fileInfo.isReadable()) {
            permissions += tr("Read ");
        }
        if (fileInfo.isWritable()) {
            permissions += tr("Write ");
        }
        if (fileInfo.isExecutable()) {
            permissions += tr("Execute ");
        }
        if (isNTFS) {
            qt_ntfs_permission_lookup--;
        }

        ui->groupBox_ItemInfo->setTitle(fileInfo.fileName().isEmpty() ? fileInfo.absoluteFilePath() : fileInfo.fileName());
        ui->label_Permissions->setText(permissions.trimmed());
        if (fileInfo.isDir()) {
            const QDir dir(fileInfo.absoluteFilePath());
            const int entryCount = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).size();
            ui->label_Size->setText(tr("%1 item(s)").arg(entryCount));
            ui->label_Type->setText(tr("Folder"));
        } else {
            const QMimeDatabase mimedb;
            ui->label_Size->setText(format_bytes(fileInfo.size()));
            ui->label_Type->setText(mimedb.mimeTypeForFile(fileInfo).comment());
        }
        ui->groupBox_ItemInfo->show();
    }

	    void Files::clearItemInfo() {
        ui->groupBox_ItemInfo->hide();
        ui->groupBox_ItemInfo->setTitle(QString());
        ui->label_Size->clear();
        ui->label_Type->clear();
        ui->label_Permissions->clear();
    }

	    QString Files::selectedOrCurrentPath() const {
        const QStringList selection = getSelection();
        if (!selection.isEmpty()) {
            return selection.first();
        }
	        return m_currentFilePath;
	    }

        void Files::clearClipboardState() {
            m_itemsToCopy.clear();
            m_itemsToMove.clear();
            updateClipboardStatus();
            updateActionStates();
        }

        void Files::showOperationSummary(const QString &title, const QString &summary, const QStringList &issues) const {
            QString message = summary;
            if (!issues.isEmpty()) {
                message += QStringLiteral("\n\n") + issues.join(QStringLiteral("\n"));
                drawer::warning(const_cast<Files *>(this), title, message);
            } else {
                drawer::information(const_cast<Files *>(this), title, message);
            }
        }
} // fairwindsk::ui::mydata
