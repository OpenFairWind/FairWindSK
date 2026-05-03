//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
#define FAIRWINDSK_UI_SETTINGS_COMFORT_HPP

#include <QString>
#include <QWidget>

class QEvent;
class QLabel;
class QPushButton;
class QFrame;

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

    protected:
        bool event(QEvent *event) override;

    private slots:
        void onPresetChanged(int index);
        void resetCurrentPreset();
        void resetAllPresets();

    private:
        QString selectedPreset() const;
        void buildUi();
        void refreshChrome();
        void clearPresetCustomization(const QString &preset) const;
        void updateStatusLabel(const QString &message = QString());

        Settings *m_settings = nullptr;
        fairwindsk::ui::widgets::TouchComboBox *m_presetComboBox = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_hintLabel = nullptr;
        QLabel *m_presetLabel = nullptr;
        QLabel *m_statusLabel = nullptr;
        QFrame *m_controlFrame = nullptr;
        QPushButton *m_resetCurrentButton = nullptr;
        QPushButton *m_resetAllButton = nullptr;
        bool m_isUpdating = false;
    };
}

#endif // FAIRWINDSK_UI_SETTINGS_COMFORT_HPP
