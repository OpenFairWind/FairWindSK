//
// Created by Codex on 18/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_ROUTESLITE_HPP
#define FAIRWINDSK_UI_MYDATA_ROUTESLITE_HPP

#include <QWidget>

#include "ResourceModel.hpp"

class QLineEdit;
class QTableWidget;
class QToolButton;

namespace fairwindsk::ui::mydata {

    class RoutesLite final : public QWidget {
        Q_OBJECT

    public:
        explicit RoutesLite(QWidget *parent = nullptr);
        ~RoutesLite() override = default;

    private slots:
        void rebuildTable();
        void onSearchTextChanged(const QString &text);
        void onRefreshClicked();

    private:
        void configureTable();

        ResourceModel *m_model = nullptr;
        QLineEdit *m_searchEdit = nullptr;
        QToolButton *m_refreshButton = nullptr;
        QTableWidget *m_tableWidget = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_ROUTESLITE_HPP
