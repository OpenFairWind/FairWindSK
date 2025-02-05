//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_CONNECTION_HPP
#define FAIRWINDSK_CONNECTION_HPP

#include <QWidget>
#include <QTimer>
#include <QtZeroConf/qzeroconf.h>
#include "Settings.hpp"
#include "ui/web/Web.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Connection; }
    QT_END_NAMESPACE

    class Connection : public QWidget {
    Q_OBJECT

    public:
        explicit Connection(Settings *settingsWidget, QWidget *parent = nullptr);

        ~Connection() override;


    private slots:

        void onCheckRequestToken();
        void onRequestToken();
        void onCancelRequest();
        void onRemoveToken();

        void onUpdateSignalKServerUrl() const;

        void addService(const QZeroConfService& item) const;



    private:
        Ui::Connection *ui = nullptr;
        Settings *m_settings = nullptr;

        QZeroConf m_zeroConf;
        QTimer *m_timer = nullptr;
        QWebEnginePage *m_page = nullptr;

    };
} // fairwindsk::ui::settings

#endif //FAIRWINDSK_CONNECTION_HPP
