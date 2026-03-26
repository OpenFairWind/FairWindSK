//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_MAIN_HPP
#define FAIRWINDSK_MAIN_HPP

#include <QWidget>
#include "Settings.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Main; }
    QT_END_NAMESPACE

    class Main : public QWidget {
    Q_OBJECT

    public:
        explicit Main(Settings *settings, QWidget *parent = nullptr);

        ~Main() override;

    private slots:
        void onCurrentIndexChanged(int index);
        void onWindowModeChanged(int index);
        void onWindowWidthTextChanged();
        void onWindowHeightTextChanged();
        void onWindowLeftTextChanged();
        void onWindowTopTextChanged();
        void onVirtualKeyboardStateChanged(int state);
        void onUiScaleModeStateChanged(int state);
        void onUiScalePresetChanged(int index);
        void onLauncherRowsValueChanged(int value);
        void onLauncherColumnsValueChanged(int value);
        void onCoordinateFormatChanged(int index);

    private:
        void setWindowGeometryFieldsEnabled(const QString &windowMode) const;
        void setUiScaleFieldsEnabled(bool automatic) const;
        void applyUiPreview() const;

        Ui::Main *ui;

        Settings *m_settings;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_MAIN_HPP
