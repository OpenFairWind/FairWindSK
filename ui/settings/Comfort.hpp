//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
#define FAIRWINDSK_UI_SETTINGS_COMFORT_HPP

#include <QColor>
#include <QMap>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QGridLayout;
class QGroupBox;

namespace fairwindsk::ui::widgets {
    class TouchComboBox;
}

namespace fairwindsk::ui::launcher {
    class Launcher;
}

namespace fairwindsk::ui::settings {
    class Settings;

    class Comfort : public QWidget {
        Q_OBJECT

    public:
        explicit Comfort(Settings *settings, QWidget *parent = nullptr);
        ~Comfort() override = default;

    private slots:
        void onPresetChanged(int index);
        void onStyleSheetChanged();
        void importStyleSheet();
        void exportStyleSheet();
        void resetPreset();

    private:
        QString selectedPreset() const;
        QString defaultStyleSheetForPreset(const QString &preset) const;
        QString effectiveStyleSheetForPreset(const QString &preset) const;
        QString effectiveBackgroundStyleSheetForPreset(const QString &preset) const;
        void buildUi();
        void createColorControl(const QString &key, const QString &labelText, QWidget *parent, QGridLayout *layout, int row);
        void createBackgroundImageControl(const QString &area, const QString &labelText, QWidget *parent, QGridLayout *layout, int row);
        void loadPresetEditor();
        void updateStatusLabel();
        void updatePreview();
        void updateColorButtons();
        void updateBackgroundImageLabels();
        QString colorLabel(const QString &key) const;
        void pickColor(const QString &key);
        void browseBackgroundImage(const QString &area);
        void clearBackgroundImage(const QString &area);
        void applyVisualThemeOverride();
        void syncVisualControlsFromStyleSheet();
        QString buildVisualOverrideBlock() const;
        QString styleSheetWithVisualOverride() const;
        QColor colorValue(const QString &key) const;
        QColor extractColor(const QString &styleSheet, const QString &pattern, const QColor &fallback) const;
        QString extractCapturedValue(const QString &styleSheet, const QString &pattern, const QString &fallback = QString()) const;
        static QString generatedOverrideStartMarker();
        static QString generatedOverrideEndMarker();
        static QString removeGeneratedOverrideBlock(QString styleSheet);

        Settings *m_settings = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_presetComboBox = nullptr;
        QPlainTextEdit *m_styleEditor = nullptr;
        QLabel *m_statusLabel = nullptr;
        QPushButton *m_importButton = nullptr;
        QPushButton *m_exportButton = nullptr;
        QPushButton *m_editorButton = nullptr;
        QPushButton *m_resetButton = nullptr;
        QMap<QString, QColor> m_visualColors;
        QMap<QString, QPushButton *> m_colorButtons;
        QMap<QString, QLabel *> m_backgroundPathLabels;
        QWidget *m_visualEditorWidget = nullptr;
        QWidget *m_previewTopBar = nullptr;
        fairwindsk::ui::launcher::Launcher *m_previewLauncher = nullptr;
        QWidget *m_previewBottomBar = nullptr;
        QTabWidget *m_previewTabs = nullptr;
        QGroupBox *m_advancedGroup = nullptr;
        bool m_isUpdating = false;
        bool m_isSyncingVisualControls = false;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
