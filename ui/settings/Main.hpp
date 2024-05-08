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

        static void onVirtualKeyboard(int state);


    private:
        Ui::Main *ui;

        Settings *m_settings;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_MAIN_HPP
