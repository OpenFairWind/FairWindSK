//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/launcher/Launcher.hpp"
#include "ui/widgets/TouchColorPicker.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchScrollArea.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr const char *kColorWindow = "window";
        constexpr const char *kColorApplicationBackground = "applicationBackground";
        constexpr const char *kColorPanel = "panel";
        constexpr const char *kColorBase = "base";
        constexpr const char *kColorText = "text";
        constexpr const char *kColorButtonBackground = "buttonBackground";
        constexpr const char *kColorButtonText = "buttonText";
        constexpr const char *kColorScrollBarBackground = "scrollBarBackground";
        constexpr const char *kColorScrollBarKnob = "scrollBarKnob";
        constexpr const char *kColorBorder = "border";
        constexpr const char *kColorAccentTop = "accentTop";
        constexpr const char *kColorAccentBottom = "accentBottom";
        constexpr const char *kColorAccentText = "accentText";
        constexpr const char *kColorIconDefault = "iconDefault";

        QString normalizedPreset(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == QStringLiteral("sunrise")) {
                return QStringLiteral("dawn");
            }
            if (preset != QStringLiteral("default") &&
                preset != QStringLiteral("dawn") &&
                preset != QStringLiteral("day") &&
                preset != QStringLiteral("sunset") &&
                preset != QStringLiteral("dusk") &&
                preset != QStringLiteral("night")) {
                return QStringLiteral("default");
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

        QString buildSelectorBorderImageRule(const QString &selector, const QString &imagePath) {
            if (imagePath.trimmed().isEmpty()) {
                return QString();
            }

            return QStringLiteral(
                "%1 {"
                " border-image: url(\"%2\") 0 0 0 0 stretch stretch;"
                " background: transparent;"
                " border: 0px;"
                " }")
                .arg(selector, QUrl::fromLocalFile(imagePath).toString());
        }

        QString buildCheckboxIndicatorRule(const QString &selector, const QString &imagePath) {
            if (imagePath.trimmed().isEmpty()) {
                return QString();
            }

            return QStringLiteral(
                "%1 {"
                " image: url(\"%2\");"
                " border: 0px;"
                " background: transparent;"
                " width: 28px;"
                " height: 28px;"
                " }")
                .arg(selector, QUrl::fromLocalFile(imagePath).toString());
        }

        QStringList comfortBackgroundAreas() {
            return {
                QStringLiteral("topbar"),
                QStringLiteral("launcher"),
                QStringLiteral("bottombar"),
                QStringLiteral("buttons-default"),
                QStringLiteral("buttons-selected"),
                QStringLiteral("tabs-default"),
                QStringLiteral("tabs-selected"),
                QStringLiteral("checkbox-default"),
                QStringLiteral("checkbox-selected")
            };
        }

    }

    Comfort::Comfort(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
        buildUi();

        const QString currentPreset = normalizedPreset(m_settings->getConfiguration()->getComfortViewPreset());
        const int currentIndex = m_presetComboBox->findData(currentPreset);
        m_presetComboBox->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
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
        contentLayout->setContentsMargins(6, 6, 6, 6);
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
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-default.svg")), tr("Default"), QStringLiteral("default"));
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
        m_editorButton = new QPushButton(tr("Editor"), content);
        m_editorButton->setCheckable(true);
        m_resetButton = new QPushButton(tr("Reset Preset"), content);
        buttonRow->addWidget(m_importButton);
        buttonRow->addWidget(m_exportButton);
        buttonRow->addWidget(m_editorButton);
        buttonRow->addWidget(m_resetButton);
        buttonRow->addStretch(1);

        m_statusLabel = new QLabel(content);
        m_statusLabel->hide();

        auto *paletteGroup = new QGroupBox(tr("Palette"), content);
        auto *paletteGroupLayout = new QVBoxLayout(paletteGroup);
        paletteGroupLayout->setContentsMargins(8, 8, 8, 8);
        paletteGroupLayout->setSpacing(8);

        auto *paletteHint = new QLabel(tr("Tap a swatch to adjust the saved colors for the selected comfort preset."), paletteGroup);
        paletteHint->setWordWrap(true);
        paletteGroupLayout->addWidget(paletteHint);

        m_visualEditorWidget = new QWidget(paletteGroup);
        auto *visualLayout = new QGridLayout(m_visualEditorWidget);
        visualLayout->setContentsMargins(0, 0, 0, 0);
        visualLayout->setHorizontalSpacing(12);
        visualLayout->setVerticalSpacing(8);
        createColorControl(QString::fromLatin1(kColorWindow), tr("Window"), m_visualEditorWidget, visualLayout, 0);
        createColorControl(QString::fromLatin1(kColorApplicationBackground), tr("Application background"), m_visualEditorWidget, visualLayout, 1);
        createColorControl(QString::fromLatin1(kColorPanel), tr("Panel"), m_visualEditorWidget, visualLayout, 2);
        createColorControl(QString::fromLatin1(kColorBase), tr("Fields"), m_visualEditorWidget, visualLayout, 3);
        createColorControl(QString::fromLatin1(kColorText), tr("Text"), m_visualEditorWidget, visualLayout, 4);
        createColorControl(QString::fromLatin1(kColorButtonBackground), tr("Button background"), m_visualEditorWidget, visualLayout, 5);
        createColorControl(QString::fromLatin1(kColorButtonText), tr("Button text"), m_visualEditorWidget, visualLayout, 6);
        createColorControl(QString::fromLatin1(kColorScrollBarBackground), tr("Scroll bar background"), m_visualEditorWidget, visualLayout, 7);
        createColorControl(QString::fromLatin1(kColorScrollBarKnob), tr("Scroll bar knob"), m_visualEditorWidget, visualLayout, 8);
        createColorControl(QString::fromLatin1(kColorBorder), tr("Borders"), m_visualEditorWidget, visualLayout, 9);
        createColorControl(QString::fromLatin1(kColorAccentTop), tr("Accent top"), m_visualEditorWidget, visualLayout, 10);
        createColorControl(QString::fromLatin1(kColorAccentBottom), tr("Accent bottom"), m_visualEditorWidget, visualLayout, 11);
        createColorControl(QString::fromLatin1(kColorAccentText), tr("Accent text"), m_visualEditorWidget, visualLayout, 12);
        createColorControl(QString::fromLatin1(kColorIconDefault), tr("SVG icon color"), m_visualEditorWidget, visualLayout, 13);
        paletteGroupLayout->addWidget(m_visualEditorWidget);
        contentLayout->addWidget(paletteGroup);

        auto *imagesGroup = new QGroupBox(tr("Theme Images"), content);
        auto *imagesLayout = new QVBoxLayout(imagesGroup);
        imagesLayout->setContentsMargins(8, 8, 8, 8);
        imagesLayout->setSpacing(10);

        auto *imagesHint = new QLabel(tr("Choose optional images for major surfaces and control states. Clear any entry to fall back to the QSS colors."), imagesGroup);
        imagesHint->setWordWrap(true);
        imagesLayout->addWidget(imagesHint);

        imagesLayout->addWidget(createImageGroup(
            tr("Surface Images"),
            imagesGroup,
            {
                {QStringLiteral("topbar"), tr("Top bar")},
                {QStringLiteral("launcher"), tr("Launcher")},
                {QStringLiteral("bottombar"), tr("Bottom bar")}
            }));
        imagesLayout->addWidget(createImageGroup(
            tr("Control Images"),
            imagesGroup,
            {
                {QStringLiteral("buttons-default"), tr("Buttons")},
                {QStringLiteral("buttons-selected"), tr("Buttons selected")},
                {QStringLiteral("tabs-default"), tr("Tabs")},
                {QStringLiteral("tabs-selected"), tr("Tabs selected")},
                {QStringLiteral("checkbox-default"), tr("Checkbox")},
                {QStringLiteral("checkbox-selected"), tr("Checkbox selected")}
            }));
        contentLayout->addWidget(imagesGroup);

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

        m_previewLauncher = new fairwindsk::ui::launcher::Launcher(previewGroup);
        m_previewLauncher->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_previewLauncher->setMinimumHeight(220);
        m_previewLauncher->refreshFromConfiguration(true);
        previewLayout->addWidget(m_previewLauncher);

        m_previewTabs = new QTabWidget(previewGroup);
        auto *mainPage = new QWidget(m_previewTabs);
        auto *mainLayout = new QVBoxLayout(mainPage);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);
        auto *fieldLabel = new QLabel(tr("Field preview"), mainPage);
        auto *sampleField = new QLineEdit(QStringLiteral("Sample data"), mainPage);
        auto *sampleCheckBox = new QCheckBox(tr("Sample checkbox"), mainPage);
        auto *buttonRowPreview = new QHBoxLayout();
        auto *primaryButton = new QPushButton(tr("Primary"), mainPage);
        auto *secondaryButton = new QPushButton(tr("Secondary"), mainPage);
        buttonRowPreview->addWidget(primaryButton);
        buttonRowPreview->addWidget(secondaryButton);
        mainLayout->addWidget(fieldLabel);
        mainLayout->addWidget(sampleField);
        mainLayout->addWidget(sampleCheckBox);
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

        m_advancedGroup = new QGroupBox(tr("Advanced QSS"), content);
        m_advancedGroup->setVisible(false);
        auto *advancedLayout = new QVBoxLayout(m_advancedGroup);
        advancedLayout->setContentsMargins(8, 8, 8, 8);
        advancedLayout->setSpacing(8);
        m_styleEditor = new QPlainTextEdit(m_advancedGroup);
        m_styleEditor->setPlaceholderText(tr("Edit the full QSS for the selected comfort preset."));
        m_styleEditor->setMinimumHeight(320);
        advancedLayout->addWidget(m_styleEditor);
        contentLayout->addWidget(m_advancedGroup, 1);

        connect(m_presetComboBox,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Comfort::onPresetChanged);
        connect(m_styleEditor, &QPlainTextEdit::textChanged, this, &Comfort::onStyleSheetChanged);
        connect(m_importButton, &QPushButton::clicked, this, &Comfort::importStyleSheet);
        connect(m_exportButton, &QPushButton::clicked, this, &Comfort::exportStyleSheet);
        connect(m_editorButton, &QPushButton::toggled, this, [this](const bool checked) {
            if (m_advancedGroup) {
                m_advancedGroup->setVisible(checked);
            }
            if (m_visualEditorWidget) {
                m_visualEditorWidget->setVisible(!checked);
            }
        });
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

    QGroupBox *Comfort::createImageGroup(const QString &title, QWidget *parent, const QList<QPair<QString, QString>> &entries) {
        auto *group = new QGroupBox(title, parent);
        auto *layout = new QGridLayout(group);
        layout->setHorizontalSpacing(12);
        layout->setVerticalSpacing(8);

        int row = 0;
        for (const auto &entry : entries) {
            createBackgroundImageControl(entry.first, entry.second, group, layout, row);
            ++row;
        }

        return group;
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
            persistVisualColorsToConfiguration();
        }
        updateColorButtons();
        updateBackgroundImageLabels();
        updatePreview();
        updateStatusLabel();
        m_settings->markDirty(FairWindSK::RuntimeUi, 250);
    }

    void Comfort::importStyleSheet() {
        const QString fileName = fairwindsk::ui::drawer::getOpenFilePath(
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
        const QString fileName = fairwindsk::ui::drawer::getSaveFilePath(
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
        m_settings->getConfiguration()->clearComfortThemeColors(preset);
        for (const QString &area : comfortBackgroundAreas()) {
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
            + buildBackgroundRule(QStringLiteral("BottomBar"), configuration->getComfortBackgroundImagePath(preset, QStringLiteral("bottombar")))
            + buildSelectorBorderImageRule(
                QStringLiteral("QToolButton, QPushButton"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("buttons-default")))
            + buildSelectorBorderImageRule(
                QStringLiteral("QToolButton:pressed, QPushButton:pressed, QToolButton:checked, QPushButton:checked"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("buttons-selected")))
            + buildSelectorBorderImageRule(
                QStringLiteral("QTabBar::tab"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("tabs-default")))
            + buildSelectorBorderImageRule(
                QStringLiteral("QTabBar::tab:selected"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("tabs-selected")))
            + buildCheckboxIndicatorRule(
                QStringLiteral("QCheckBox::indicator:unchecked"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("checkbox-default")))
            + buildCheckboxIndicatorRule(
                QStringLiteral("QCheckBox::indicator:checked"),
                configuration->getComfortBackgroundImagePath(preset, QStringLiteral("checkbox-selected")));
    }

    void Comfort::loadPresetEditor() {
        if (!m_styleEditor) {
            return;
        }

        const QString preset = selectedPreset();
        const QString styleSheet = effectiveStyleSheetForPreset(preset);
        {
            const QSignalBlocker blocker(m_styleEditor);
            m_isUpdating = true;
            m_styleEditor->setPlainText(styleSheet);
            m_isUpdating = false;
        }

        const bool hasGeneratedOverrideBlock =
            styleSheet.contains(generatedOverrideStartMarker(), Qt::CaseInsensitive) &&
            styleSheet.contains(generatedOverrideEndMarker(), Qt::CaseInsensitive);

        if (hasGeneratedOverrideBlock) {
            syncVisualControlsFromStyleSheet();
            persistVisualColorsToConfiguration();
        } else if (!loadVisualColorsFromConfiguration()) {
            syncVisualControlsFromStyleSheet();
            persistVisualColorsToConfiguration();
        }
        updateColorButtons();
        updateBackgroundImageLabels();
        updatePreview();
        updateStatusLabel();
    }

    void Comfort::updateStatusLabel() {
        if (m_statusLabel) {
            m_statusLabel->clear();
            m_statusLabel->hide();
        }
    }

    void Comfort::updatePreview() {
        const QString preset = selectedPreset();
        const QString previewStyleSheet = removeGeneratedOverrideBlock(m_styleEditor->toPlainText())
            + buildVisualOverrideBlock()
            + effectiveBackgroundStyleSheetForPreset(preset);

        for (QWidget *widget : {m_previewTopBar, static_cast<QWidget *>(m_previewLauncher), static_cast<QWidget *>(m_previewTabs), m_previewBottomBar}) {
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

    QString Comfort::colorLabel(const QString &key) const {
        if (!m_colorButtons.contains(key) || !m_colorButtons.value(key)) {
            return key;
        }

        const auto *layout = qobject_cast<QGridLayout *>(
            m_colorButtons.value(key)->parentWidget() ? m_colorButtons.value(key)->parentWidget()->layout() : nullptr);
        if (!layout) {
            return key;
        }

        for (int row = 0; row < layout->rowCount(); ++row) {
            if (layout->itemAtPosition(row, 1) && layout->itemAtPosition(row, 1)->widget() == m_colorButtons.value(key)) {
                if (auto *label = qobject_cast<QLabel *>(layout->itemAtPosition(row, 0)->widget())) {
                    return label->text();
                }
            }
        }

        return key;
    }

    void Comfort::pickColor(const QString &key) {
        bool accepted = false;
        const QColor color = fairwindsk::ui::widgets::TouchColorPickerDialog::getColor(
            this,
            tr("Choose %1").arg(colorLabel(key)),
            colorValue(key),
            &accepted,
            false);
        if (!accepted) {
            return;
        }

        m_visualColors.insert(key, color);
        applyVisualThemeOverride();
    }

    void Comfort::browseBackgroundImage(const QString &area) {
        const QString fileName = fairwindsk::ui::drawer::getOpenFilePath(
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
        const QString preset = selectedPreset();
        const QString updatedStyleSheet = styleSheetWithVisualOverride();
        {
            const QSignalBlocker blocker(m_styleEditor);
            m_isUpdating = true;
            m_styleEditor->setPlainText(updatedStyleSheet);
            m_isUpdating = false;
        }

        auto *configuration = m_settings->getConfiguration();
        configuration->setComfortThemeStyleSheet(preset, updatedStyleSheet);
        for (auto it = m_visualColors.cbegin(); it != m_visualColors.cend(); ++it) {
            configuration->setComfortThemeColor(preset, it.key(), it.value());
        }
        updateColorButtons();
        updatePreview();
        updateStatusLabel();
        m_settings->markDirty(FairWindSK::RuntimeUi, 250);
    }

    bool Comfort::loadVisualColorsFromConfiguration() {
        const QString preset = selectedPreset();
        auto *configuration = m_settings ? m_settings->getConfiguration() : nullptr;
        if (!configuration) {
            return false;
        }

        const QString baselineStyleSheet = defaultStyleSheetForPreset(preset);
        const QColor baselineTextColor = extractColor(
            baselineStyleSheet,
            QStringLiteral("QWidget\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"),
            QColor(QStringLiteral("#e9f3ff")));
        const QColor baselineAccentTextColor = extractColor(
            baselineStyleSheet,
            QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"),
            QColor(QStringLiteral("#f7fbff")));

        const QList<QPair<QString, QColor>> defaults = {
            {QString::fromLatin1(kColorWindow), extractColor(baselineStyleSheet, QStringLiteral("QWidget\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#0f1622")))},
            {QString::fromLatin1(kColorApplicationBackground), extractColor(baselineStyleSheet, QStringLiteral("QMainWindow[\\s\\S]*?QAbstractScrollArea\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#15283d")))},
            {QString::fromLatin1(kColorPanel), extractColor(baselineStyleSheet, QStringLiteral("QMenuBar,\\s*QMenu,\\s*QStatusBar,\\s*QToolTip\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#1d3146")))},
            {QString::fromLatin1(kColorBase), extractColor(baselineStyleSheet, QStringLiteral("QLineEdit[\\s\\S]*?QDateTimeEdit\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#dce7f2")))},
            {QString::fromLatin1(kColorText), baselineTextColor},
            {QString::fromLatin1(kColorButtonBackground), extractColor(baselineStyleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#d8be74")))},
            {QString::fromLatin1(kColorButtonText), extractColor(baselineStyleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), baselineTextColor)},
            {QString::fromLatin1(kColorScrollBarBackground), extractColor(baselineStyleSheet, QStringLiteral("QScrollBar:vertical,\\s*QScrollBar:horizontal\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#22354a")))},
            {QString::fromLatin1(kColorScrollBarKnob), extractColor(baselineStyleSheet, QStringLiteral("QScrollBar::handle:vertical,\\s*QScrollBar::handle:horizontal\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#c8d8e8")))},
            {QString::fromLatin1(kColorBorder), extractColor(baselineStyleSheet, QStringLiteral("QToolTip\\s*\\{[^}]*border\\s*:\\s*1px solid\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#7690ac")))},
            {QString::fromLatin1(kColorAccentTop), extractColor(baselineStyleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#2d5ea6")))},
            {QString::fromLatin1(kColorAccentBottom), extractColor(baselineStyleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:1\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#1f447a")))},
            {QString::fromLatin1(kColorAccentText), baselineAccentTextColor},
            {QString::fromLatin1(kColorIconDefault), baselineAccentTextColor}
        };

        bool hasConfiguredColors = false;
        QMap<QString, QColor> restoredColors;
        for (const auto &entry : defaults) {
            const QColor storedColor = configuration->getComfortThemeColor(preset, entry.first, QColor());
            if (storedColor.isValid()) {
                restoredColors.insert(entry.first, storedColor);
                hasConfiguredColors = true;
            } else {
                restoredColors.insert(entry.first, entry.second);
            }
        }

        if (!hasConfiguredColors) {
            return false;
        }

        m_visualColors = restoredColors;
        return true;
    }

    void Comfort::persistVisualColorsToConfiguration() const {
        auto *configuration = m_settings ? m_settings->getConfiguration() : nullptr;
        if (!configuration) {
            return;
        }

        const QString preset = selectedPreset();
        for (auto it = m_visualColors.cbegin(); it != m_visualColors.cend(); ++it) {
            configuration->setComfortThemeColor(preset, it.key(), it.value());
        }
    }

    void Comfort::syncVisualControlsFromStyleSheet() {
        m_isSyncingVisualControls = true;

        const QString styleSheet = m_styleEditor ? m_styleEditor->toPlainText() : QString();
        m_visualColors.insert(QString::fromLatin1(kColorWindow), extractColor(styleSheet, QStringLiteral("QWidget\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#0f1622"))));
        m_visualColors.insert(QString::fromLatin1(kColorApplicationBackground), extractColor(styleSheet, QStringLiteral("QMainWindow[\\s\\S]*?QAbstractScrollArea\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#15283d"))));
        m_visualColors.insert(QString::fromLatin1(kColorPanel), extractColor(styleSheet, QStringLiteral("QMenuBar,\\s*QMenu,\\s*QStatusBar,\\s*QToolTip\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#1d3146"))));
        m_visualColors.insert(QString::fromLatin1(kColorBase), extractColor(styleSheet, QStringLiteral("QLineEdit[\\s\\S]*?QDateTimeEdit\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#dce7f2"))));
        m_visualColors.insert(QString::fromLatin1(kColorText), extractColor(styleSheet, QStringLiteral("QWidget\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#e9f3ff"))));
        m_visualColors.insert(QString::fromLatin1(kColorButtonBackground), extractColor(styleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#d8be74"))));
        m_visualColors.insert(QString::fromLatin1(kColorButtonText), extractColor(styleSheet, QStringLiteral("QToolButton,\\s*QPushButton\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), m_visualColors.value(QString::fromLatin1(kColorText))));
        m_visualColors.insert(QString::fromLatin1(kColorScrollBarBackground), extractColor(styleSheet, QStringLiteral("QScrollBar:vertical,\\s*QScrollBar:horizontal\\s*\\{[^}]*background\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#22354a"))));
        m_visualColors.insert(QString::fromLatin1(kColorScrollBarKnob), extractColor(styleSheet, QStringLiteral("QScrollBar::handle:vertical,\\s*QScrollBar::handle:horizontal\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#c8d8e8"))));
        m_visualColors.insert(QString::fromLatin1(kColorBorder), extractColor(styleSheet, QStringLiteral("QToolTip\\s*\\{[^}]*border\\s*:\\s*1px solid\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#7690ac"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentTop), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:0\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#2d5ea6"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentBottom), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*stop:1\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#1f447a"))));
        m_visualColors.insert(QString::fromLatin1(kColorAccentText), extractColor(styleSheet, QStringLiteral("QTabBar::tab:selected\\s*\\{[^}]*color\\s*:\\s*(#[0-9A-Fa-f]{6,8})"), QColor(QStringLiteral("#f7fbff"))));
        m_visualColors.insert(
            QString::fromLatin1(kColorIconDefault),
            m_settings && m_settings->getConfiguration()
                ? m_settings->getConfiguration()->getComfortThemeColor(
                    selectedPreset(),
                    QString::fromLatin1(kColorIconDefault),
                    m_visualColors.value(QString::fromLatin1(kColorAccentText), QColor(QStringLiteral("#f7fbff"))))
                : QColor(QStringLiteral("#f7fbff")));

        m_isSyncingVisualControls = false;
    }

    QString Comfort::buildVisualOverrideBlock() const {
        const QColor windowColor = colorValue(QString::fromLatin1(kColorWindow));
        const QColor applicationBackgroundColor = colorValue(QString::fromLatin1(kColorApplicationBackground));
        const QColor panelColor = colorValue(QString::fromLatin1(kColorPanel));
        const QColor baseColor = colorValue(QString::fromLatin1(kColorBase));
        const QColor textColor = colorValue(QString::fromLatin1(kColorText));
        const QColor buttonBackgroundColor = colorValue(QString::fromLatin1(kColorButtonBackground));
        const QColor buttonTextColor = colorValue(QString::fromLatin1(kColorButtonText));
        const QColor buttonTopColor = buttonBackgroundColor.lighter(114);
        const QColor buttonBottomColor = buttonBackgroundColor.darker(114);
        const QColor scrollBarBackgroundColor = colorValue(QString::fromLatin1(kColorScrollBarBackground));
        const QColor scrollBarKnobColor = colorValue(QString::fromLatin1(kColorScrollBarKnob));
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
                   "    background: %5;\n"
                   "    color: %2;\n"
                   "}\n"
                   "QToolTip {\n"
                   "    border: 1px solid %6;\n"
                   "}\n"
                   "QAbstractScrollArea {\n"
                   "    background: %4;\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 6px;\n"
                   "}\n"
                   "QScrollBar:vertical, QScrollBar:horizontal {\n"
                   "    background: %7;\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 8px;\n"
                   "}\n"
                   "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %8, stop:1 %9);\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 7px;\n"
                   "    min-height: 28px;\n"
                   "    min-width: 28px;\n"
                   "}\n"
                   "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page {\n"
                   "    background: transparent;\n"
                   "    border: none;\n"
                   "}\n"
                   "QLineEdit, QTextEdit, QPlainTextEdit, QListView, QListWidget, QTreeView, QTreeWidget, QTableView, QTableWidget,\n"
                   "QComboBox, QAbstractSpinBox, QDateTimeEdit {\n"
                   "    background: %10;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 6px;\n"
                   "    selection-background-color: %11;\n"
                   "    selection-color: %12;\n"
                   "}\n"
                   "QComboBox QAbstractItemView {\n"
                   "    background: %10;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %6;\n"
                   "    selection-background-color: %11;\n"
                   "    selection-color: %12;\n"
                   "}\n"
                   "QToolButton, QPushButton {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %13, stop:1 %14);\n"
                   "    color: %15;\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 6px;\n"
                   "}\n"
                   "QToolButton:hover, QPushButton:hover {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %16, stop:1 %17);\n"
                   "}\n"
                   "QToolButton:pressed, QPushButton:pressed, QToolButton:checked, QPushButton:checked {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %18, stop:1 %19);\n"
                   "    color: %12;\n"
                   "}\n"
                   "QTabWidget::pane {\n"
                   "    border: 1px solid %6;\n"
                   "    top: -1px;\n"
                   "}\n"
                   "QTabBar::tab {\n"
                   "    color: %15;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %13, stop:1 %14);\n"
                   "    border: 1px solid %6;\n"
                   "}\n"
                   "QTabBar::tab:selected {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %11, stop:1 %20);\n"
                   "    color: %12;\n"
                   "    border-color: %21;\n"
                   "}\n"
                   "QTabBar::tab:hover:!selected {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %16, stop:1 %17);\n"
                   "}\n"
                   "QCheckBox, QLabel, QGroupBox {\n"
                   "    color: %2;\n"
                   "}\n"
                   "QLabel {\n"
                   "    background: transparent;\n"
                   "}\n"
                   "QGroupBox {\n"
                   "    background: transparent;\n"
                   "    border: 1px solid %6;\n"
                   "}\n"
                   "QGroupBox::title {\n"
                   "    subcontrol-origin: margin;\n"
                   "    color: %11;\n"
                   "}\n"
                   "QCheckBox::indicator {\n"
                   "    border: 1px solid %6;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %13, stop:1 %14);\n"
                   "}\n"
                   "QCheckBox::indicator:checked {\n"
                   "    border-color: %21;\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %11, stop:1 %20);\n"
                   "}\n"
                   "QHeaderView::section, QTableCornerButton::section {\n"
                   "    background: %5;\n"
                   "    color: %2;\n"
                   "    border: 1px solid %6;\n"
                   "}\n"
                   "QProgressBar {\n"
                   "    border: 1px solid %6;\n"
                   "    border-radius: 6px;\n"
                   "    background: %5;\n"
                   "    color: %2;\n"
                   "    text-align: center;\n"
                   "}\n"
                   "QProgressBar::chunk {\n"
                   "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %11, stop:1 %20);\n"
                   "    border-radius: 5px;\n"
                   "}\n"
                   "%22\n")
            .arg(generatedOverrideStartMarker(),
                 textColor.name(),
                 windowColor.name(),
                 applicationBackgroundColor.name(),
                 panelColor.name(),
                 borderColor.name(),
                 scrollBarBackgroundColor.name(),
                 scrollBarKnobColor.lighter(108).name(),
                 scrollBarKnobColor.darker(108).name(),
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
