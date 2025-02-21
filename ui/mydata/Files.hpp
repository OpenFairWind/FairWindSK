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

    enum class CDSource { Navbar, Navpage, Navbutton };

    bool selectFileSystemItem(const QString &path, CDSource source);


    QStringList getSelection() const;

    void setCurrentDir(const QString& new_dir);
    QString getCurrentDir() const;

    /*!
     * \brief Format the provided number as bytes.
     * \param bytes
     * \return a string
     *
     * Aims to present the number in a human-friendly format.
     * Converts bytes to a higher order unit ("KB", "MB", "GB", "TB"), if possible.
     */
    static QString format_bytes(qint64 bytes);

    /*!
         * \brief copyOrMoveDirectorySubtree
         * \param from
         * \param to
         * \param copyAndRemove If `true`, move instead of copy.
         * \param overWriteExistingFiles If `true`, overwrite existing files at the destination. **Destructive.**
         *
         * Copies a folder and all its contents.
         * Reference: https://forum.qt.io/topic/105993/copy-folder-qt-c/5?_=1675790958476&lang=en-GB
         */
    static void copyOrMoveDirectorySubtree(const QString &from,
                                    const QString &to,
                                    bool copyAndRemove,
                                    bool overwriteExistingFiles);

    public slots:

        void onItemViewActivated(const QModelIndex &index);

        void onItemViewClicked(const QModelIndex &index) const;

        void onHome();

        void onPathReturnPressed();

        void onBackClicked();

        void onUpClicked();

        void onNewFileClicked();

        void onNewFolderClicked();

        void onDeleteClicked();

        void onRenameClicked();

        void onCopyClicked();

        void onPasteClicked();

        void onCutClicked();

        void onSearchClicked();

private:
    Ui::Files *ui;

    QList<QString> m_visitedPaths;
    QFileSystemModel *m_fileSystemModel;
    QStringList m_itemsToCopy;
    QStringList m_itemsToMove;
    QDir *m_currentDir;


    void addSearchPage();

    void showWarning(const QString &message);
};
} // fairwindsk::ui::mydata

#endif //FILES_H
