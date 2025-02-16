//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef DIRITEMINFO_H
#define DIRITEMINFO_H

#include <QStorageInfo>
#include <QWidget>

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class DirItemInfo; }
QT_END_NAMESPACE

    class DirItemInfo : public QWidget
{
    Q_OBJECT

public:
    explicit DirItemInfo(QFileInfo *info, QWidget *parent = nullptr);
    ~DirItemInfo() override;
    void refresh();

private:
    Ui::DirItemInfo *ui;
    QFileInfo *info;
};
} // fairwindsk::ui::mydata

#endif //DIRITEMINFO_H
