//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include <QWidget>

#include "FileInfoListModel.hpp"

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class SearchPage; }
QT_END_NAMESPACE

class SearchPage : public QWidget {
Q_OBJECT

public:
    explicit SearchPage(const QString& key, QWidget *parent = nullptr);
    ~SearchPage() override;


        void doSearch();

private:
    Ui::SearchPage *ui;

    FileInfoListModel *m_fileInfoListModel;
    QList<QFileInfo> m_results;

    QString m_key;
};
} // fairwindsk::ui::mydata

#endif //SEARCHPAGE_H
