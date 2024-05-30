//
// Created by Raffaele Montella on 06/05/24.
//

#ifndef FAIRWINDSK_APPS_HPP
#define FAIRWINDSK_APPS_HPP

#include <QWidget>
#include <QListWidgetItem>
#include "Settings.hpp"

namespace fairwindsk::ui::settings {
    QT_BEGIN_NAMESPACE
    namespace Ui { class Apps; }
    QT_END_NAMESPACE

    class Apps : public QWidget {
    Q_OBJECT

    public:
        explicit Apps(Settings *settings, QWidget *parent = nullptr);

        ~Apps() override;



    private slots:

        void onAppsListSelectionChanged();
        void onAppsListItemChanged(QListWidgetItem* listWidgetItem);
        void onAppsEditSaveClicked();
        void onAppsDetailsFieldsTextChanged(const QString &text);
        void onAppsAppIconBrowse();
        void onAppsNameBrowse();
        void onAppsAddClicked();
        void onAppsRemoveClicked();
        void onAppsUpClicked();
        void onAppsDownClicked();

    private:
        bool eventFilter(QObject *object, QEvent *event) override;
        void setAppsEditMode(bool appsEditMode);
        void saveAppsDetails();

    private:
        Ui::Apps *ui;

        Settings *m_settings;
        bool m_appsEditMode;
        bool m_appsEditChanged;

        //QMap<QString, AppItem *> m_mapHash2AppItem;
    };
} // fairwindsk::ui::settings 

#endif //FAIRWINDSK_APPS_HPP
