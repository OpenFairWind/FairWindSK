//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_APPS_HPP
#define FAIRWINDSK_APPS_HPP

#include <QWidget>

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Apps; }
    QT_END_NAMESPACE

    class Apps : public QWidget {
    Q_OBJECT

    public:
        explicit Apps(QWidget *parent = nullptr);

        ~Apps() override;


    private slots:

        void onAppsListSelectionChanged();
        void onAppsEditSaveClicked();
        void onAppsDetailsFieldsTextChanged(const QString &text);

    private:
        void setAppsEditMode(bool appsEditMode);
        void saveAppsDetails();

    private:
        Ui::Apps *ui;

        bool m_appsEditMode;
        bool m_appsEditChanged;
    };
} // fairwindsk::ui::settings 

#endif //FAIRWINDSK_APPS_HPP
