//
// Created by Codex on 02/04/26.
//

#ifndef FAIRWINDSK_SIGNALKSERVERBOX_HPP
#define FAIRWINDSK_SIGNALKSERVERBOX_HPP

#include <QLabel>
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

    private slots:
        void onServerHealthChanged(bool healthy, const QString &statusText);
        void onConnectivityChanged(bool restHealthy, bool streamHealthy, const QString &statusText);
        void onRequestActivityChanged(bool active);
        void onServerMessageChanged(const QString &message);

    private:
        void applyIndicatorColor(QLabel *label, const QString &color);

        Ui::SignalKServerBox *ui = nullptr;
        QMovie *m_throbber = nullptr;
    };
}

#endif // FAIRWINDSK_SIGNALKSERVERBOX_HPP
