//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_APPDETAILSWIDGET_HPP
#define FAIRWINDSK_APPDETAILSWIDGET_HPP

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AppDetailsWidget; }
QT_END_NAMESPACE

namespace fairwindsk::ui::settings {
    class AppDetailsWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit AppDetailsWidget(QWidget *parent = nullptr);
        ~AppDetailsWidget() override;

        ::Ui::AppDetailsWidget *ui = nullptr;
    };
}

#endif // FAIRWINDSK_APPDETAILSWIDGET_HPP
