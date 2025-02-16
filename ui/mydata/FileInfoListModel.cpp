//
// Created by Raffaele Montella on 16/02/25.
//

#include "FileInfoListModel.hpp"

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {

    FileInfoListModel::FileInfoListModel(QObject *parent)
    : QAbstractTableModel(parent)
    {
        header << "Name"
               << "Type"
               << "Path";
    }

    FileInfoListModel::~FileInfoListModel() {}

    QVariant FileInfoListModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0
            && section < header.size()) {
            qDebug() << "header data " << header.at(section) << " fetched for section " << section;
            return header.at(section);
            }
        return QVariant();
    }

    int FileInfoListModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return header.size();
    }

    int FileInfoListModel::rowCount(const QModelIndex &parent) const
    {
        // For list models only the root node (an invalid parent) should return the list's size. For all
        // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
        if (parent.isValid())
            return 0;

        return fileInfoList.size();
    }

    QVariant FileInfoListModel::data(const QModelIndex &index, int role) const
    {
        QStringList _data;
        _data << fileInfoList.at(index.row()).fileName()
              << (fileInfoList.at(index.row()).isFile() ? "File" : "Folder")
              << fileInfoList.at(index.row()).absoluteFilePath();

        if (!index.isValid())
            return QVariant();

        if (index.row() < 0 || index.row() >= fileInfoList.size())
            return QVariant();

        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return _data.at(index.column());

        return QVariant();
    }

    void FileInfoListModel::setQFileInfoList(const QList<QFileInfo> &fileInfos)
    {
        beginResetModel();
        fileInfoList = fileInfos;
        endResetModel();
    }
} // fairwindsk::ui::mydata