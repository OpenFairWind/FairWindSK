//
// Created by Codex on 21/03/26.
//

#include "ResourceDialog.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QUuid>

#include "FairWindSK.hpp"
#include "ui/DrawerDialogHost.hpp"

namespace {
    QString stringValue(const QJsonObject &resource, const QString &key, const QString &fallback = {}) {
        if (resource.contains(key) && resource[key].isString()) {
            return resource[key].toString();
        }

        if (resource.contains("feature") && resource["feature"].isObject()) {
            const auto properties = resource["feature"].toObject()["properties"].toObject();
            if (properties.contains(key) && properties[key].isString()) {
                return properties[key].toString();
            }
        }

        return fallback;
    }

    QJsonObject featureObject(const QJsonObject &resource) {
        return resource["feature"].toObject();
    }

    QJsonObject geometryObject(const QJsonObject &resource) {
        return featureObject(resource)["geometry"].toObject();
    }

    QJsonObject featurePropertiesObject(const QJsonObject &resource) {
        return featureObject(resource)["properties"].toObject();
    }

    QJsonArray coordinateArray(const QJsonObject &resource) {
        return geometryObject(resource)["coordinates"].toArray();
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
          m_geometryEdit(new QPlainTextEdit(this)),
          m_propertiesEdit(new QPlainTextEdit(this)),
          m_hrefEdit(new QLineEdit(this)),
          m_mimeTypeEdit(new QLineEdit(this)),
          m_notePositionCheckBox(new QCheckBox(tr("Include position"), this)),
          m_identifierEdit(new QLineEdit(this)),
          m_chartFormatEdit(new QLineEdit(this)),
          m_chartUrlEdit(new QLineEdit(this)),
          m_tilemapUrlEdit(new QLineEdit(this)),
          m_chartRegionEdit(new QLineEdit(this)),
          m_chartScaleSpinBox(new QDoubleSpinBox(this)),
          m_chartLayersEdit(new QLineEdit(this)),
          m_chartBoundsEdit(new QPlainTextEdit(this)),
          m_stackedWidget(new QStackedWidget(this)) {
        setWindowTitle(tr("%1 Editor").arg(resourceKindToSingularTitle(kind)));
        resize(760, 560);

        auto *layout = new QVBoxLayout(this);
        auto *formLayout = new QFormLayout();
        layout->addLayout(formLayout);

        formLayout->addRow(kind == ResourceKind::Note ? tr("Title") : tr("Name"), m_nameEdit);
        formLayout->addRow(tr("Description"), m_descriptionEdit);

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        m_chartScaleSpinBox->setRange(0.0, 1000000000.0);
        m_chartScaleSpinBox->setDecimals(0);

        QWidget *page = new QWidget(this);
        auto *pageLayout = new QFormLayout(page);
        m_propertiesEdit->setPlaceholderText("{\n  \"color\": \"red\"\n}");
        m_coordinatesEdit->setPlaceholderText(tr("41.9028, 12.4964\n41.9031, 12.4970"));
        m_geometryEdit->setPlaceholderText("{\n  \"type\": \"Polygon\",\n  \"coordinates\": [[[12.4, 41.9], [12.5, 41.9], [12.5, 42.0], [12.4, 41.9]]]\n}");
        m_mimeTypeEdit->setPlaceholderText("text/plain");
        m_chartLayersEdit->setPlaceholderText("base,depth");
        m_chartBoundsEdit->setPlaceholderText("[[12.0, 41.0], [13.0, 42.0]]");

        switch (kind) {
            case ResourceKind::Waypoint:
                pageLayout->addRow(tr("Type"), m_typeEdit);
                pageLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
                pageLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
                pageLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
                pageLayout->addRow(tr("Feature properties (JSON)"), m_propertiesEdit);
                break;
            case ResourceKind::Route:
                m_typeEdit->setPlaceholderText(tr("route"));
                pageLayout->addRow(tr("Type"), m_typeEdit);
                pageLayout->addRow(tr("Coordinates"), m_coordinatesEdit);
                pageLayout->addRow(tr("Feature properties (JSON)"), m_propertiesEdit);
                break;
            case ResourceKind::Region:
                pageLayout->addRow(tr("Geometry (JSON)"), m_geometryEdit);
                pageLayout->addRow(tr("Feature properties (JSON)"), m_propertiesEdit);
                break;
            case ResourceKind::Note:
                pageLayout->addRow(tr("Href"), m_hrefEdit);
                pageLayout->addRow(tr("MIME Type"), m_mimeTypeEdit);
                pageLayout->addRow(QString(), m_notePositionCheckBox);
                pageLayout->addRow(tr("Latitude"), m_latitudeSpinBox);
                pageLayout->addRow(tr("Longitude"), m_longitudeSpinBox);
                pageLayout->addRow(tr("Altitude"), m_altitudeSpinBox);
                pageLayout->addRow(tr("Properties (JSON)"), m_propertiesEdit);
                break;
            case ResourceKind::Chart:
                pageLayout->addRow(tr("Identifier"), m_identifierEdit);
                pageLayout->addRow(tr("Format"), m_chartFormatEdit);
                pageLayout->addRow(tr("Chart URL"), m_chartUrlEdit);
                pageLayout->addRow(tr("Tilemap URL"), m_tilemapUrlEdit);
                pageLayout->addRow(tr("Region"), m_chartRegionEdit);
                pageLayout->addRow(tr("Scale"), m_chartScaleSpinBox);
                pageLayout->addRow(tr("Layers"), m_chartLayersEdit);
                pageLayout->addRow(tr("Bounds (JSON)"), m_chartBoundsEdit);
                break;
        }

        m_stackedWidget->addWidget(page);
        layout->addWidget(m_stackedWidget);
        m_stackedWidget->setCurrentIndex(0);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &ResourceDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &ResourceDialog::reject);
        layout->addWidget(buttons);

        if (kind == ResourceKind::Route) {
            m_typeEdit->setText("route");
        } else if (kind == ResourceKind::Waypoint) {
            m_typeEdit->setText("generic");
        } else if (kind == ResourceKind::Note) {
            m_mimeTypeEdit->setText("text/plain");
        }
    }

