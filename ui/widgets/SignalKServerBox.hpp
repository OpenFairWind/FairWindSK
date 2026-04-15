//
// Created by Codex on 02/04/26.
//

#ifndef FAIRWINDSK_SIGNALKSERVERBOX_HPP
#define FAIRWINDSK_SIGNALKSERVERBOX_HPP

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>

class QMovie;

namespace fairwindsk::ui::widgets {
    QT_BEGIN_NAMESPACE
    namespace Ui { class SignalKServerBox; }
    QT_END_NAMESPACE

    class SignalKServerBox : public QWidget {
        Q_OBJECT

    public:
        explicit SignalKServerBox(QWidget *parent = nullptr);
        ~SignalKServerBox() override;

    protected:
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void onServerHealthChanged(bool healthy, const QString &statusText);
        void onConnectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void onRequestActivityChanged(bool active);
        void onServerMessageChanged(const QString &message);

    private:
        void applyIndicatorColor(QLabel *label, const QColor &color);
        void applyStatusBadge(QLabel *label, const QString &text, const QColor &fillColor, const QColor &textColor);
        void updateStatusLabel(const QString &text);
        void setBusyVisible(bool active);

        Ui::SignalKServerBox *ui = nullptr;
        QMovie *m_throbber = nullptr;
        QPixmap m_idleBusyPixmap;
    };
}

#endif // FAIRWINDSK_SIGNALKSERVERBOX_HPP
