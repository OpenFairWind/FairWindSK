//
// Created by Codex on 21/03/26.
//

#include "ResourceDialog.hpp"

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QUuid>

#include "FairWindSK.hpp"
#include "ui/DrawerDialogHost.hpp"
#include "ui_ResourceDialog.h"

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
          ui(new Ui::ResourceDialog) {
        ui->setupUi(this);
        setWindowTitle(tr("%1 Editor").arg(resourceKindToSingularTitle(kind)));
        m_nameEdit = ui->lineEditName;
        m_descriptionEdit = ui->lineEditDescription;
        m_typeEdit = ui->lineEditType;
        m_latitudeSpinBox = ui->doubleSpinBoxLatitude;
        m_longitudeSpinBox = ui->doubleSpinBoxLongitude;
        m_altitudeSpinBox = ui->doubleSpinBoxAltitude;
        m_coordinatesEdit = ui->plainTextEditCoordinates;
        m_geometryEdit = ui->plainTextEditGeometry;
        m_propertiesEdit = ui->plainTextEditProperties;
        m_hrefEdit = ui->lineEditHref;
        m_mimeTypeEdit = ui->lineEditMimeType;
        m_notePositionCheckBox = ui->checkBoxNotePosition;
        m_identifierEdit = ui->lineEditIdentifier;
        m_chartFormatEdit = ui->lineEditChartFormat;
        m_chartUrlEdit = ui->lineEditChartUrl;
        m_tilemapUrlEdit = ui->lineEditTilemapUrl;
        m_chartRegionEdit = ui->lineEditChartRegion;
        m_chartScaleSpinBox = ui->doubleSpinBoxChartScale;
        m_chartLayersEdit = ui->lineEditChartLayers;
        m_chartBoundsEdit = ui->plainTextEditChartBounds;
        m_stackedWidget = ui->stackedWidgetPages;

        ui->labelName->setText(kind == ResourceKind::Note ? tr("Title") : tr("Name"));

        m_latitudeSpinBox->setRange(-90.0, 90.0);
        m_latitudeSpinBox->setDecimals(8);
        m_longitudeSpinBox->setRange(-180.0, 180.0);
        m_longitudeSpinBox->setDecimals(8);
        m_altitudeSpinBox->setRange(-100000.0, 100000.0);
        m_altitudeSpinBox->setDecimals(2);
        ui->doubleSpinBoxNoteLatitude->setRange(-90.0, 90.0);
        ui->doubleSpinBoxNoteLatitude->setDecimals(8);
        ui->doubleSpinBoxNoteLongitude->setRange(-180.0, 180.0);
        ui->doubleSpinBoxNoteLongitude->setDecimals(8);
        ui->doubleSpinBoxNoteAltitude->setRange(-100000.0, 100000.0);
        ui->doubleSpinBoxNoteAltitude->setDecimals(2);
        m_chartScaleSpinBox->setRange(0.0, 1000000000.0);
        m_chartScaleSpinBox->setDecimals(0);

        m_propertiesEdit->setPlaceholderText("{\n  \"color\": \"red\"\n}");
        m_coordinatesEdit->setPlaceholderText(tr("41.9028, 12.4964\n41.9031, 12.4970"));
        m_geometryEdit->setPlaceholderText("{\n  \"type\": \"Polygon\",\n  \"coordinates\": [[[12.4, 41.9], [12.5, 41.9], [12.5, 42.0], [12.4, 41.9]]]\n}");
        m_mimeTypeEdit->setPlaceholderText("text/plain");
        m_chartLayersEdit->setPlaceholderText("base,depth");
        m_chartBoundsEdit->setPlaceholderText("[[12.0, 41.0], [13.0, 42.0]]");

        const QList<QLineEdit *> lineEdits = findChildren<QLineEdit *>();
        for (QLineEdit *lineEdit : lineEdits) {
            lineEdit->setMinimumHeight(44);
        }

        const QList<QDoubleSpinBox *> spinBoxes = findChildren<QDoubleSpinBox *>();
        for (QDoubleSpinBox *spinBox : spinBoxes) {
            spinBox->setMinimumHeight(44);
        }

        const QList<QPlainTextEdit *> plainTextEdits = findChildren<QPlainTextEdit *>();
        for (QPlainTextEdit *plainTextEdit : plainTextEdits) {
            plainTextEdit->setMinimumHeight(108);
        }

        const QList<QCheckBox *> checkBoxes = findChildren<QCheckBox *>();
        for (QCheckBox *checkBox : checkBoxes) {
            checkBox->setMinimumHeight(40);
        }

        const QList<QLabel *> labels = findChildren<QLabel *>();
        for (QLabel *label : labels) {
            QFont font = label->font();
            font.setPointSizeF(std::max(11.0, font.pointSizeF()));
            label->setFont(font);
        }

        if (ui->buttonBox) {
            const auto buttons = ui->buttonBox->buttons();
            for (QAbstractButton *button : buttons) {
                button->setMinimumHeight(46);
                button->setMinimumWidth(110);
            }
        }

        switch (kind) {
            case ResourceKind::Waypoint:
                m_stackedWidget->setCurrentWidget(ui->pageWaypoint);
                break;
            case ResourceKind::Route:
                m_typeEdit = ui->lineEditRouteType;
                m_propertiesEdit = ui->plainTextEditRouteProperties;
                m_coordinatesEdit = ui->plainTextEditCoordinates;
                m_typeEdit->setPlaceholderText(tr("route"));
                m_stackedWidget->setCurrentWidget(ui->pageRoute);
                break;
            case ResourceKind::Region:
                m_propertiesEdit = ui->plainTextEditRegionProperties;
                m_stackedWidget->setCurrentWidget(ui->pageRegion);
                break;
            case ResourceKind::Note:
                m_latitudeSpinBox = ui->doubleSpinBoxNoteLatitude;
                m_longitudeSpinBox = ui->doubleSpinBoxNoteLongitude;
                m_altitudeSpinBox = ui->doubleSpinBoxNoteAltitude;
                m_propertiesEdit = ui->plainTextEditNoteProperties;
                m_stackedWidget->setCurrentWidget(ui->pageNote);
                break;
            case ResourceKind::Chart:
                m_stackedWidget->setCurrentWidget(ui->pageChart);
                break;
        }

        if (kind == ResourceKind::Route) {
            m_typeEdit->setText("route");
        } else if (kind == ResourceKind::Waypoint) {
            m_typeEdit->setText("generic");
        } else if (kind == ResourceKind::Note) {
            m_mimeTypeEdit->setText("text/plain");
        }
    }

    ResourceDialog::~ResourceDialog() {
        delete ui;
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
