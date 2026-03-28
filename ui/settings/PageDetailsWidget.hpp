//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_PAGEDETAILSWIDGET_HPP
#define FAIRWINDSK_PAGEDETAILSWIDGET_HPP

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class PageDetailsWidget; }
QT_END_NAMESPACE

namespace fairwindsk::ui::settings {
    class PageDetailsWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit PageDetailsWidget(QWidget *parent = nullptr);
        ~PageDetailsWidget() override;

        ::Ui::PageDetailsWidget *ui = nullptr;
    };
}

#endif // FAIRWINDSK_PAGEDETAILSWIDGET_HPP
