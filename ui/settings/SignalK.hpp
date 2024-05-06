//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_SIGNALK_HPP
#define FAIRWINDSK_SIGNALK_HPP

#include <QWidget>

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class SignalK; }
    QT_END_NAMESPACE

    class SignalK : public QWidget {
    Q_OBJECT

    public:
        explicit SignalK(QWidget *parent = nullptr);

        ~SignalK() override;

    private:
        Ui::SignalK *ui;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_SIGNALK_HPP
