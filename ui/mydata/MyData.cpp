//
// Created by Raffaele Montella on 07/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MyData.h" resolved

#include <QtWidgets/QDialogButtonBox>
#include "MyData.hpp"
#include "ui_MyData.h"

namespace fairwindsk::ui::mydata {
    MyData::MyData(QWidget *parent, QWidget *currenWidget) :
            QWidget(parent), ui(new Ui::MyData) {
        ui->setupUi(this);

        m_currentWidget = currenWidget;

        connect(ui->buttonBox,&QDialogButtonBox::clicked,this,&MyData::onClose);
    }

    MyData::~MyData() {
        delete ui;
    }

    void MyData::onClose() {
        setVisible(false);
        emit closed(this);
    }

    QWidget *MyData::getCurrentWidget() {
        return m_currentWidget;
    }
} // fairwindsk::ui::mydata
