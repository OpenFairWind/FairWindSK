//
// Created by Raffaele Montella on 08/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Colophon.h" resolved


#include <QDialogButtonBox>
#include <QDate>
#include "About.hpp"
#include <ui/MainWindow.hpp>

namespace fairwindsk::ui::about {
    About::About(QWidget *parent,  QWidget *currenWidget) :
            QWidget(parent), ui(new Ui::About) {
        ui->setupUi(this);
        m_currentWidget = currenWidget;
        ui->labelVersion->setText(tr("FairWindSK Version %1").arg(QStringLiteral(FAIRWINDSK_VERSION)));
        ui->label->setText(tr("Copyright %1").arg(QStringLiteral(FAIRWINDSK_BUILD_YEAR)));
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &About::onClose);
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &About::onClose);
    }

    About::~About() {
        delete ui;
    }

    void About::onClose() {
        emit closed(this);
    }

    QWidget *About::getCurrentWidget() {
        return m_currentWidget;
    }

    void About::setCurrentWidget(QWidget *currentWidget) {
        m_currentWidget = currentWidget;
    }
} // fairwindsk::ui::colophon
