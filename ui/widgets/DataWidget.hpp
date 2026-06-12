#ifndef FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP
#define FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP

#include <QJsonObject>
#include <QPointer>
#include <QWidget>
#include <limits>

#include "DataWidgetConfig.hpp"

class QLabel;
class QProgressBar;
class QWidget;

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
        void setDisplayOptions(bool showIcon, bool showText, bool showUnits, bool showTrend);
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
        void updateHeaderVisibility();
        void updateTrend(double value, bool hasValue);
        QString trendText() const;

        DataWidgetDefinition m_definition;
        QJsonObject m_lastUpdate;
        double m_lastTrendValue = std::numeric_limits<double>::quiet_NaN();
        int m_trendDirection = 0;
        fairwindsk::Units *m_units = nullptr;
        QPointer<fairwindsk::signalk::Client> m_client;
        QWidget *m_headerWidget = nullptr;
        QWidget *m_valueWidget = nullptr;
        QLabel *m_iconLabel = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_valueLabel = nullptr;
        QLabel *m_unitLabel = nullptr;
        QLabel *m_trendLabel = nullptr;
        QProgressBar *m_gauge = nullptr;
        bool m_showIcon = true;
        bool m_showText = true;
        bool m_showUnits = true;
        bool m_showTrend = false;
        bool m_rendering = false;
        // Guards against re-entrant calls: setStyleSheet() triggers changeEvent()
        // which would call applyComfortChrome() again causing a stack overflow.
        bool m_applyingChrome = false;
    };
}

#endif // FAIRWINDSK_UI_WIDGETS_DATAWIDGET_HPP
