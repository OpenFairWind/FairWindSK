//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef FILES_H
#define FILES_H

#include <QWidget>
#include <QFileSystemModel>


// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class Files; }
QT_END_NAMESPACE

class Files : public QWidget {
Q_OBJECT

public:
    explicit Files(QWidget *parent = nullptr);
    ~Files() override;

    int addPageToTabWidget(const QString& dir, const QString &label);

    enum class CDSource { Navbar, Navpage, Navtree, Navbutton, Tabchange };

    bool selectFileSystemItem(const QString &path, CDSource source, bool suppress_warning = false);

    public slots:

        void onHome();

        void onNewTabClicked();

        void onTabWidgetCurrentChanged(int index);

        void onTabWidgetTabCloseRequested(int index);

        void onAddressBarReturnPressed();

        void onTreeViewClicked(const QModelIndex &index);

        void onBackClicked();

        void onUpClicked();

        void onNewFileClicked();

        void onNewFolderClicked();

        void onDeleteClicked();

        void onRenameClicked();

        void onCopyClicked();

        void onPasteClicked();

        void onCutClicked();

        void onCloseTabClicked();

        void onSearchClicked();

private:
    Ui::Files *ui;

    QList<QString> m_visitedPaths;
    QFileSystemModel *m_fileSystemModel;
    QStringList m_itemsToCopy;
    QStringList m_itemsToMove;


    void addSearchPage();
    void locateInTree(const QString &path);
    void showWarning(const QString &message);
};
} // fairwindsk::ui::mydata

#endif //FILES_H
