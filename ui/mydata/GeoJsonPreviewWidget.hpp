//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP
#define FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP

#include <QJsonDocument>
#include <QWidget>

class QWebEngineView;

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit GeoJsonPreviewWidget(QWidget *parent = nullptr);

        void setGeoJson(const QJsonDocument &document, const QString &title = QString());
        void setMessage(const QString &message, const QString &title = QString());

    private:
        static QString htmlForContent(const QString &title, const QString &bodyScript);

        QWebEngineView *m_view = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP
