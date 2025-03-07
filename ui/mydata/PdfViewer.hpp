//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef PDFVIEWER_H
#define PDFVIEWER_H

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QPdfDocument>
#include <QPdfView>

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class PdfViewer; }
QT_END_NAMESPACE

class PdfViewer : public QWidget {
Q_OBJECT

public:
    explicit PdfViewer(const QString& path, QWidget *parent = nullptr);
    ~PdfViewer() override;

    signals:
        void askedToBeClosed();

    public slots:
        void onZoomInClicked();
    void onZoomOutClicked();
    void onOne2OneClicked() ;
    void onCloseClicked();

private:
    Ui::PdfViewer *ui;

    QPdfDocument *m_document = nullptr;

    const double m_zoomMultiplier = qSqrt(2.0);
};
} // fairwindsk::ui::mydata

#endif //PDFVIEWER_H
