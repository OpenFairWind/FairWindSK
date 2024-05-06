//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_MAIN_HPP
#define FAIRWINDSK_MAIN_HPP

#include <QWidget>

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Main; }
    QT_END_NAMESPACE

    class Main : public QWidget {
    Q_OBJECT

    public:
        explicit Main(QWidget *parent = nullptr);

        ~Main() override;

    private slots:

        void onVirtualKeyboard(int state);


    private:
        Ui::Main *ui;
    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_MAIN_HPP
