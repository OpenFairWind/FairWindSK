//
// Created by Raffaele Montella on 14/01/22.
//

#ifndef FAIRWINDSK_WEB_HPP
#define FAIRWINDSK_WEB_HPP

#include <QWidget>

#include <FairWindSK.hpp>
#include "ui_Web.h"

#include "WebView.hpp"

namespace Ui { class Web; }

namespace fairwindsk::ui::web {


    class Web : public QWidget {
    Q_OBJECT

    public:
        explicit Web(QWidget *parent = nullptr);

        //void onAdded() override;

        ~Web() override;

    public slots:
        void toolButton_home_clicked();

    private:
        Ui::Web *ui;
        WebView *m_webView = nullptr;
        QString m_url;
    };
} // fairwindsk::ui::web

#endif //FAIRWIND_MAINPAGE_HPP