    void ResourceDialog::setResource(const QString &id, const QJsonObject &resource) {
        m_id = id;
        m_nameEdit->setText(stringValue(resource, m_kind == ResourceKind::Note ? "title" : "name", stringValue(resource, "name")));
        m_descriptionEdit->setText(stringValue(resource, "description"));

        const auto properties = featurePropertiesObject(resource);
        if (!properties.isEmpty()) {
            m_propertiesEdit->setPlainText(QString::fromUtf8(QJsonDocument(properties).toJson(QJsonDocument::Indented)));
        }

        switch (m_kind) {
            case ResourceKind::Waypoint: {
                m_typeEdit->setText(stringValue(resource, "type"));
                const auto coordinates = coordinateArray(resource);
                if (coordinates.size() > 1) {
                    m_longitudeSpinBox->setValue(coordinates.at(0).toDouble());
                    m_latitudeSpinBox->setValue(coordinates.at(1).toDouble());
                }
                if (coordinates.size() > 2) {
                    m_altitudeSpinBox->setValue(coordinates.at(2).toDouble());
                }
                break;
            }
            case ResourceKind::Route: {
                m_typeEdit->setText(stringValue(resource, "type", "route"));
                QStringList lines;
                for (const auto &value : coordinateArray(resource)) {
                    const auto point = value.toArray();
                    if (point.size() >= 2) {
                        QString line = QString::number(point.at(1).toDouble(), 'f', 8) + ", " +
                                       QString::number(point.at(0).toDouble(), 'f', 8);
                        if (point.size() > 2) {
                            line += ", " + QString::number(point.at(2).toDouble(), 'f', 2);
                        }
                        lines.append(line);
                    }
                }
                m_coordinatesEdit->setPlainText(lines.join("\n"));
                break;
            }
            case ResourceKind::Region: {
                const auto geometry = geometryObject(resource);
                m_geometryEdit->setPlainText(QString::fromUtf8(QJsonDocument(geometry).toJson(QJsonDocument::Indented)));
                break;
            }
            case ResourceKind::Note: {
                m_hrefEdit->setText(resource["href"].toString());
                m_mimeTypeEdit->setText(resource["mimeType"].toString());
                const auto position = resource["position"].toObject();
                const bool hasPosition = !position.isEmpty();
                m_notePositionCheckBox->setChecked(hasPosition);
                if (hasPosition) {
                    m_latitudeSpinBox->setValue(position["latitude"].toDouble());
                    m_longitudeSpinBox->setValue(position["longitude"].toDouble());
                    if (position.contains("altitude")) {
                        m_altitudeSpinBox->setValue(position["altitude"].toDouble());
                    }
                }
                break;
            }
            case ResourceKind::Chart: {
                m_identifierEdit->setText(resource["identifier"].toString());
                m_chartFormatEdit->setText(resource["chartFormat"].toString());
                m_chartUrlEdit->setText(resource["chartUrl"].toString());
                m_tilemapUrlEdit->setText(resource["tilemapUrl"].toString());
                m_chartRegionEdit->setText(resource["region"].toString());
                m_chartScaleSpinBox->setValue(resource["scale"].toDouble());
                if (resource.contains("chartLayers") && resource["chartLayers"].isArray()) {
                    QStringList layers;
                    for (const auto &layer : resource["chartLayers"].toArray()) {
                        layers.append(layer.toString());
                    }
                    m_chartLayersEdit->setText(layers.join(", "));
                }
                if (resource.contains("bounds") && resource["bounds"].isArray()) {
                    m_chartBoundsEdit->setPlainText(QString::fromUtf8(QJsonDocument(resource["bounds"].toArray()).toJson(QJsonDocument::Indented)));
                }
                break;
            }
        }
    }

