#ifndef FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP
#define FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP

#include <QWidget>

namespace Ui { class GeoCoordinateEditorWidget; }

namespace fairwindsk::ui {

    class GeoCoordinateEditorWidget final : public QWidget {
    Q_OBJECT

    public:
        explicit GeoCoordinateEditorWidget(QWidget *parent = nullptr);
        ~GeoCoordinateEditorWidget() override;

        void setCoordinate(double latitude, double longitude, double altitude, const QString &formatId);
        QString formatId() const;
        bool coordinate(double *latitude, double *longitude, double *altitude = nullptr, QString *message = nullptr) const;

    private slots:
        void onFormatChanged(int index);

    private:
        void applyCurrentFormat();

        ::Ui::GeoCoordinateEditorWidget *ui = nullptr;
        double m_latitude = 0.0;
        double m_longitude = 0.0;
        double m_altitude = 0.0;
    };
}

#endif // FAIRWINDSK_UI_GEOCOORDINATEEDITORWIDGET_HPP
