#include "GeoCoordinateEditorWidget.hpp"
#include <QGuiApplication>
#include <QInputMethod>
#include <QTimer>
#include "GeoCoordinateUtils.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui_GeoCoordinateEditorWidget.h"

namespace fairwindsk::ui {

    GeoCoordinateEditorWidget::GeoCoordinateEditorWidget(QWidget *parent)
        : QWidget(parent),
          ui(new ::Ui::GeoCoordinateEditorWidget) {
        ui->setupUi(this);

        for (const auto &option : geo::coordinateFormatOptions()) {
            ui->comboBoxFormat->addItem(option.label, option.id);
        }

        connect(ui->comboBoxFormat,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &GeoCoordinateEditorWidget::onFormatChanged);
        // AGGIUNTA PER IL FIX DEFINITIVO DELLA TASTIERA ANDROID
        // Creiamo una funzione (lambda) che forza la chiusura della tastiera a livello di OS
        // e rimuove il focus in modo "asincrono" per non far arrabbiare Android.
        // FIX RADICALE: Intercettiamo l'evento di focus in uscita su tutti i campi di input
        auto forceCloseKeyboard = [](QWidget *widget) {
            if (widget) {
                widget->clearFocus();
            }
            if (auto *inputMethod = QGuiApplication::inputMethod()) {
                inputMethod->hide();
            }
        };

        // Quando clicchi fuori o premi invio su qualsiasi casella delle coordinate
        connect(ui->lineEditLatitude, &QLineEdit::editingFinished, this, [=]() { forceCloseKeyboard(ui->lineEditLatitude); });
        connect(ui->lineEditLongitude, &QLineEdit::editingFinished, this, [=]() { forceCloseKeyboard(ui->lineEditLongitude); });
        connect(ui->lineEditAltitude, &QLineEdit::editingFinished, this, [=]() { forceCloseKeyboard(ui->lineEditAltitude); });
    }
    GeoCoordinateEditorWidget::~GeoCoordinateEditorWidget() {
        delete ui;
    }

    void GeoCoordinateEditorWidget::setCoordinate(const double latitude,
                                                  const double longitude,
                                                  const double altitude,
                                                  const QString &formatId) {
        m_latitude = latitude;
        m_longitude = longitude;
        m_altitude = altitude;
        const int index = ui->comboBoxFormat->findData(geo::normalizeCoordinateFormatId(formatId));
        ui->comboBoxFormat->setCurrentIndex(index >= 0 ? index : 0);
        applyCurrentFormat();
    }

    QString GeoCoordinateEditorWidget::formatId() const {
        return ui->comboBoxFormat->currentData().toString();
    }

    QPushButton *GeoCoordinateEditorWidget::applyButton() const {
        return ui->pushButtonApply;
    }

    QPushButton *GeoCoordinateEditorWidget::cancelButton() const {
        return ui->pushButtonCancel;
    }

    bool GeoCoordinateEditorWidget::coordinate(double *latitude,
                                               double *longitude,
                                               double *altitude,
                                               QString *message) const {
        double parsedLatitude = 0.0;
        if (!geo::parseSingleCoordinate(ui->lineEditLatitude->text(), true, formatId(), &parsedLatitude, message)) {
            return false;
        }

        double parsedLongitude = 0.0;
        if (!geo::parseSingleCoordinate(ui->lineEditLongitude->text(), false, formatId(), &parsedLongitude, message)) {
            return false;
        }

        if (latitude) {
            *latitude = parsedLatitude;
        }
        if (longitude) {
            *longitude = parsedLongitude;
        }
        if (altitude) {
            bool ok = false;
            const double parsedAltitude = ui->lineEditAltitude->text().trimmed().toDouble(&ok);
            if (!ok) {
                if (message) {
                    *message = tr("Altitude format is invalid.");
                }
                return false;
            }
            *altitude = parsedAltitude;
        }
        return true;
    }

    void GeoCoordinateEditorWidget::onFormatChanged(const int) {
        applyCurrentFormat();
    }

    void GeoCoordinateEditorWidget::applyCurrentFormat() {
        ui->lineEditLatitude->setText(geo::formatSingleCoordinate(m_latitude, true, formatId()));
        ui->lineEditLongitude->setText(geo::formatSingleCoordinate(m_longitude, false, formatId()));
        ui->lineEditAltitude->setText(QString::number(m_altitude, 'f', 2));
    }
}