    QString ResourceDialog::resourceId() const {
        return m_id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_id;
    }

    QJsonArray ResourceDialog::coordinatesJson() const {
        QJsonArray coordinates;
        const QStringList lines = m_coordinatesEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
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

    QJsonObject ResourceDialog::parseJsonObject(const QPlainTextEdit *edit) const {
        const QByteArray text = edit->toPlainText().trimmed().toUtf8();
        if (text.isEmpty()) {
            return {};
        }

        const auto document = QJsonDocument::fromJson(text);
        return document.isObject() ? document.object() : QJsonObject{};
    }

    bool ResourceDialog::validate(QString *message) const {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            *message = tr("%1 is required.").arg(m_kind == ResourceKind::Note ? tr("Title") : tr("Name"));
            return false;
        }

        switch (m_kind) {
            case ResourceKind::Waypoint:
                return true;
            case ResourceKind::Route:
                if (coordinatesJson().size() < 2) {
                    *message = tr("Routes require at least two coordinate points.");
                    return false;
                }
                return true;
            case ResourceKind::Region: {
                const auto geometry = QJsonDocument::fromJson(m_geometryEdit->toPlainText().trimmed().toUtf8());
                if (!geometry.isObject() || !geometry.object().contains("type") || !geometry.object().contains("coordinates")) {
                    *message = tr("Region geometry must be a JSON object with type and coordinates.");
                    return false;
                }
                return true;
            }
            case ResourceKind::Note:
                return true;
            case ResourceKind::Chart:
                if (m_identifierEdit->text().trimmed().isEmpty()) {
                    *message = tr("Chart identifier is required.");
                    return false;
                }
                if (m_chartFormatEdit->text().trimmed().isEmpty()) {
                    *message = tr("Chart format is required.");
                    return false;
                }
                return true;
        }

        return true;
    }

