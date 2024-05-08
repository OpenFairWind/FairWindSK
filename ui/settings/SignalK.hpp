//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_SIGNALK_HPP
#define FAIRWINDSK_SIGNALK_HPP

#include <QWidget>
#include "Settings.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class SignalK; }
    QT_END_NAMESPACE

    class SignalK : public QWidget {
    Q_OBJECT

    public:
        explicit SignalK(Settings *settings, QWidget *parent = nullptr);

        ~SignalK() override;

    private:
        Ui::SignalK *ui;

        Settings *m_settings;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_SIGNALK_HPP
