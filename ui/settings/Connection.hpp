//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_CONNECTION_HPP
#define FAIRWINDSK_CONNECTION_HPP

#include <QWidget>
#include <QtZeroConf/qzeroconf.h>

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Connection; }
    QT_END_NAMESPACE

    class Connection : public QWidget {
    Q_OBJECT

    public:
        explicit Connection(QWidget *parent = nullptr);

        ~Connection() override;


    private slots:



        void onConnect();
        void onStop();
        void onRestart();

        void addService(const QZeroConfService& item);



    private:
        Ui::Connection *ui;

        bool m_stop;
        QZeroConf m_zeroConf;

    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_CONNECTION_HPP
