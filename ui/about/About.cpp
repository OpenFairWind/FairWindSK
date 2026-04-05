//
// Created by Raffaele Montella on 08/01/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Colophon.h" resolved


#include <QDialogButtonBox>
#include <QDate>
#include <QResizeEvent>
#include "About.hpp"
#include <ui/MainWindow.hpp>

namespace fairwindsk::ui::about {
    About::About(QWidget *parent,  QWidget *currenWidget) :
            QWidget(parent), ui(new Ui::About) {
        ui->setupUi(this);
        m_currentWidget = currenWidget;
        m_logoPixmap = QPixmap(QStringLiteral(":/resources/images/other/splash_logo.png"));
        ui->labelVersion->setText(tr("FairWindSK Version %1").arg(QStringLiteral(FAIRWINDSK_VERSION)));
        ui->label->setText(tr("Copyright %1").arg(QStringLiteral(FAIRWINDSK_BUILD_YEAR)));
        ui->buttonBox->hide();
        ui->scrollArea->setBorderless(true);
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &About::onClose);
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &About::onClose);
        updateLogoPixmap();
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

    void About::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        updateLogoPixmap();
    }

    void About::updateLogoPixmap() {
        if (!ui || !ui->label_3 || m_logoPixmap.isNull()) {
            return;
        }

        const QSize availableSize = ui->label_3->contentsRect().size();
        if (!availableSize.isValid()) {
            return;
        }

        ui->label_3->setPixmap(
            m_logoPixmap.scaled(
                availableSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }
} // fairwindsk::ui::colophon
