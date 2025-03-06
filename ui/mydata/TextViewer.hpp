//
// Created by Raffaele Montella on 03/03/25.
//

#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H

#include <QWidget>
#include <nlohmann/json.hpp>

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class TextViewer; }
QT_END_NAMESPACE

class TextViewer : public QWidget {
Q_OBJECT

public:
    explicit TextViewer(const QString& path, QWidget *parent = nullptr);
    ~TextViewer() override;



    signals:
        void askedToBeClosed();

    public slots:
        void onZoomInClicked();
        void onZoomOutClicked();
        void onOne2OneClicked() ;
        void onCloseClicked();


private:
    Ui::TextViewer *ui;

    qreal m_fontPS;

    QString m_text;
};
} // fairwindsk::ui::mydata

#endif //TEXTVIEWER_H
