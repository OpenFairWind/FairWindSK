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
        void updateScrollButtons() const;

    private:
        Ui::Launcher *ui = nullptr;

        int m_cols = 0;
        int m_rows = 0;
        int m_iconSize = 0;
        int m_stableViewportHeight = 0;
        QGridLayout *m_layout = nullptr;
        QMap<QString, QToolButton *> m_buttons;
    };
} // fairwindsk::ui

#endif //LAUNCHER_HPP
