//
// Created by Raffaele Montella on 28/03/21.
//

#ifndef FAIRWINDSK_WEBPOPUPWINDOW_HPP
#define FAIRWINDSK_WEBPOPUPWINDOW_HPP


#include <QWidget>
#include "FairWindSK.hpp"

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace fairwindsk::ui::web {
    class WebView;

    class WebPopupWindow : public QWidget
    {
    Q_OBJECT

    public:
        explicit WebPopupWindow(fairwindsk::WebProfileHandle *profile);
        WebView *view() const;



    private slots:
        void handleGeometryChangeRequested(const QRect &newGeometry);

    private:
        QLineEdit *m_urlLineEdit;
        QAction *m_favAction;
        WebView *m_view;
    };
}

#endif //FAIRWINDSK_WEBPOPUPWINDOW_HPP
