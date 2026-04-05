//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QFile>
#include <QFileInfo>
#include <QFrame>
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
#include <QSlider>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QPointer>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/launcher/Launcher.hpp"
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

        class DrawerColorPickerWidget final : public QWidget {
        public:
            explicit DrawerColorPickerWidget(const QString &label, const QColor &initialColor, QWidget *parent = nullptr)
                : QWidget(parent) {
                auto *rootLayout = new QVBoxLayout(this);
                rootLayout->setContentsMargins(0, 0, 0, 0);
                rootLayout->setSpacing(10);

                auto *titleLabel = new QLabel(tr("Adjust %1 using touch-friendly controls.").arg(label), this);
                titleLabel->setWordWrap(true);
                rootLayout->addWidget(titleLabel);

                auto *headerLayout = new QHBoxLayout();
                headerLayout->setContentsMargins(0, 0, 0, 0);
                headerLayout->setSpacing(12);
                rootLayout->addLayout(headerLayout);

                m_preview = new QFrame(this);
                m_preview->setMinimumSize(96, 72);
                m_preview->setFrameShape(QFrame::StyledPanel);
                headerLayout->addWidget(m_preview, 0);

                auto *valueLayout = new QVBoxLayout();
                valueLayout->setContentsMargins(0, 0, 0, 0);
                valueLayout->setSpacing(4);
                headerLayout->addLayout(valueLayout, 1);

                m_hexLabel = new QLabel(this);
                valueLayout->addWidget(m_hexLabel);

                m_rgbaLabel = new QLabel(this);
                valueLayout->addWidget(m_rgbaLabel);
                valueLayout->addStretch(1);

                auto *swatchesLayout = new QHBoxLayout();
                swatchesLayout->setContentsMargins(0, 0, 0, 0);
                swatchesLayout->setSpacing(8);
                rootLayout->addLayout(swatchesLayout);

                const QList<QColor> swatches = {
                    QColor(QStringLiteral("#f7f2d9")),
                    QColor(QStringLiteral("#fbf6e3")),
                    QColor(QStringLiteral("#fffdf3")),
                    QColor(QStringLiteral("#2d5ea6")),
                    QColor(QStringLiteral("#1f447a")),
                    QColor(QStringLiteral("#10233a")),
                    QColor(QStringLiteral("#6f7683")),
                    QColor(QStringLiteral("#d05b3f"))
                };
                for (const QColor &swatch : swatches) {
                    auto *button = new QToolButton(this);
                    button->setAutoRaise(false);
                    button->setMinimumSize(52, 52);
                    button->setStyleSheet(QStringLiteral(
                                              "QToolButton {"
                                              " border: 1px solid %1;"
                                              " border-radius: 8px;"
                                              " background: %2;"
                                              " }")
                                              .arg(swatch.darker(135).name(), swatch.name(QColor::HexArgb)));
                    connect(button, &QToolButton::clicked, this, [this, swatch]() {
                        setColor(swatch);
                    });
                    swatchesLayout->addWidget(button);
                }
                swatchesLayout->addStretch(1);

                addSlider(rootLayout, tr("Red"), &m_redSlider);
                addSlider(rootLayout, tr("Green"), &m_greenSlider);
                addSlider(rootLayout, tr("Blue"), &m_blueSlider);
                addSlider(rootLayout, tr("Opacity"), &m_alphaSlider);

                setMinimumHeight(360);
                setColor(initialColor.isValid() ? initialColor : QColor(QStringLiteral("#ffffff")));
            }

            QColor color() const {
                return m_color;
            }

        private:
            void addSlider(QVBoxLayout *layout, const QString &label, QSlider **sliderPtr) {
                auto *rowLayout = new QHBoxLayout();
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(10);

                auto *textLabel = new QLabel(label, this);
                textLabel->setMinimumWidth(70);
                rowLayout->addWidget(textLabel);

                auto *slider = new QSlider(Qt::Horizontal, this);
                slider->setRange(0, 255);
                slider->setPageStep(16);
                slider->setSingleStep(1);
                slider->setMinimumHeight(44);
                rowLayout->addWidget(slider, 1);

                auto *valueLabel = new QLabel(this);
                valueLabel->setMinimumWidth(36);
                rowLayout->addWidget(valueLabel);

                layout->addLayout(rowLayout);

                m_valueLabels.insert(slider, valueLabel);
                connect(slider, &QSlider::valueChanged, this, [this]() {
                    syncFromSliders();
                });
                *sliderPtr = slider;
            }

            void setColor(const QColor &color) {
                const QSignalBlocker redBlocker(m_redSlider);
                const QSignalBlocker greenBlocker(m_greenSlider);
                const QSignalBlocker blueBlocker(m_blueSlider);
                const QSignalBlocker alphaBlocker(m_alphaSlider);
                m_redSlider->setValue(color.red());
                m_greenSlider->setValue(color.green());
                m_blueSlider->setValue(color.blue());
                m_alphaSlider->setValue(color.alpha());
                m_color = color;
                updatePreview();
            }

            void syncFromSliders() {
                m_color = QColor(
                    m_redSlider->value(),
                    m_greenSlider->value(),
                    m_blueSlider->value(),
                    m_alphaSlider->value());
                updatePreview();
            }

            void updatePreview() {
                m_preview->setStyleSheet(QStringLiteral(
                                             "QFrame {"
                                             " border: 1px solid %1;"
                                             " border-radius: 10px;"
                                             " background: %2;"
                                             " }")
                                             .arg(m_color.darker(150).name(), m_color.name(QColor::HexArgb)));
                m_hexLabel->setText(tr("HEX: %1").arg(m_color.name(QColor::HexArgb).toUpper()));
                m_rgbaLabel->setText(tr("RGBA: %1, %2, %3, %4")
                                         .arg(m_color.red())
                                         .arg(m_color.green())
                                         .arg(m_color.blue())
                                         .arg(m_color.alpha()));

                for (auto it = m_valueLabels.cbegin(); it != m_valueLabels.cend(); ++it) {
                    it.value()->setText(QString::number(it.key()->value()));
                }
            }

            QColor m_color;
            QFrame *m_preview = nullptr;
            QLabel *m_hexLabel = nullptr;
            QLabel *m_rgbaLabel = nullptr;
            QSlider *m_redSlider = nullptr;
            QSlider *m_greenSlider = nullptr;
            QSlider *m_blueSlider = nullptr;
            QSlider *m_alphaSlider = nullptr;
            QMap<QSlider *, QLabel *> m_valueLabels;
        };
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

        m_visualEditorWidget = new QWidget(content);
        auto *visualLayout = new QGridLayout(m_visualEditorWidget);
        visualLayout->setContentsMargins(0, 0, 0, 0);
        visualLayout->setHorizontalSpacing(12);
        visualLayout->setVerticalSpacing(8);
        createColorControl(QString::fromLatin1(kColorWindow), tr("Window"), m_visualEditorWidget, visualLayout, 0);
        createColorControl(QString::fromLatin1(kColorPanel), tr("Panel"), m_visualEditorWidget, visualLayout, 1);
        createColorControl(QString::fromLatin1(kColorBase), tr("Fields"), m_visualEditorWidget, visualLayout, 2);
        createColorControl(QString::fromLatin1(kColorText), tr("Text"), m_visualEditorWidget, visualLayout, 3);
        createColorControl(QString::fromLatin1(kColorButtonText), tr("Button text"), m_visualEditorWidget, visualLayout, 4);
        createColorControl(QString::fromLatin1(kColorButtonTop), tr("Button top"), m_visualEditorWidget, visualLayout, 5);
        createColorControl(QString::fromLatin1(kColorButtonBottom), tr("Button bottom"), m_visualEditorWidget, visualLayout, 6);
        createColorControl(QString::fromLatin1(kColorBorder), tr("Borders"), m_visualEditorWidget, visualLayout, 7);
        createColorControl(QString::fromLatin1(kColorAccentTop), tr("Accent top"), m_visualEditorWidget, visualLayout, 8);
        createColorControl(QString::fromLatin1(kColorAccentBottom), tr("Accent bottom"), m_visualEditorWidget, visualLayout, 9);
        createColorControl(QString::fromLatin1(kColorAccentText), tr("Accent text"), m_visualEditorWidget, visualLayout, 10);
        contentLayout->addWidget(m_visualEditorWidget);

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

        m_previewLauncher = new fairwindsk::ui::launcher::Launcher(previewGroup);
        m_previewLauncher->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_previewLauncher->setMinimumHeight(260);
        m_previewLauncher->refreshFromConfiguration(true);
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
        auto *picker = new DrawerColorPickerWidget(colorLabel(key), colorValue(key));
        QPointer<DrawerColorPickerWidget> pickerGuard(picker);
        const int result = fairwindsk::ui::drawer::execDrawer(
            this,
            tr("Choose %1").arg(colorLabel(key)),
            picker,
            {
                {tr("Apply"), int(QMessageBox::Apply), true},
                {tr("Cancel"), int(QMessageBox::Cancel), false}
            },
            int(QMessageBox::Cancel));
        if (!pickerGuard || result != QMessageBox::Apply) {
            return;
        }

        m_visualColors.insert(key, pickerGuard->color());
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
