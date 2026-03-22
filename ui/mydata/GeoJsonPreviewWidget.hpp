//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP
#define FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP

#include <QJsonDocument>
#include <QWidget>

class QPlainTextEdit;
class QTabWidget;
class QWebEngineView;

namespace fairwindsk::ui::mydata {

    class GeoJsonPreviewWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit GeoJsonPreviewWidget(QWidget *parent = nullptr);

        void setGeoJson(const QJsonDocument &document, const QString &title = QString());
        void setMessage(const QString &message, const QString &title = QString());

    private:
        static QString detectFreeboardUrl();
        static QString htmlForContent(const QString &bodyScript);
        void ensureFreeboardTab(const QString &url);

        QTabWidget *m_tabWidget = nullptr;
        QWebEngineView *m_view = nullptr;
        QWebEngineView *m_freeboardView = nullptr;
        QPlainTextEdit *m_textView = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_GEOJSONPREVIEWWIDGET_HPP
