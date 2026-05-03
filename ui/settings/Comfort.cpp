//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui/IconUtils.hpp"
#include "ui/widgets/TouchColorPicker.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchScrollArea.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        QStringList comfortPresetIds() {
            return {
                QStringLiteral("default"),
                QStringLiteral("dawn"),
                QStringLiteral("day"),
                QStringLiteral("sunset"),
                QStringLiteral("dusk"),
                QStringLiteral("night")
            };
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
                QStringLiteral("checkbox-selected"),
                QStringLiteral("window")
            };
        }

        QString normalizedPreset(QString preset) {
            preset = preset.trimmed().toLower();
            if (preset == QStringLiteral("sunrise")) {
                return QStringLiteral("dawn");
            }
            if (!comfortPresetIds().contains(preset)) {
                return QStringLiteral("default");
            }
            return preset;
        }

        void applyTextPalette(QWidget *widget, const QColor &color) {
            if (!widget) {
                return;
            }

            QPalette palette = widget->palette();
            palette.setColor(QPalette::WindowText, color);
            palette.setColor(QPalette::Text, color);
            widget->setPalette(palette);
        }

        // Color rows exposed to the user: (configuration key, display label).
        QList<QPair<QString, QString>> colorRows() {
            return {
                {QStringLiteral("window"),           QObject::tr("Window background")},
                {QStringLiteral("text"),             QObject::tr("Text")},
                {QStringLiteral("buttonBackground"), QObject::tr("Button background")},
                {QStringLiteral("buttonText"),       QObject::tr("Button text")},
                {QStringLiteral("border"),           QObject::tr("Border")},
                {QStringLiteral("accentTop"),        QObject::tr("Accent")},
                {QStringLiteral("accentText"),       QObject::tr("Accent text")},
                {QStringLiteral("iconDefault"),      QObject::tr("Icon")},
            };
        }
    }

    Comfort::Comfort(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
        buildUi();

        const QString currentPreset = normalizedPreset(m_settings->getConfiguration()->getComfortViewPreset());
        const int currentIndex = m_presetComboBox->findData(currentPreset);
        {
            const QSignalBlocker blocker(m_presetComboBox);
            m_isUpdating = true;
            m_presetComboBox->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
            m_isUpdating = false;
        }
        updateStatusLabel();
    }

    bool Comfort::event(QEvent *event) {
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            refreshChrome();
        }
        return QWidget::event(event);
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
        contentLayout->setContentsMargins(12, 12, 12, 12);
        contentLayout->setSpacing(12);

        m_titleLabel = new QLabel(tr("Comfort Theme Presets"), content);
        contentLayout->addWidget(m_titleLabel);

        m_hintLabel = new QLabel(
            tr("Select the active visibility preset for the helm display. Custom theme overrides remain supported in the configuration file, but this page keeps the underway controls compact."),
            content);
        m_hintLabel->setWordWrap(true);
        contentLayout->addWidget(m_hintLabel);

        // --- Control frame: preset selection and actions ---
        m_controlFrame = new QFrame(content);
        auto *controlLayout = new QVBoxLayout(m_controlFrame);
        controlLayout->setContentsMargins(12, 12, 12, 12);
        controlLayout->setSpacing(12);
        contentLayout->addWidget(m_controlFrame);

        auto *presetRow = new QHBoxLayout();
        presetRow->setContentsMargins(0, 0, 0, 0);
        presetRow->setSpacing(12);
        controlLayout->addLayout(presetRow);

        m_presetLabel = new QLabel(tr("Preset"), m_controlFrame);
        presetRow->addWidget(m_presetLabel);

        m_presetComboBox = new fairwindsk::ui::widgets::TouchComboBox(m_controlFrame);
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-default.svg")), tr("Default"), QStringLiteral("default"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-dawn.svg")), tr("Dawn"), QStringLiteral("dawn"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-day.svg")), tr("Day"), QStringLiteral("day"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-sunset.svg")), tr("Sunset"), QStringLiteral("sunset"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-dusk.svg")), tr("Dusk"), QStringLiteral("dusk"));
        m_presetComboBox->addItem(QIcon(QStringLiteral(":/resources/svg/OpenBridge/comfort-night.svg")), tr("Night"), QStringLiteral("night"));
        m_presetComboBox->setMinimumHeight(52);
        presetRow->addWidget(m_presetComboBox, 1);

        // Row 1: reset actions
        auto *resetRow = new QHBoxLayout();
        resetRow->setContentsMargins(0, 0, 0, 0);
        resetRow->setSpacing(8);
        controlLayout->addLayout(resetRow);

        m_resetCurrentButton = new QPushButton(tr("Reset Current"), m_controlFrame);
        m_resetAllButton = new QPushButton(tr("Reset All"), m_controlFrame);
        m_resetCurrentButton->setToolTip(tr("Clear custom colors, images, and QSS for the selected comfort preset"));
        m_resetAllButton->setToolTip(tr("Clear custom colors, images, and QSS for every comfort preset"));
        m_resetCurrentButton->setAccessibleName(tr("Reset current comfort preset"));
        m_resetAllButton->setAccessibleName(tr("Reset all comfort presets"));
        resetRow->addWidget(m_resetCurrentButton);
        resetRow->addWidget(m_resetAllButton);
        resetRow->addStretch(1);

        // Row 2: stylesheet import/export
        auto *qssRow = new QHBoxLayout();
        qssRow->setContentsMargins(0, 0, 0, 0);
        qssRow->setSpacing(8);
        controlLayout->addLayout(qssRow);

        m_exportQssButton = new QPushButton(tr("Export QSS"), m_controlFrame);
        m_exportQssButton->setToolTip(tr("Save the current preset stylesheet to a .qss file"));
        m_exportQssButton->setAccessibleName(tr("Export comfort stylesheet"));

        m_importQssButton = new QPushButton(tr("Import QSS"), m_controlFrame);
        m_importQssButton->setToolTip(tr("Load a .qss file as the current preset stylesheet"));
        m_importQssButton->setAccessibleName(tr("Import comfort stylesheet"));

        qssRow->addWidget(m_exportQssButton);
        qssRow->addWidget(m_importQssButton);
        qssRow->addStretch(1);

        // --- Colors section ---
        m_colorsGroupBox = new QGroupBox(tr("Colors"), content);
        auto *colorsFormLayout = new QFormLayout(m_colorsGroupBox);
        colorsFormLayout->setContentsMargins(0, 0, 0, 0);
        colorsFormLayout->setSpacing(8);
        colorsFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        contentLayout->addWidget(m_colorsGroupBox);

        for (const auto &row : colorRows()) {
            auto *swatchButton = new QPushButton(m_colorsGroupBox);
            swatchButton->setMinimumHeight(44);
            swatchButton->setMinimumWidth(100);
            swatchButton->setCursor(Qt::PointingHandCursor);
            swatchButton->setAccessibleName(tr("Edit %1 color").arg(row.second));
            m_colorSwatchButtons.insert(row.first, swatchButton);
            colorsFormLayout->addRow(row.second, swatchButton);

            const QString key = row.first;
            connect(swatchButton, &QPushButton::clicked, this, [this, key]() {
                editColor(key);
            });
        }

        // --- Background section ---
        m_backgroundGroupBox = new QGroupBox(tr("Background"), content);
        auto *backgroundLayout = new QVBoxLayout(m_backgroundGroupBox);
        backgroundLayout->setContentsMargins(0, 0, 0, 0);
        backgroundLayout->setSpacing(8);
        contentLayout->addWidget(m_backgroundGroupBox);

        auto *bgButtonRow = new QHBoxLayout();
        bgButtonRow->setContentsMargins(0, 0, 0, 0);
        bgButtonRow->setSpacing(8);
        backgroundLayout->addLayout(bgButtonRow);

        auto *bgLabel = new QLabel(tr("Main window"), m_backgroundGroupBox);
        bgButtonRow->addWidget(bgLabel);
        bgButtonRow->addStretch(1);

        m_selectBackgroundButton = new QPushButton(tr("Select…"), m_backgroundGroupBox);
        m_selectBackgroundButton->setMinimumHeight(44);
        m_selectBackgroundButton->setAccessibleName(tr("Select main window background image"));
        bgButtonRow->addWidget(m_selectBackgroundButton);

        m_clearBackgroundButton = new QPushButton(tr("Clear"), m_backgroundGroupBox);
        m_clearBackgroundButton->setMinimumHeight(44);
        m_clearBackgroundButton->setAccessibleName(tr("Clear main window background image"));
        bgButtonRow->addWidget(m_clearBackgroundButton);

        m_backgroundPathLabel = new QLabel(tr("No image selected"), m_backgroundGroupBox);
        m_backgroundPathLabel->setWordWrap(true);
        m_backgroundPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        backgroundLayout->addWidget(m_backgroundPathLabel);

        // --- Status label and padding ---
        m_statusLabel = new QLabel(content);
        m_statusLabel->setWordWrap(true);
        contentLayout->addWidget(m_statusLabel);
        contentLayout->addStretch(1);

        connect(m_presetComboBox,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Comfort::onPresetChanged);
        connect(m_resetCurrentButton, &QPushButton::clicked, this, &Comfort::resetCurrentPreset);
        connect(m_resetAllButton, &QPushButton::clicked, this, &Comfort::resetAllPresets);
        connect(m_exportQssButton, &QPushButton::clicked, this, &Comfort::exportQss);
        connect(m_importQssButton, &QPushButton::clicked, this, &Comfort::importQss);
        connect(m_selectBackgroundButton, &QPushButton::clicked, this, &Comfort::selectBackground);
        connect(m_clearBackgroundButton, &QPushButton::clicked, this, &Comfort::clearBackground);

        refreshChrome();
        refreshBackgroundPath();
    }

    void Comfort::onPresetChanged(const int) {
        if (m_isUpdating || !m_settings || !m_settings->getConfiguration()) {
            return;
        }

        m_settings->getConfiguration()->setComfortViewPreset(selectedPreset());
        updateStatusLabel();
        refreshColorSwatches();
        refreshBackgroundPath();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::resetCurrentPreset() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        const QString preset = selectedPreset();
        clearPresetCustomization(preset);
        refreshColorSwatches();
        refreshBackgroundPath();
        updateStatusLabel(tr("Preset reset."));
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::resetAllPresets() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        for (const QString &preset : comfortPresetIds()) {
            clearPresetCustomization(preset);
        }
        refreshColorSwatches();
        refreshBackgroundPath();
        updateStatusLabel(tr("All presets reset."));
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::exportQss() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        const QString preset = selectedPreset();

        // Prefer any custom stylesheet stored in the configuration; fall back to the
        // built-in resource so the user always gets a usable starting point.
        QString content = m_settings->getConfiguration()->getComfortThemeStyleSheet(preset);
        if (content.isEmpty()) {
            QFile builtIn(QStringLiteral(":/resources/stylesheets/%1.qss").arg(preset));
            if (builtIn.open(QIODevice::ReadOnly | QIODevice::Text)) {
                content = QString::fromUtf8(builtIn.readAll());
            }
        }

        const QString defaultPath = QDir(QDir::homePath()).filePath(
            QStringLiteral("%1.qss").arg(preset));
        const QString path = fairwindsk::ui::drawer::getSaveFilePath(
            this,
            tr("Export Comfort Stylesheet"),
            defaultPath,
            tr("Qt Stylesheets (*.qss);;All files (*)"));

        if (path.isEmpty()) {
            return;
        }

        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text) ||
            file.write(content.toUtf8()) < 0 ||
            !file.commit()) {
            fairwindsk::ui::drawer::warning(
                this, tr("Comfort"), tr("Unable to export the stylesheet."));
            return;
        }

        updateStatusLabel(tr("Stylesheet exported."));
    }

    void Comfort::importQss() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        const QString path = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Import Comfort Stylesheet"),
            QDir::homePath(),
            tr("Qt Stylesheets (*.qss);;All files (*)"));

        if (path.isEmpty()) {
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fairwindsk::ui::drawer::warning(
                this, tr("Comfort"), tr("Unable to read the selected stylesheet."));
            return;
        }

        const QString content = QString::fromUtf8(file.readAll());
        if (content.trimmed().isEmpty()) {
            fairwindsk::ui::drawer::warning(
                this, tr("Comfort"), tr("The selected stylesheet file is empty."));
            return;
        }

        m_settings->getConfiguration()->setComfortThemeStyleSheet(selectedPreset(), content);
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
        updateStatusLabel(tr("Stylesheet imported."));
    }

    void Comfort::selectBackground() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        const QString path = fairwindsk::ui::drawer::getOpenFilePath(
            this,
            tr("Select Background Image"),
            QDir::homePath(),
            tr("Images (*.png *.jpg *.jpeg *.webp *.bmp *.svg);;All files (*)"));

        if (path.isEmpty()) {
            return;
        }

        m_settings->getConfiguration()->setComfortBackgroundImagePath(
            selectedPreset(), QStringLiteral("window"), path);
        refreshBackgroundPath();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::clearBackground() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        m_settings->getConfiguration()->clearComfortBackgroundImagePath(
            selectedPreset(), QStringLiteral("window"));
        refreshBackgroundPath();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    QString Comfort::selectedPreset() const {
        return normalizedPreset(m_presetComboBox ? m_presetComboBox->currentData().toString() : QString());
    }

    void Comfort::refreshChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = m_settings ? m_settings->getConfiguration() : (fairWindSK ? fairWindSK->getConfiguration() : nullptr);
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);

        fairwindsk::ui::applySectionTitleLabelStyle(m_titleLabel, configuration, preset, palette(), 18.0);
        applyTextPalette(m_hintLabel, chrome.text);
        applyTextPalette(m_presetLabel, chrome.text);
        applyTextPalette(m_statusLabel, chrome.text);

        if (m_controlFrame) {
            m_controlFrame->setStyleSheet(QStringLiteral(
                "QFrame { border: 1px solid %1; border-radius: 14px; background: %2; }")
                                              .arg(chrome.border.name(),
                                                   fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 18).name(QColor::HexArgb)));
        }

        fairwindsk::ui::applyBottomBarPushButtonChrome(m_resetCurrentButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_resetAllButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_exportQssButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_importQssButton, chrome, false, 52);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_selectBackgroundButton, chrome, false, 44);
        fairwindsk::ui::applyBottomBarPushButtonChrome(m_clearBackgroundButton, chrome, false, 44);

        // Style both new group boxes uniformly: titled frame matching the control frame chrome.
        const QString groupStyle = QStringLiteral(
            "QGroupBox {"
            " border: 1px solid %1;"
            " border-radius: 10px;"
            " background: %2;"
            " margin-top: 14px;"
            " padding: 8px;"
            "}"
            "QGroupBox::title {"
            " color: %3;"
            " subcontrol-origin: margin;"
            " subcontrol-position: top left;"
            " padding: 2px 6px;"
            "}"
            "QGroupBox QLabel {"
            " color: %3;"
            " background: transparent;"
            "}")
            .arg(chrome.border.name(),
                 fairwindsk::ui::comfortAlpha(chrome.buttonBackground, 18).name(QColor::HexArgb),
                 chrome.text.name());

        if (m_colorsGroupBox) {
            m_colorsGroupBox->setStyleSheet(groupStyle);
        }
        if (m_backgroundGroupBox) {
            m_backgroundGroupBox->setStyleSheet(groupStyle);
        }

        refreshColorSwatches();
    }

    void Comfort::clearPresetCustomization(const QString &preset) const {
        auto *configuration = m_settings ? m_settings->getConfiguration() : nullptr;
        if (!configuration) {
            return;
        }

        const QString normalized = normalizedPreset(preset);
        configuration->clearComfortThemeStyleSheet(normalized);
        configuration->clearComfortThemeColors(normalized);
        for (const QString &area : comfortBackgroundAreas()) {
            configuration->clearComfortBackgroundImagePath(normalized, area);
        }
    }

    void Comfort::updateStatusLabel(const QString &message) {
        if (!m_statusLabel) {
            return;
        }

        const QString normalizedMessage = message.trimmed();
        m_statusLabel->setText(normalizedMessage.isEmpty()
                                   ? tr("Active preset: %1").arg(m_presetComboBox ? m_presetComboBox->currentText() : QString())
                                   : normalizedMessage);
    }

    void Comfort::editColor(const QString &key) {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        bool accepted = false;
        const QColor chosen = fairwindsk::ui::widgets::TouchColorPickerDialog::getColor(
            this,
            tr("Edit Color"),
            resolveInitialColor(key),
            &accepted,
            false);

        if (!accepted) {
            return;
        }

        m_settings->getConfiguration()->setComfortThemeColor(selectedPreset(), key, chosen);
        refreshColorSwatches();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    QColor Comfort::resolveInitialColor(const QString &key) const {
        const auto *configuration = m_settings ? m_settings->getConfiguration() : nullptr;
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(
            configuration, selectedPreset(), palette(), false);

        if (key == QLatin1String("window"))           return chrome.window;
        if (key == QLatin1String("text"))             return chrome.text;
        if (key == QLatin1String("buttonBackground")) return chrome.buttonBackground;
        if (key == QLatin1String("buttonText"))       return chrome.buttonText;
        if (key == QLatin1String("border"))           return chrome.border;
        if (key == QLatin1String("accentTop"))        return chrome.accentTop;
        if (key == QLatin1String("accentText"))       return chrome.accentText;
        if (key == QLatin1String("iconDefault"))      return chrome.icon;
        return palette().color(QPalette::Window);
    }

    void Comfort::refreshColorSwatches() {
        if (m_colorSwatchButtons.isEmpty()) {
            return;
        }

        const auto *configuration = m_settings ? m_settings->getConfiguration() : nullptr;
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(
            configuration, selectedPreset(), palette(), false);

        const QMap<QString, QColor> resolved = {
            {QStringLiteral("window"),           chrome.window},
            {QStringLiteral("text"),             chrome.text},
            {QStringLiteral("buttonBackground"), chrome.buttonBackground},
            {QStringLiteral("buttonText"),       chrome.buttonText},
            {QStringLiteral("border"),           chrome.border},
            {QStringLiteral("accentTop"),        chrome.accentTop},
            {QStringLiteral("accentText"),       chrome.accentText},
            {QStringLiteral("iconDefault"),      chrome.icon},
        };

        for (auto it = m_colorSwatchButtons.cbegin(); it != m_colorSwatchButtons.cend(); ++it) {
            QPushButton *btn = it.value();
            if (!btn) {
                continue;
            }

            const QColor color = resolved.value(it.key());
            if (!color.isValid()) {
                continue;
            }

            // Foreground: whichever of white/black contrasts better with the swatch.
            const QColor fg = fairwindsk::ui::bestContrastingColor(color, {Qt::white, Qt::black});
            btn->setText(color.name().toUpper());
            btn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                " background: %1;"
                " color: %2;"
                " border: 2px solid %3;"
                " border-radius: 6px;"
                " min-height: 44px;"
                " min-width: 100px;"
                " padding: 4px 12px;"
                "}"
                "QPushButton:hover { background: %4; }"
                "QPushButton:pressed { background: %5; border-width: 3px; }")
                .arg(color.name(),
                     fg.name(),
                     color.darker(130).name(),
                     color.lighter(115).name(),
                     color.darker(115).name()));
        }
    }

    void Comfort::refreshBackgroundPath() {
        if (!m_backgroundPathLabel || !m_settings) {
            return;
        }

        const QString path = m_settings->getConfiguration()->getComfortBackgroundImagePath(
            selectedPreset(), QStringLiteral("window"));

        if (path.isEmpty()) {
            m_backgroundPathLabel->setText(tr("No image selected"));
            if (m_clearBackgroundButton) {
                m_clearBackgroundButton->setEnabled(false);
            }
        } else {
            m_backgroundPathLabel->setText(QFileInfo(path).fileName());
            m_backgroundPathLabel->setToolTip(path);
            if (m_clearBackgroundButton) {
                m_clearBackgroundButton->setEnabled(true);
            }
        }
    }
}