    QJsonObject ResourceDialog::resourceObject() const {
        const QString id = resourceId();
        const QString name = m_nameEdit->text().trimmed();
        const QString description = m_descriptionEdit->text().trimmed();
        const QJsonObject featureProperties = parseJsonObject(m_propertiesEdit);

        QJsonObject resource;
        resource["timestamp"] = fairwindsk::signalk::Client::currentISO8601TimeUTC();

        switch (m_kind) {
            case ResourceKind::Waypoint: {
                QJsonArray coordinates;
                coordinates.append(m_longitudeSpinBox->value());
                coordinates.append(m_latitudeSpinBox->value());
                coordinates.append(m_altitudeSpinBox->value());

                QJsonObject geometry;
                geometry["type"] = "Point";
                geometry["coordinates"] = coordinates;

                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;
                if (!featureProperties.isEmpty()) {
                    feature["properties"] = featureProperties;
                }

                resource["name"] = name;
                resource["description"] = description;
                resource["type"] = m_typeEdit->text().trimmed();
                resource["feature"] = feature;
                break;
            }
            case ResourceKind::Route: {
                QJsonObject geometry;
                geometry["type"] = "LineString";
                geometry["coordinates"] = coordinatesJson();

                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;
                if (!featureProperties.isEmpty()) {
                    feature["properties"] = featureProperties;
                }

                resource["name"] = name;
                resource["description"] = description;
                resource["type"] = m_typeEdit->text().trimmed();
                resource["feature"] = feature;
                break;
            }
            case ResourceKind::Region: {
                const auto geometry = QJsonDocument::fromJson(m_geometryEdit->toPlainText().trimmed().toUtf8()).object();

                QJsonObject feature;
                feature["id"] = id;
                feature["type"] = "Feature";
                feature["geometry"] = geometry;

                QJsonObject properties = featureProperties;
                properties["name"] = name;
                properties["description"] = description;
                feature["properties"] = properties;

                resource["name"] = name;
                resource["description"] = description;
                resource["feature"] = feature;
                break;
            }
            case ResourceKind::Note: {
                resource["title"] = name;
                resource["description"] = description;
                resource["href"] = m_hrefEdit->text().trimmed();
                resource["mimeType"] = m_mimeTypeEdit->text().trimmed();
                if (!featureProperties.isEmpty()) {
                    resource["properties"] = featureProperties;
                }
                if (m_notePositionCheckBox->isChecked()) {
                    QJsonObject position;
                    position["latitude"] = m_latitudeSpinBox->value();
                    position["longitude"] = m_longitudeSpinBox->value();
                    position["altitude"] = m_altitudeSpinBox->value();
                    resource["position"] = position;
                }
                break;
            }
            case ResourceKind::Chart: {
                resource["name"] = name;
                resource["description"] = description;
                resource["identifier"] = m_identifierEdit->text().trimmed();
                resource["chartFormat"] = m_chartFormatEdit->text().trimmed();
                resource["chartUrl"] = m_chartUrlEdit->text().trimmed();
                resource["tilemapUrl"] = m_tilemapUrlEdit->text().trimmed();
                resource["region"] = m_chartRegionEdit->text().trimmed();
                if (m_chartScaleSpinBox->value() > 0.0) {
                    resource["scale"] = m_chartScaleSpinBox->value();
                }
                const QStringList layers = m_chartLayersEdit->text().split(',', Qt::SkipEmptyParts);
                if (!layers.isEmpty()) {
                    QJsonArray chartLayers;
                    for (const auto &layer : layers) {
                        chartLayers.append(layer.trimmed());
                    }
                    resource["chartLayers"] = chartLayers;
                }
                const QByteArray boundsText = m_chartBoundsEdit->toPlainText().trimmed().toUtf8();
                if (!boundsText.isEmpty()) {
                    const auto boundsDocument = QJsonDocument::fromJson(boundsText);
                    if (boundsDocument.isArray()) {
                        resource["bounds"] = boundsDocument.array();
                    }
                }
                break;
            }
        }

        return resource;
    }

    void ResourceDialog::accept() {
        QString message;
        if (!validate(&message)) {
            drawer::warning(this, tr("Invalid %1").arg(resourceKindToSingularTitle(m_kind)), message);
            return;
        }

        QDialog::accept();
    }
}
