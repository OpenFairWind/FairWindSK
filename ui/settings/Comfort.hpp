//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
#define FAIRWINDSK_UI_SETTINGS_COMFORT_HPP

#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace fairwindsk::ui::widgets {
    class TouchComboBox;
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
        void loadPresetEditor();
        void updateStatusLabel();

        Settings *m_settings = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_presetComboBox = nullptr;
        QPlainTextEdit *m_styleEditor = nullptr;
        QLabel *m_statusLabel = nullptr;
        QPushButton *m_importButton = nullptr;
        QPushButton *m_exportButton = nullptr;
        QPushButton *m_resetButton = nullptr;
        bool m_isUpdating = false;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
