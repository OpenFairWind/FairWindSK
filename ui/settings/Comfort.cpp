//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchScrollArea.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr const char *kColorWindow = "window";
        constexpr const char *kColorPanel = "panel";
        constexpr const char *kColorBase = "base";
        constexpr const char *kColorText = "text";
        constexpr const char *kColorButtonText = "buttonText";
        constexpr const char *kColorButtonTop = "buttonTop";
        constexpr const char *kColorButtonBottom = "buttonBottom";
        constexpr const char *kColorBorder = "border";
        constexpr const char *kColorAccentTop = "accentTop";
        constexpr const char *kColorAccentBottom = "accentBottom";
        constexpr const char *kColorAccentText = "accentText";

        QString normalizedPreset(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == QStringLiteral("sunrise")) {
                return QStringLiteral("dawn");
            }
            if (preset != QStringLiteral("dawn") &&
                preset != QStringLiteral("day") &&
                preset != QStringLiteral("sunset") &&
                preset != QStringLiteral("dusk") &&
                preset != QStringLiteral("night")) {
                return QStringLiteral("day");
            }
            return preset;
        }

        QString comfortThemeResourcePath(const QString &preset) {
            return QStringLiteral(":/resources/stylesheets/%1.qss").arg(normalizedPreset(preset));
        }

        QString buildBackgroundRule(const QString &objectName, const QString &imagePath) {
            if (imagePath.trimmed().isEmpty()) {
                return QString();
            }

            return QStringLiteral(
                "QWidget#%1 {"
                " border-image: url(\"%2\") 0 0 0 0 stretch stretch;"
                " background-position: center;"
                " }")
                .arg(objectName, QUrl::fromLocalFile(imagePath).toString());
        }
    }

    Comfort::Comfort(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
        buildUi();

        const QString currentPreset = normalizedPreset(m_settings->getConfiguration()->getComfortViewPreset());
        const int currentIndex = m_presetComboBox->findData(currentPreset);
        m_presetComboBox->setCurrentIndex(currentIndex >= 0 ? currentIndex : 1);
        loadPresetEditor();
    }

    void Comfort::buildUi() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        auto *scrollArea = new fairwindsk::ui::widgets::TouchScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        rootLayout->addWidget(scrollArea);

        auto *content = new QWidget(scrollArea);
        scrollArea->setWidget(content);

        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(12);

        auto *titleLabel = new QLabel(tr("Comfort Theme Presets"), content);
        contentLayout->addWidget(titleLabel);

        auto *presetRow = new QHBoxLayout();
        presetRow->setContentsMargins(0, 0, 0, 0);
        presetRow->setSpacing(8);
        contentLayout->addLayout(presetRow);

        auto *presetLabel = new QLabel(tr("Preset"), content);
        presetRow->addWidget(presetLabel);

        m_presetComboBox = new fairwindsk::ui::widgets::TouchComboBox(content);
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-dawn.svg")), tr("Dawn"), QStringLiteral("dawn"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-day.svg")), tr("Day"), QStringLiteral("day"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-sunset.svg")), tr("Sunset"), QStringLiteral("sunset"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-dusk.svg")), tr("Dusk"), QStringLiteral("dusk"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-night.svg")), tr("Night"), QStringLiteral("night"));
        presetRow->addWidget(m_presetComboBox, 1);

        auto *buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 0, 0, 0);
        buttonRow->setSpacing(8);
        contentLayout->addLayout(buttonRow);

        m_importButton = new QPushButton(tr("Import QSS"), content);
        m_exportButton = new QPushButton(tr("Export QSS"), content);
        m_resetButton = new QPushButton(tr("Reset Preset"), content);
        buttonRow->addWidget(m_importButton);
        buttonRow->addWidget(m_exportButton);
        buttonRow->addWidget(m_resetButton);
        buttonRow->addStretch(1);

        m_statusLabel = new QLabel(content);
        m_statusLabel->setWordWrap(true);
        contentLayout->addWidget(m_statusLabel);

        auto *visualGroup = new QGroupBox(tr("Visual Editor"), content);
        auto *visualLayout = new QGridLayout(visualGroup);
        visualLayout->setHorizontalSpacing(12);
        visualLayout->setVerticalSpacing(8);
        createColorControl(QString::fromLatin1(kColorWindow), tr("Window"), visualGroup, visualLayout, 0);
        createColorControl(QString::fromLatin1(kColorPanel), tr("Panel"), visualGroup, visualLayout, 1);
        createColorControl(QString::fromLatin1(kColorBase), tr("Fields"), visualGroup, visualLayout, 2);
        createColorControl(QString::fromLatin1(kColorText), tr("Text"), visualGroup, visualLayout, 3);
        createColorControl(QString::fromLatin1(kColorButtonText), tr("Button text"), visualGroup, visualLayout, 4);
        createColorControl(QString::fromLatin1(kColorButtonTop), tr("Button top"), visualGroup, visualLayout, 5);
        createColorControl(QString::fromLatin1(kColorButtonBottom), tr("Button bottom"), visualGroup, visualLayout, 6);
        createColorControl(QString::fromLatin1(kColorBorder), tr("Borders"), visualGroup, visualLayout, 7);
        createColorControl(QString::fromLatin1(kColorAccentTop), tr("Accent top"), visualGroup, visualLayout, 8);
        createColorControl(QString::fromLatin1(kColorAccentBottom), tr("Accent bottom"), visualGroup, visualLayout, 9);
        createColorControl(QString::fromLatin1(kColorAccentText), tr("Accent text"), visualGroup, visualLayout, 10);
        contentLayout->addWidget(visualGroup);

        auto *backgroundGroup = new QGroupBox(tr("Background Images"), content);
        auto *backgroundLayout = new QGridLayout(backgroundGroup);
        backgroundLayout->setHorizontalSpacing(12);
        backgroundLayout->setVerticalSpacing(8);
        createBackgroundImageControl(QStringLiteral("topbar"), tr("Top bar"), backgroundGroup, backgroundLayout, 0);
        createBackgroundImageControl(QStringLiteral("launcher"), tr("Launcher"), backgroundGroup, backgroundLayout, 1);
        createBackgroundImageControl(QStringLiteral("bottombar"), tr("Bottom bar"), backgroundGroup, backgroundLayout, 2);
        contentLayout->addWidget(backgroundGroup);

        auto *previewGroup = new QGroupBox(tr("Live Preview"), content);
        auto *previewLayout = new QVBoxLayout(previewGroup);
        previewLayout->setSpacing(10);

        m_previewTopBar = new QWidget(previewGroup);
        m_previewTopBar->setObjectName(QStringLiteral("TopBar"));
        m_previewTopBar->setMinimumHeight(74);
        auto *topBarLayout = new QHBoxLayout(m_previewTopBar);
        topBarLayout->setContentsMargins(12, 8, 12, 8);
        topBarLayout->setSpacing(10);
        auto *leftStatus = new QLabel(QStringLiteral("FairWindSK"), m_previewTopBar);
        auto *title = new QLabel(tr("Comfort Preview"), m_previewTopBar);
        title->setAlignment(Qt::AlignCenter);
        auto *rightStatus = new QLabel(QStringLiteral("05-04-2026 19:58"), m_previewTopBar);
        topBarLayout->addWidget(leftStatus);
        topBarLayout->addStretch(1);
        topBarLayout->addWidget(title);
        topBarLayout->addStretch(1);
        topBarLayout->addWidget(rightStatus);
        previewLayout->addWidget(m_previewTopBar);

        m_previewLauncher = new QWidget(previewGroup);
        m_previewLauncher->setObjectName(QStringLiteral("Launcher"));
        auto *launcherLayout = new QGridLayout(m_previewLauncher);
        launcherLayout->setContentsMargins(12, 12, 12, 12);
        launcherLayout->setHorizontalSpacing(10);
        launcherLayout->setVerticalSpacing(10);
        for (int row = 0; row < 2; ++row) {
            for (int column = 0; column < 3; ++column) {
                auto *tile = new QPushButton(tr("Tile %1").arg((row * 3) + column + 1), m_previewLauncher);
                tile->setMinimumHeight(56);
                launcherLayout->addWidget(tile, row, column);
            }
        }
        previewLayout->addWidget(m_previewLauncher);

        m_previewTabs = new QTabWidget(previewGroup);
        auto *mainPage = new QWidget(m_previewTabs);
        auto *mainLayout = new QVBoxLayout(mainPage);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);
        auto *fieldLabel = new QLabel(tr("Field preview"), mainPage);
        auto *sampleField = new QLineEdit(QStringLiteral("Sample data"), mainPage);
        auto *buttonRowPreview = new QHBoxLayout();
        auto *primaryButton = new QPushButton(tr("Primary"), mainPage);
        auto *secondaryButton = new QPushButton(tr("Secondary"), mainPage);
        buttonRowPreview->addWidget(primaryButton);
        buttonRowPreview->addWidget(secondaryButton);
        mainLayout->addWidget(fieldLabel);
        mainLayout->addWidget(sampleField);
        mainLayout->addLayout(buttonRowPreview);

        auto *settingsPage = new QWidget(m_previewTabs);
        auto *settingsLayout = new QVBoxLayout(settingsPage);
        settingsLayout->setContentsMargins(12, 12, 12, 12);
        settingsLayout->setSpacing(8);
        auto *settingsButton = new QToolButton(settingsPage);
        settingsButton->setText(tr("Settings button"));
        settingsButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        auto *settingsInfo = new QLabel(tr("Tabs, fields, and buttons update as you edit colors."), settingsPage);
        settingsInfo->setWordWrap(true);
        settingsLayout->addWidget(settingsButton);
        settingsLayout->addWidget(settingsInfo);
        settingsLayout->addStretch(1);

        m_previewTabs->addTab(mainPage, tr("Main"));
        m_previewTabs->addTab(settingsPage, tr("Settings"));
        previewLayout->addWidget(m_previewTabs);

        m_previewBottomBar = new QWidget(previewGroup);
        m_previewBottomBar->setObjectName(QStringLiteral("BottomBar"));
        m_previewBottomBar->setMinimumHeight(78);
        auto *bottomBarLayout = new QHBoxLayout(m_previewBottomBar);
        bottomBarLayout->setContentsMargins(12, 8, 12, 8);
        bottomBarLayout->setSpacing(8);
        for (const QString &label : {tr("My Data"), tr("Apps"), tr("Alarms"), tr("Settings")}) {
            auto *button = new QPushButton(label, m_previewBottomBar);
            button->setMinimumHeight(44);
            bottomBarLayout->addWidget(button);
        }
        previewLayout->addWidget(m_previewBottomBar);

        contentLayout->addWidget(previewGroup);

        auto *advancedGroup = new QGroupBox(tr("Advanced QSS"), content);
        auto *advancedLayout = new QVBoxLayout(advancedGroup);
        advancedLayout->setContentsMargins(8, 8, 8, 8);
        advancedLayout->setSpacing(8);
        m_styleEditor = new QPlainTextEdit(advancedGroup);
        m_styleEditor->setPlaceholderText(tr("Edit the full QSS for the selected comfort preset."));
        m_styleEditor->setMinimumHeight(320);
        advancedLayout->addWidget(m_styleEditor);
        contentLayout->addWidget(advancedGroup, 1);

        connect(m_presetComboBox,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Comfort::onPresetChanged);
        connect(m_styleEditor, &QPlainTextEdit::textChanged, this, &Comfort::onStyleSheetChanged);
        connect(m_importButton, &QPushButton::clicked, this, &Comfort::importStyleSheet);
        connect(m_exportButton, &QPushButton::clicked, this, &Comfort::exportStyleSheet);
        connect(m_resetButton, &QPushButton::clicked, this, &Comfort::resetPreset);
    }

    void Comfort::createColorControl(const QString &key, const QString &labelText, QWidget *parent, QGridLayout *layout, const int row) {
        auto *label = new QLabel(labelText, parent);
        auto *button = new QPushButton(parent);
        button->setMinimumHeight(44);
        button->setObjectName(QStringLiteral("button_%1").arg(key));
        layout->addWidget(label, row, 0);
        layout->addWidget(button, row, 1);
        m_colorButtons.insert(key, button);
        connect(button, &QPushButton::clicked, this, [this, key]() {
            pickColor(key);
        });
    }

    void Comfort::createBackgroundImageControl(const QString &area, const QString &labelText, QWidget *parent, QGridLayout *layout, const int row) {
        auto *label = new QLabel(labelText, parent);
        auto *pathLabel = new QLabel(tr("None"), parent);
        pathLabel->setWordWrap(true);
        auto *browseButton = new QPushButton(tr("Browse"), parent);
        auto *clearButton = new QPushButton(tr("Clear"), parent);
        browseButton->setMinimumHeight(40);
        clearButton->setMinimumHeight(40);

        layout->addWidget(label, row, 0);
        layout->addWidget(pathLabel, row, 1);
        layout->addWidget(browseButton, row, 2);
        layout->addWidget(clearButton, row, 3);

        m_backgroundPathLabels.insert(area, pathLabel);
        connect(browseButton, &QPushButton::clicked, this, [this, area]() {
            browseBackgroundImage(area);
        });
        connect(clearButton, &QPushButton::clicked, this, [this, area]() {
            clearBackgroundImage(area);
        });
    }

    void Comfort::onPresetChanged(const int) {
        loadPresetEditor();
    }

    void Comfort::onStyleSheetChanged() {
        if (m_isUpdating || !m_settings || !m_styleEditor) {
            return;
        }

        const QString preset = selectedPreset();
        m_settings->getConfiguration()->setComfortThemeStyleSheet(preset, m_styleEditor->toPlainText());
        if (!m_isSyncingVisualControls) {
            syncVisualControlsFromStyleSheet();
        }
        updateColorButtons();
        updateBackgroundImageLabels();
        updatePreview();
        updateStatusLabel();
        m_settings->markDirty(FairWindSK::RuntimeUi, 250);
    }

    void Comfort::importStyleSheet() {
        const QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Import Comfort Theme"),
            QString(),
            tr("Style Sheets (*.qss *.css);;All Files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        m_styleEditor->setPlainText(QString::fromUtf8(file.readAll()));
    }

    void Comfort::exportStyleSheet() {
        const QString preset = selectedPreset();
        const QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("Export Comfort Theme"),
            QStringLiteral("%1.qss").arg(preset),
            tr("Style Sheets (*.qss);;All Files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&file);
        stream << m_styleEditor->toPlainText();
    }

    void Comfort::resetPreset() {
        const QString preset = selectedPreset();
        m_settings->getConfiguration()->clearComfortThemeStyleSheet(preset);
        for (const QString &area : {QStringLiteral("topbar"), QStringLiteral("launcher"), QStringLiteral("bottombar")}) {
            m_settings->getConfiguration()->clearComfortBackgroundImagePath(preset, area);
        }
        loadPresetEditor();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    QString Comfort::selectedPreset() const {
        return normalizedPreset(m_presetComboBox ? m_presetComboBox->currentData().toString() : QString());
    }

    QString Comfort::defaultStyleSheetForPreset(const QString &preset) const {
        QFile file(comfortThemeResourcePath(preset));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }

        return QString::fromUtf8(file.readAll());
    }

    QString Comfort::effectiveStyleSheetForPreset(const QString &preset) const {
        const QString configured = m_settings->getConfiguration()->getComfortThemeStyleSheet(preset);
        return configured.isEmpty() ? defaultStyleSheetForPreset(preset) : configured;
    }

    QString Comfort::effectiveBackgroundStyleSheetForPreset(const QString &preset) const {
        const auto *configuration = m_settings->getConfiguration();
        return buildBackgroundRule(QStringLiteral("TopBar"), configuration->getComfortBackgroundImagePath(preset, QStringLiteral("topbar")))
            + buildBackgroundRule(QStringLiteral("Launcher"), configuration->getComfortBackgroundImagePath(preset, QStringLiteral("launcher")))
            + buildBackgroundRule(QStringLiteral("BottomBar"), configuration->getComfortBackgroundImagePath(preset, QStringLiteral("bottombar")));
    }

    void Comfort::loadPresetEditor() {
        if (!m_styleEditor) {
            return;
        }

        {
            const QSignalBlocker blocker(m_styleEditor);
            m_isUpdating = true;
            m_styleEditor->setPlainText(effectiveStyleSheetForPreset(selectedPreset()));
            m_isUpdating = false;
        }
        syncVisualControlsFromStyleSheet();
        updateColorButtons();
        updateBackgroundImageLabels();
        updatePreview();
        updateStatusLabel();
    }

    void Comfort::updateStatusLabel() {
        if (!m_statusLabel) {
            return;
        }

        const QString preset = selectedPreset();
        const bool hasOverride = !m_settings->getConfiguration()->getComfortThemeStyleSheet(preset).isEmpty();
        m_statusLabel->setText(
            hasOverride
                ? tr("Editing the saved override for the %1 preset. Color and image changes are stored in fairwindsk.json and applied live.")
                      .arg(preset)
                : tr("Editing the default %1 preset. Any visual changes you make here will be saved as an override in fairwindsk.json.")
                      .arg(preset));
    }

    void Comfort::updatePreview() {
        const QString preset = selectedPreset();
        const QString previewStyleSheet = removeGeneratedOverrideBlock(m_styleEditor->toPlainText())
            + buildVisualOverrideBlock()
            + effectiveBackgroundStyleSheetForPreset(preset);

        for (QWidget *widget : {m_previewTopBar, m_previewLauncher, static_cast<QWidget *>(m_previewTabs), m_previewBottomBar}) {
            if (widget) {
                widget->setStyleSheet(previewStyleSheet);
            }
        }
    }

    void Comfort::updateColorButtons() {
        for (auto it = m_colorButtons.cbegin(); it != m_colorButtons.cend(); ++it) {
            QPushButton *button = it.value();
            if (!button) {
                continue;
            }

            const QColor color = m_visualColors.value(it.key());
            button->setText(color.name(QColor::HexRgb).toUpper());
            const QColor textColor = color.lightnessF() > 0.55 ? QColor(QStringLiteral("#111111")) : QColor(QStringLiteral("#f8f8f8"));
            button->setStyleSheet(QStringLiteral(
                                      "QPushButton {"
                                      " background: %1;"
                                      " color: %2;"
                                      " border: 1px solid %3;"
                                      " border-radius: 6px;"
                                      " }")
                                      .arg(color.name(), textColor.name(), color.darker(130).name()));
        }
    }

    void Comfort::updateBackgroundImageLabels() {
        const QString preset = selectedPreset();
        for (auto it = m_backgroundPathLabels.cbegin(); it != m_backgroundPathLabels.cend(); ++it) {
            QLabel *label = it.value();
            if (!label) {
                continue;
            }

            const QString path = m_settings->getConfiguration()->getComfortBackgroundImagePath(preset, it.key());
            label->setText(path.isEmpty() ? tr("None") : QFileInfo(path).fileName());
            label->setToolTip(path);
        }
    }

    void Comfort::pickColor(const QString &key) {
        const QColor selected = QColorDialog::getColor(colorValue(key), this, tr("Choose Color"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }

        m_visualColors.insert(key, selected);
        applyVisualThemeOverride();
    }

    void Comfort::browseBackgroundImage(const QString &area) {
        const QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Choose Background Image"),
            QString(),
            tr("Images (*.png *.jpg *.jpeg *.bmp *.webp);;All Files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        m_settings->getConfiguration()->setComfortBackgroundImagePath(selectedPreset(), area, fileName);
        updateBackgroundImageLabels();
        updatePreview();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::clearBackgroundImage(const QString &area) {
        m_settings->getConfiguration()->clearComfortBackgroundImagePath(selectedPreset(), area);
        updateBackgroundImageLabels();
        updatePreview();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::applyVisualThemeOverride() {
        const QString updatedStyleSheet = styleSheetWithVisualOverride();
        {
            const QSignalBlocker blocker(m_styleEditor);
            m_isUpdating = true;
            m_styleEditor->setPlainText(updatedStyleSheet);
            m_isUpdating = false;
        }

        m_settings->getConfiguration()->setComfortThemeStyleSheet(selectedPreset(), updatedStyleSheet);
        updateColorButtons();
        updatePreview();
        updateStatusLabel();
        m_settings->markDirty(FairWindSK::RuntimeUi, 250);
    }

    void Comfort::syncVisualControlsFromStyleSheet() {
        m_isSyncingVisualControls = true;

        const QString styleSheet = m_styleEditor ? m_styleEditor->toPlainText() : QString();
        m_visualColors.insert(QString::fromLatin1(kColorWindow), extractColor(styleSheet, QStringLiteral("QWidget\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#f7f2d9"))));
        m_visualColors.insert(QString::fromLatin1(kColorPanel), extractColor(styleSheet, QStringLiteral("QMainWindow[\\s\\S]*?QAbstractScrollArea\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#fbf6e3"))));
        m_visualColors.insert(QString::fromLatin1(kColorBase), extractColor(styleSheet, QStringLiteral("QLineEdit[\\s\\S]*?QDateTimeEdit\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#fffdf3"))));
        m_visualColors.insert(QString::fromLatin1(kColorText), extractColor(styleSheet, QStringLiteral("QWidget\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#10233a"))));
        m_visualColors.insert(QString::fromLatin1(kColorButtonText), extractColor(styleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), m_visualColors.value(QString::fromLatin1(kColorText))));
        m_visualColors.insert(QString::fromLatin1(kColorButtonTop), extractColor(styleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#fff7c4"))));
        m_visualColors.insert(QString::fromLatin1(kColorButtonBottom), extractColor(styleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*stop:1\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#dbc66c"))));
        m_visualColors.insert(QString::fromLatin1(kColorBorder), extractColor(styleSheet, QStringLiteral("QToolTip\\s*\\{[^}]*border\\s*:\\s*1px solid\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#8b7f4e"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentTop), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#2d5ea6"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentBottom), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:1\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#1f447a"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentText), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#f7fbff"))));

        m_isSyncingVisualControls = false;
    }

    QString Comfort::buildVisualOverrideBlock() const {
        const QColor windowColor = colorValue(QString::fromLatin1(kColorWindow));
        const QColor panelColor = colorValue(QString::fromLatin1(kColorPanel));
        const QColor baseColor = colorValue(QString::fromLatin1(kColorBase));
        const QColor textColor = colorValue(QString::fromLatin1(kColorText));
        const QColor buttonTextColor = colorValue(QString::fromLatin1(kColorButtonText));
        const QColor buttonTopColor = colorValue(QString::fromLatin1(kColorButtonTop));
        const QColor buttonBottomColor = colorValue(QString::fromLatin1(kColorButtonBottom));
        const QColor borderColor = colorValue(QString::fromLatin1(kColorBorder));
        const QColor accentTopColor = colorValue(QString::fromLatin1(kColorAccentTop));
        const QColor accentBottomColor = colorValue(QString::fromLatin1(kColorAccentBottom));
        const QColor accentTextColor = colorValue(QString::fromLatin1(kColorAccentText));
        const QColor hoverTopColor = buttonTopColor.lighter(108);
        const QColor hoverBottomColor = buttonBottomColor.lighter(110);
        const QColor pressedTopColor = buttonTopColor.darker(118);
        const QColor pressedBottomColor = buttonBottomColor.darker(132);

        return QStringLiteral(
                   "%1\n"
                   "QWidget {\n"
                   "    color: %2;\n"
                   "    background: %3;\n"
                   "}\n"
                   "QMainWindow, QDialog, QWidget, QFrame, QStackedWidget, QSplitter, QScrollArea, QAbstractScrollArea {\n"
                   "    background: %4;\n"
                   "}\n"
                   "QMenuBar, QMenu, QStatusBar, QToolTip {\n"
                   "    background: %4;\n"
                   "    color: %2;\n"
                   "}\n"
                   "QToolTip {\n"
                   "    border: 1px solid %5;\n"
                   "}\n"
                   "QAbstractScrollArea {\n"
                   "    background: %4;\n"
                   "    border: 1px solid %5;\n"
                   "    border-radius: 6px;\n"
                   "}\n"
                   "QLineEdit, QTextEdit, QPlainTextEdit, QListView, QListWidget, QTreeView, QTreeWidget, QTableView, QTableWidget,\n"
                   "QComboBox, QAbstractSpinBox, QDateTimeEdit {\n"
                   "    background: %6;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %5;\n"
                   "    border-radius: 6px;\n"
                   "    selection-background-color: %7;\n"
                   "    selection-color: %8;\n"
                   "}\n"
                   "QComboBox QAbstractItemView {\n"
                   "    background: %6;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %5;\n"
                   "    selection-background-color: %7;\n"
                   "    selection-color: %8;\n"
                   "}\n"
                   "QToolButton, QPushButton {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %9, stop:1 %10);\n"
                   "    color: %11;\n"
                   "    border: 1px solid %5;\n"
                   "    border-radius: 6px;\n"
                   "}\n"
                   "QToolButton:hover, QPushButton:hover {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %12, stop:1 %13);\n"
                   "}\n"
                   "QToolButton:pressed, QPushButton:pressed, QToolButton:checked, QPushButton:checked {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %14, stop:1 %15);\n"
                   "    color: %8;\n"
                   "}\n"
                   "QTabWidget::pane {\n"
                   "    border: 1px solid %5;\n"
                   "    top: -1px;\n"
                   "}\n"
                   "QTabBar::tab {\n"
                   "    color: %11;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %9, stop:1 %10);\n"
                   "    border: 1px solid %5;\n"
                   "}\n"
                   "QTabBar::tab:selected {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %7, stop:1 %16);\n"
                   "    color: %8;\n"
                   "    border-color: %17;\n"
                   "}\n"
                   "QTabBar::tab:hover:!selected {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %12, stop:1 %13);\n"
                   "}\n"
                   "QCheckBox, QLabel, QGroupBox {\n"
                   "    color: %2;\n"
                   "}\n"
                   "QLabel {\n"
                   "    background: transparent;\n"
                   "}\n"
                   "QGroupBox {\n"
                   "    background: transparent;\n"
                   "    border: 1px solid %5;\n"
                   "}\n"
                   "QGroupBox::title {\n"
                   "    subcontrol-origin: margin;\n"
                   "    color: %7;\n"
                   "}\n"
                   "QCheckBox::indicator {\n"
                   "    border: 1px solid %5;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %9, stop:1 %10);\n"
                   "}\n"
                   "QCheckBox::indicator:checked {\n"
                   "    border-color: %17;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %7, stop:1 %16);\n"
                   "}\n"
                   "QHeaderView::section, QTableCornerButton::section {\n"
                   "    background: %4;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %5;\n"
                   "}\n"
                   "QProgressBar {\n"
                   "    border: 1px solid %5;\n"
                   "    border-radius: 6px;\n"
                   "    background: %4;\n"
                   "    color: %2;\n"
                   "    text-align: center;\n"
                   "}\n"
                   "QProgressBar::chunk {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %7, stop:1 %16);\n"
                   "    border-radius: 5px;\n"
                   "}\n"
                   "%18\n")
            .arg(generatedOverrideStartMarker(),
                 textColor.name(),
                 windowColor.name(),
                 panelColor.name(),
                 borderColor.name(),
                 baseColor.name(),
                 accentTopColor.name(),
                 accentTextColor.name(),
                 buttonTopColor.name(),
                 buttonBottomColor.name(),
                 buttonTextColor.name(),
                 hoverTopColor.name(),
                 hoverBottomColor.name(),
                 pressedTopColor.name(),
                 pressedBottomColor.name(),
                 accentBottomColor.name(),
                 accentTopColor.lighter(125).name(),
                 generatedOverrideEndMarker());
    }

    QString Comfort::styleSheetWithVisualOverride() const {
        return removeGeneratedOverrideBlock(m_styleEditor ? m_styleEditor->toPlainText() : QString())
            + buildVisualOverrideBlock();
    }

    QColor Comfort::colorValue(const QString &key) const {
        return m_visualColors.value(key, QColor(QStringLiteral("#808080")));
    }

    QColor Comfort::extractColor(const QString &styleSheet, const QString &pattern, const QColor &fallback) const {
        const QString value = extractCapturedValue(styleSheet, pattern);
        const QColor color(value);
        return color.isValid() ? color : fallback;
    }

    QString Comfort::extractCapturedValue(const QString &styleSheet, const QString &pattern, const QString &fallback) const {
        const QRegularExpression expression(
            pattern,
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpressionMatch match = expression.match(styleSheet);
        return match.hasMatch() ? match.captured(1) : fallback;
    }

    QString Comfort::generatedOverrideStartMarker() {
        return QStringLiteral("\n/* FairWindSK comfort visual override start */\n");
    }

    QString Comfort::generatedOverrideEndMarker() {
        return QStringLiteral("/* FairWindSK comfort visual override end */\n");
    }

    QString Comfort::removeGeneratedOverrideBlock(QString styleSheet) {
        const QRegularExpression overrideExpression(
            QStringLiteral(
                "\\n?/\\* FairWindSK comfort visual override start \\*/[\\s\\S]*?/\\* FairWindSK comfort visual override end \\*/\\n?"),
            QRegularExpression::CaseInsensitiveOption);
        styleSheet.remove(overrideExpression);
        return styleSheet.trimmed() + QLatin1Char('\n');
    }
}
