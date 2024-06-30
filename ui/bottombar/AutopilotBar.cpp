//
// Created by Raffaele Montella on 04/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Autopilot.h" resolved

#include <QtWidgets/QAbstractButton>
#include <QSlider>
#include "AutopilotBar.hpp"
#include "ui_AutopilotBar.h"

namespace fairwindsk::ui::bottombar {
    AutopilotBar::AutopilotBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AutopilotBar) {
        ui->setupUi(this);


        int rudderMin = -30;
        int rudderMax = 30;
        int rudderStep = 5;
        int columnSpan = 1+(abs(rudderMin) + abs(rudderMax))/rudderStep;

        auto *slider = new QSlider(Qt::Horizontal, ui->widget_Rudder);
        slider->setRange(rudderMin, rudderMax);
        slider->setTickInterval(rudderStep);
        slider->setEnabled(false);
        slider->setTickPosition(QSlider::TicksBelow);


        auto *layout = new QGridLayout();
        layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(slider, 0, 0, 1, columnSpan);
        int col=0;
        for (auto i = rudderMin; i<=rudderMax;i=i+rudderStep) {
            auto s = QString::number(i);
            if (i>0) s = "+" + s;
            auto *labelTick = new QLabel( s,  ui->widget_Rudder);
            /*
            if (i % 2) {
                labelTick->setStyleSheet("background-color:blue");
            }
             */
            if (i<0) {
                labelTick->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            } else if (i>0) {
                labelTick->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            } else {
                labelTick->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            }
            layout->addWidget(labelTick, 1, col, 1, 1);
            col++;
        }

        ui->widget_Rudder->setLayout( layout);


        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QToolButton::clicked, this, &AutopilotBar::onHideClicked);
    }

    void AutopilotBar::onHideClicked() {
        setVisible(false);
        emit hide();
    }

    AutopilotBar::~AutopilotBar() {
        delete ui;
    }
} // fairwindsk::ui::autopilot
