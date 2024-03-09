//
// Created by __author__ on 21/01/2022.
//

#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <QWidget>
#include <QToolButton>

#include <FairWindSK.hpp>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include "ui_Launcher.h"

namespace Ui { class Launcher; }

namespace fairwindsk::ui::launcher {

    class Launcher : public QWidget {
    Q_OBJECT

    public:
        explicit Launcher(QWidget *parent = nullptr);

        ~Launcher() override ;

        void resizeEvent(QResizeEvent *event) override;
        void toolButton_App_released();

    public slots:
        void onScrollLeft();
        void onScrollRight();

    signals:

        void foregroundAppChanged(const QString hash);

    private:
        void resize();

    private:
        Ui::Launcher *ui;

        int m_cols;
        int m_rows;
        int m_iconSize;
        QGridLayout *m_layout;
        QMap<QString, QToolButton *> m_buttons;
    };
} // fairwindsk::ui

#endif //LAUNCHER_HPP
