//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP
#define FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>

#include "ResourceModel.hpp"

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
        QString coordinatesText() const;
        QJsonArray coordinatesJson() const;
        bool validate(QString *message) const;

        ResourceKind m_kind;
        QString m_id;
        QLineEdit *m_nameEdit;
        QLineEdit *m_descriptionEdit;
        QLineEdit *m_typeEdit;
        QDoubleSpinBox *m_latitudeSpinBox;
        QDoubleSpinBox *m_longitudeSpinBox;
        QDoubleSpinBox *m_altitudeSpinBox;
        QPlainTextEdit *m_coordinatesEdit;
        QStackedWidget *m_stackedWidget;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_RESOURCEDIALOG_HPP
