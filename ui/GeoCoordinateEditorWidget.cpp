#include "GeoCoordinateEditorWidget.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

#include "GeoCoordinateUtils.hpp"

namespace fairwindsk::ui {

    GeoCoordinateEditorWidget::GeoCoordinateEditorWidget(QWidget *parent) : QWidget(parent) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *formatLabel = new QLabel(tr("Format"), this);
        formatLabel->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(formatLabel);

        m_formatCombo = new QComboBox(this);
        m_formatCombo->setStyleSheet(
            "QComboBox { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_formatCombo->setMinimumWidth(170);
        for (const auto &option : geo::coordinateFormatOptions()) {
            m_formatCombo->addItem(option.label, option.id);
        }
        layout->addWidget(m_formatCombo);

        auto *latitudeLabel = new QLabel(tr("Latitude"), this);
        latitudeLabel->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(latitudeLabel);

        m_latitudeEdit = new QLineEdit(this);
        m_latitudeEdit->setStyleSheet(
            "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_latitudeEdit->setMinimumWidth(170);
        layout->addWidget(m_latitudeEdit, 1);

        auto *longitudeLabel = new QLabel(tr("Longitude"), this);
        longitudeLabel->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(longitudeLabel);

        m_longitudeEdit = new QLineEdit(this);
        m_longitudeEdit->setStyleSheet(
            "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_longitudeEdit->setMinimumWidth(190);
        layout->addWidget(m_longitudeEdit, 1);

        auto *altitudeLabel = new QLabel(tr("Altitude"), this);
        altitudeLabel->setStyleSheet("QLabel { color: white; }");
        layout->addWidget(altitudeLabel);

        m_altitudeEdit = new QLineEdit(this);
        m_altitudeEdit->setStyleSheet(
            "QLineEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_altitudeEdit->setMinimumWidth(100);
        layout->addWidget(m_altitudeEdit);

        connect(m_formatCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &GeoCoordinateEditorWidget::onFormatChanged);
    }

    void GeoCoordinateEditorWidget::setCoordinate(const double latitude,
                                                  const double longitude,
                                                  const double altitude,
                                                  const QString &formatId) {
        m_latitude = latitude;
        m_longitude = longitude;
        m_altitude = altitude;
        const int index = m_formatCombo->findData(geo::normalizeCoordinateFormatId(formatId));
        m_formatCombo->setCurrentIndex(index >= 0 ? index : 0);
        applyCurrentFormat();
    }

    QString GeoCoordinateEditorWidget::formatId() const {
        return m_formatCombo->currentData().toString();
    }

    bool GeoCoordinateEditorWidget::coordinate(double *latitude,
                                               double *longitude,
                                               double *altitude,
                                               QString *message) const {
        double parsedLatitude = 0.0;
        if (!geo::parseSingleCoordinate(m_latitudeEdit->text(), true, formatId(), &parsedLatitude, message)) {
            return false;
        }

        double parsedLongitude = 0.0;
        if (!geo::parseSingleCoordinate(m_longitudeEdit->text(), false, formatId(), &parsedLongitude, message)) {
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
            const double parsedAltitude = m_altitudeEdit->text().trimmed().toDouble(&ok);
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
        m_latitudeEdit->setText(geo::formatSingleCoordinate(m_latitude, true, formatId()));
        m_longitudeEdit->setText(geo::formatSingleCoordinate(m_longitude, false, formatId()));
        m_altitudeEdit->setText(QString::number(m_altitude, 'f', 2));
    }
}
