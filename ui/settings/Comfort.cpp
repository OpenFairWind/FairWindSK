//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/IconUtils.hpp"
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
                QStringLiteral("checkbox-selected")
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

        auto *buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 0, 0, 0);
        buttonRow->setSpacing(8);
        controlLayout->addLayout(buttonRow);

        m_resetCurrentButton = new QPushButton(tr("Reset Current"), m_controlFrame);
        m_resetAllButton = new QPushButton(tr("Reset All"), m_controlFrame);
        m_resetCurrentButton->setToolTip(tr("Clear custom colors, images, and QSS for the selected comfort preset"));
        m_resetAllButton->setToolTip(tr("Clear custom colors, images, and QSS for every comfort preset"));
        m_resetCurrentButton->setAccessibleName(tr("Reset current comfort preset"));
        m_resetAllButton->setAccessibleName(tr("Reset all comfort presets"));
        buttonRow->addWidget(m_resetCurrentButton);
        buttonRow->addWidget(m_resetAllButton);
        buttonRow->addStretch(1);

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

        refreshChrome();
    }

    void Comfort::onPresetChanged(const int) {
        if (m_isUpdating || !m_settings || !m_settings->getConfiguration()) {
            return;
        }

        m_settings->getConfiguration()->setComfortViewPreset(selectedPreset());
        updateStatusLabel();
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Comfort::resetCurrentPreset() {
        if (!m_settings || !m_settings->getConfiguration()) {
            return;
        }

        const QString preset = selectedPreset();
        clearPresetCustomization(preset);
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
        updateStatusLabel(tr("All presets reset."));
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
}
