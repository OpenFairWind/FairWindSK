//
// Created by Codex on 21/03/26.
//

#include "ResourceDialog.hpp"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QUuid>

#include "FairWindSK.hpp"

namespace {
    QJsonArray geometryCoordinates(const QJsonObject &resource) {
        return resource["feature"].toObject()["geometry"].toObject()["coordinates"].toArray();
    }

    QString topLevelValue(const QJsonObject &resource, const QString &key) {
        if (resource.contains(key) && resource[key].isString()) {
            return resource[key].toString();
        }

        return resource["feature"].toObject()["properties"].toObject()[key].toString();
    }
}

namespace fairwindsk::ui::mydata {
    ResourceDialog::ResourceDialog(const ResourceKind kind, QWidget *parent)
        : QDialog(parent),
          m_kind(kind),
          m_nameEdit(new QLineEdit(this)),
          m_descriptionEdit(new QLineEdit(this)),
          m_typeEdit(new QLineEdit(this)),
          m_latitudeSpinBox(new QDoubleSpinBox(this)),
          m_longitudeSpinBox(new QDoubleSpinBox(this)),
          m_altitudeSpinBox(new QDoubleSpinBox(this)),
          m_coordinatesEdit(new QPlainTextEdit(this)),
          m_stackedWidget(new QStackedWidget(this)) {
        setWindowTitle(tr("%1 Editor").arg(resourceKindToTitle(kind)));
        resize(640, 420);

        auto *layout = new QVBoxLayout(this);
        auto *formLayout = new QFormLayout();
        layout->addLayout(formLayout);

        formLayout->addRow(tr("Name"), m_nameEdit);
        formLayout->addRow(tr("Description"), m_descriptionEdit);
        formLayout->addRow(tr("Type"), m_typeEdit);

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);

        auto *waypointPage = new QWidget(this);
        auto *waypointForm = new QFormLayout(waypointPage);
        waypointForm->addRow(tr("Latitude"), m_latitudeSpinBox);
        waypointForm->addRow(tr("Longitude"), m_longitudeSpinBox);
        waypointForm->addRow(tr("Altitude"), m_altitudeSpinBox);

        auto *pathPage = new QWidget(this);
        auto *pathLayout = new QVBoxLayout(pathPage);
        pathLayout->addWidget(new QLabel(tr("Coordinates, one point per line: latitude, longitude[, altitude]"), pathPage));
        m_coordinatesEdit->setPlaceholderText(tr("41.9028, 12.4964\n41.9031, 12.4970"));
        pathLayout->addWidget(m_coordinatesEdit);

        m_stackedWidget->addWidget(waypointPage);
        m_stackedWidget->addWidget(pathPage);
        m_stackedWidget->setCurrentIndex(kind == ResourceKind::Waypoint ? 0 : 1);
        layout->addWidget(m_stackedWidget);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &ResourceDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &ResourceDialog::reject);
        layout->addWidget(buttons);

        switch (kind) {
            case ResourceKind::Waypoint:
                m_typeEdit->setText("generic");
                break;
            case ResourceKind::Route:
                m_typeEdit->setText("route");
                break;
            case ResourceKind::Track:
                m_typeEdit->setText("track");
                break;
        }
    }

    void ResourceDialog::setResource(const QString &id, const QJsonObject &resource) {
        m_id = id;
        m_nameEdit->setText(topLevelValue(resource, "name"));
        m_descriptionEdit->setText(topLevelValue(resource, "description"));
        m_typeEdit->setText(topLevelValue(resource, "type"));

        const QJsonArray coordinates = geometryCoordinates(resource);
        if (m_kind == ResourceKind::Waypoint) {
            if (coordinates.size() > 1) {
                m_latitudeSpinBox->setValue(coordinates.at(1).toDouble());
                m_longitudeSpinBox->setValue(coordinates.at(0).toDouble());
            }
            if (coordinates.size() > 2) {
                m_altitudeSpinBox->setValue(coordinates.at(2).toDouble());
            }
        } else {
            QStringList lines;
            for (const auto &value : coordinates) {
                const QJsonArray point = value.toArray();
                if (point.size() > 1) {
                    QString line = QString::number(point.at(1).toDouble(), 'f', 8) + ", " +
                                   QString::number(point.at(0).toDouble(), 'f', 8);
                    if (point.size() > 2) {
                        line += ", " + QString::number(point.at(2).toDouble(), 'f', 2);
                    }
                    lines.append(line);
                }
            }
            m_coordinatesEdit->setPlainText(lines.join("\n"));
        }
    }

    QString ResourceDialog::resourceId() const {
        return m_id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_id;
    }

    QString ResourceDialog::coordinatesText() const {
        return m_coordinatesEdit->toPlainText().trimmed();
    }

    QJsonArray ResourceDialog::coordinatesJson() const {
        QJsonArray coordinates;

        if (m_kind == ResourceKind::Waypoint) {
            coordinates.append(m_longitudeSpinBox->value());
            coordinates.append(m_latitudeSpinBox->value());
            coordinates.append(m_altitudeSpinBox->value());
            return coordinates;
        }

        const QStringList lines = coordinatesText().split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(',', Qt::SkipEmptyParts);
            if (parts.size() < 2) {
                continue;
            }

            QJsonArray point;
            point.append(parts.at(1).trimmed().toDouble());
            point.append(parts.at(0).trimmed().toDouble());
            if (parts.size() > 2) {
                point.append(parts.at(2).trimmed().toDouble());
            }
            coordinates.append(point);
        }

        return coordinates;
    }

    bool ResourceDialog::validate(QString *message) const {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            *message = tr("Name is required.");
            return false;
        }

        if (m_kind == ResourceKind::Waypoint) {
            return true;
        }

        if (coordinatesJson().size() < 2) {
            *message = tr("Routes and tracks require at least two coordinate points.");
            return false;
        }

        return true;
    }

    QJsonObject ResourceDialog::resourceObject() const {
        const QString id = resourceId();
        const QString name = m_nameEdit->text().trimmed();
        const QString description = m_descriptionEdit->text().trimmed();
        const QString type = m_typeEdit->text().trimmed();
        const QJsonArray coordinates = coordinatesJson();

        QJsonObject geometry;
        geometry["type"] = m_kind == ResourceKind::Waypoint ? "Point" : "LineString";
        geometry["coordinates"] = coordinates;

        QJsonObject properties;
        properties["name"] = name;
        properties["description"] = description;
        properties["type"] = type;

        QJsonObject feature;
        feature["id"] = id;
        feature["type"] = "Feature";
        feature["properties"] = properties;
        feature["geometry"] = geometry;

        QJsonObject resource;
        resource["name"] = name;
        resource["description"] = description;
        resource["type"] = type;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();
        resource["feature"] = feature;

        if (m_kind == ResourceKind::Waypoint) {
            QJsonObject position;
            position["latitude"] = m_latitudeSpinBox->value();
            position["longitude"] = m_longitudeSpinBox->value();
            position["altitude"] = m_altitudeSpinBox->value();
            resource["position"] = position;
        }

        return resource;
    }

    void ResourceDialog::accept() {
        QString message;
        if (!validate(&message)) {
            QMessageBox::warning(this, tr("Invalid Resource"), message);
            return;
        }

        QDialog::accept();
    }
}
