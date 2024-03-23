//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QtZeroConf/qzeroconf.h>

namespace Ui { class Settings; }

namespace fairwindsk::ui {

    class Settings : public QWidget {
    Q_OBJECT

    public:
        explicit Settings(QWidget *parent = nullptr);

        ~Settings() override;

    private slots:

        void onVirtualKeyboard(int state);

        void onConnect();
        void onStop();
        void onRestart();

        void addService(const QZeroConfService& item);

    private:
        Ui::Settings *ui;
        bool m_stop;
        QZeroConf m_zeroConf;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
