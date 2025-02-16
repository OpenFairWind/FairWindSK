//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef NAVPAGE_H
#define NAVPAGE_H




#include <QAbstractItemView>
#include <QDir>

#include "DirItemInfo.hpp"
#include "DriveInfo.hpp"
#include "Files.hpp"

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class NavPage; }
QT_END_NAMESPACE

class NavPage : public QWidget {
Q_OBJECT

public:
    explicit NavPage(QFileSystemModel *model, Files *root, QWidget *parent = nullptr);
    ~NavPage() override;

    int setCurrentDir(QString new_dir);
    inline QString getCurrentDir() const { return m_currentDir->absolutePath(); }
    QStringList getSelection();

private:
    Ui::NavPage *ui;

    DriveInfo *m_driveInfo;
    DirItemInfo *m_itemInfo;
    Files *m_files;
    QDir *m_currentDir;
    QAbstractItemView *m_dirView;
    QFileSystemModel *m_model;

    void setDirView();

    private slots:
        void onItemViewActivated(const QModelIndex &index);
        void onItemViewClicked(const QModelIndex &index);
};
} // fairwindsk::ui::mydata

#endif //NAVPAGE_H
