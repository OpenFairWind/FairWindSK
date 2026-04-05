//
// Created by Codex on 05/04/26.
//

#include "Comfort.hpp"

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextStream>
#include <QVBoxLayout>

#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchScrollArea.hpp"

namespace fairwindsk::ui::settings {
    namespace {
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
    }

    Comfort::Comfort(Settings *settings, QWidget *parent)
        : QWidget(parent),
          m_settings(settings) {
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

        m_styleEditor = new QPlainTextEdit(content);
        m_styleEditor->setPlaceholderText(tr("Edit the QSS for the selected comfort preset."));
        m_styleEditor->setMinimumHeight(320);
        contentLayout->addWidget(m_styleEditor, 1);

        connect(m_presetComboBox,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Comfort::onPresetChanged);
        connect(m_styleEditor, &QPlainTextEdit::textChanged, this, &Comfort::onStyleSheetChanged);
        connect(m_importButton, &QPushButton::clicked, this, &Comfort::importStyleSheet);
        connect(m_exportButton, &QPushButton::clicked, this, &Comfort::exportStyleSheet);
        connect(m_resetButton, &QPushButton::clicked, this, &Comfort::resetPreset);

        const QString currentPreset = normalizedPreset(m_settings->getConfiguration()->getComfortViewPreset());
        const int currentIndex = m_presetComboBox->findData(currentPreset);
        m_presetComboBox->setCurrentIndex(currentIndex >= 0 ? currentIndex : 1);
        loadPresetEditor();
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

    void Comfort::loadPresetEditor() {
        if (!m_styleEditor) {
            return;
        }

        const QSignalBlocker blocker(m_styleEditor);
        m_isUpdating = true;
        m_styleEditor->setPlainText(effectiveStyleSheetForPreset(selectedPreset()));
        m_isUpdating = false;
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
                ? tr("Editing the saved override for the %1 preset. Changes are stored in fairwindsk.json and applied live.")
                      .arg(preset)
                : tr("Editing the default %1 preset. Any changes you make here will be saved as an override in fairwindsk.json.")
                      .arg(preset));
    }
}
