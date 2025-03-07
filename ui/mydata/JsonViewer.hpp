//
// Created by Raffaele Montella on 06/03/25.
//

#ifndef JSONVIEWER_H
#define JSONVIEWER_H

#include <QWidget>

namespace fairwindsk::ui::mydata
{
    QT_BEGIN_NAMESPACE
    namespace Ui { class JsonViewer; }
    QT_END_NAMESPACE

    class JsonViewer : public QWidget {
        Q_OBJECT

        public:
        explicit JsonViewer(const QString& path, QWidget *parent = nullptr);
        ~JsonViewer() override;

        signals:
        void askedToBeClosed();

        public slots:
            void onCloseClicked();

    private:
        Ui::JsonViewer *ui;
    };
}

#endif //JSONVIEWER_H
