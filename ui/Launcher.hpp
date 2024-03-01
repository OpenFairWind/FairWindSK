//
// Created by __author__ on 21/01/2022.
//

#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <QWidget>
#include <QToolButton>
#include "ui_Launcher.h"
#include <FairWindSK.hpp>

namespace fairwindsk::ui {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Launcher; }
    QT_END_NAMESPACE

    class Launcher : public QWidget {
    Q_OBJECT

    public:
        explicit Launcher(QWidget *parent = nullptr);

        ~Launcher() ;

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
        int mCols;
        int mRows;
        QMap<QString, QToolButton *> mButtons;
    };
} // fairwindsk::ui

#endif //LAUNCHER_HPP
