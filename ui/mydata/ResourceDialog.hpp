//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>

#include "ResourceModel.hpp"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;

namespace fairwindsk::ui::mydata {

    class ResourceDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ResourceDialog(ResourceKind kind, QWidget *parent = nullptr);

        void setResource(const QString &id, const QJsonObject &resource);
        QString resourceId() const;
        QJsonObject resourceObject() const;

    public slots:
        void accept() override;

    private:
        QJsonArray coordinatesJson() const;
        QJsonObject parseJsonObject(const QPlainTextEdit *edit) const;
        bool validate(QString *message) const;

        ResourceKind m_kind;
        QString m_id;
        QLineEdit *m_nameEdit = nullptr;
        QLineEdit *m_descriptionEdit = nullptr;
        QLineEdit *m_typeEdit = nullptr;
        QDoubleSpinBox *m_latitudeSpinBox = nullptr;
        QDoubleSpinBox *m_longitudeSpinBox = nullptr;
        QDoubleSpinBox *m_altitudeSpinBox = nullptr;
        QPlainTextEdit *m_coordinatesEdit = nullptr;
        QPlainTextEdit *m_geometryEdit = nullptr;
        QPlainTextEdit *m_propertiesEdit = nullptr;
        QLineEdit *m_hrefEdit = nullptr;
        QLineEdit *m_mimeTypeEdit = nullptr;
        QCheckBox *m_notePositionCheckBox = nullptr;
        QLineEdit *m_identifierEdit = nullptr;
        QLineEdit *m_chartFormatEdit = nullptr;
        QLineEdit *m_chartUrlEdit = nullptr;
        QLineEdit *m_tilemapUrlEdit = nullptr;
        QLineEdit *m_chartRegionEdit = nullptr;
        QDoubleSpinBox *m_chartScaleSpinBox = nullptr;
        QLineEdit *m_chartLayersEdit = nullptr;
        QPlainTextEdit *m_chartBoundsEdit = nullptr;
        QStackedWidget *m_stackedWidget = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP
