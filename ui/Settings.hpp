//
// Created by Raffaele Montella on 18/03/24.
//

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QtZeroConf/qzeroconf.h>

namespace Ui { class Settings; }

namespace fairwindsk::ui {

    class Settings : public QWidget {
    Q_OBJECT

    public:
        explicit Settings(QWidget *parent = nullptr, QWidget *currenWidget = nullptr);

        ~Settings() override;

        QWidget *getCurrentWidget();

    public slots:
        void onAccepted();

    signals:
        void accepted(Settings *);

    private slots:

        void onVirtualKeyboard(int state);

        void onConnect();
        void onStop();
        void onRestart();

        void addService(const QZeroConfService& item);

        void onAppsListSelectionChanged();
        void onAppsEditSaveClicked();
        void onAppsDetailsFieldsTextChanged(const QString &text);

    private:
        void setAppsEditMode(bool appsEditMode);
        void saveAppsDetails();

    private:
        Ui::Settings *ui;
        bool m_stop;
        QZeroConf m_zeroConf;
        bool m_appsEditMode;
        bool m_appsEditChanged;

        QWidget *m_currentWidget;

    };

} // fairwindsk::ui

#endif //SETTINGS_HPP
