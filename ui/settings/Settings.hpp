//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QtZeroConf/qzeroconf.h>

//namespace Ui { class Settings; }

namespace fairwindsk::ui::settings {

    QT_BEGIN_NAMESPACE
    namespace Ui { class Settings; }
    QT_END_NAMESPACE


    class Settings : public QWidget {
    Q_OBJECT

    public:
        explicit Settings(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~Settings() override;

        QWidget *getCurrentWidget();

    public slots:
        void onAccepted();

    signals:
        void accepted(Settings *);

    private:
        Ui::Settings *ui;


        QWidget *m_currentWidget;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
