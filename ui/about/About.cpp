//
// Created by Raffaele Montella on 08/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Colophon.h" resolved

#include <ui/MainWindow.hpp>
#include <QDialogButtonBox>
#include "About.hpp"
#include "ui_About.h"

namespace fairwindsk::ui::about {
    About::About(QWidget *parent,  QWidget *currenWidget) :
            QWidget(parent), ui(new Ui::About) {
        ui->setupUi(this);
        m_currentWidget = currenWidget;
        connect(ui->buttonBox,&QDialogButtonBox::accepted,this,&About::onAccepted);
    }

    About::~About() {
        delete ui;
    }

    void About::onAccepted() {
        emit accepted(this);
    }

    QWidget *About::getCurrentWidget() {
        return m_currentWidget;
    }
} // fairwindsk::ui::colophon
