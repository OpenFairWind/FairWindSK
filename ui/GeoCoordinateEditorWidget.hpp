#ifndef FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP
#define FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP

#include <QWidget>

class QComboBox;
class QLineEdit;

namespace fairwindsk::ui {

    class GeoCoordinateEditorWidget final : public QWidget {
    Q_OBJECT

    public:
        explicit GeoCoordinateEditorWidget(QWidget *parent = nullptr);

        void setCoordinate(double latitude, double longitude, const QString &formatId);
        QString formatId() const;
        bool coordinate(double *latitude, double *longitude, QString *message = nullptr) const;

    private slots:
        void onFormatChanged(int index);

    private:
        void applyCurrentFormat();

        QComboBox *m_formatCombo = nullptr;
        QLineEdit *m_latitudeEdit = nullptr;
        QLineEdit *m_longitudeEdit = nullptr;
        double m_latitude = 0.0;
        double m_longitude = 0.0;
    };
}

#endif // FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP
