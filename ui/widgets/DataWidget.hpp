#ifndef FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP
#define FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP

#include <QJsonObject>
#include <QPointer>
#include <QWidget>

#include "DataWidgetConfig.hpp"

class QLabel;
class QProgressBar;

namespace fairwindsk {
    class Units;
}

namespace fairwindsk::signalk {
    class Client;
}

namespace fairwindsk::ui::widgets {

    class DataWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit DataWidget(const DataWidgetDefinition &definition, QWidget *parent = nullptr);
        ~DataWidget() override;

        QString widgetId() const;
        void setDefinition(const DataWidgetDefinition &definition);
        QSize sizeHint() const override;

    public slots:
        void updateFromSignalK(const QJsonObject &update);

    protected:
        void changeEvent(QEvent *event) override;

    private:
        void buildUi();
        void subscribeToSignalK();
        void unsubscribeFromSignalK();
        void renderCurrentUpdate();
        void applyComfortChrome();
        QString formattedNumericValue(double value) const;
        QString unitLabel() const;
        void updateIcon();

        DataWidgetDefinition m_definition;
        QJsonObject m_lastUpdate;
        fairwindsk::Units *m_units = nullptr;
        QPointer<fairwindsk::signalk::Client> m_client;
        QLabel *m_iconLabel = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_valueLabel = nullptr;
        QLabel *m_unitLabel = nullptr;
        QProgressBar *m_gauge = nullptr;
        // Guards against re-entrant calls: setStyleSheet() triggers changeEvent()
        // which would call applyComfortChrome() again causing a stack overflow.
        bool m_applyingChrome = false;
    };
}

#endif // FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP
