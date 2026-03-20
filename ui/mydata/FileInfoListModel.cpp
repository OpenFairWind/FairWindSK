//
// Created by Raffaele Montella on 16/02/25.
//

#include "FileInfoListModel.hpp"

#include "Files.hpp"


// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {

    FileInfoListModel::FileInfoListModel(QObject *parent)
    : QAbstractTableModel(parent)
    {
        m_header << "Name"
            << "Size"
               << "Type"
               << "Path";
    }

    FileInfoListModel::~FileInfoListModel() = default;




    QVariant FileInfoListModel::headerData(const int section, const Qt::Orientation orientation, const int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 && section < m_header.size()) {

            return m_header.at(section);
        }

        return {};
    }

    int FileInfoListModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return m_header.size();
    }

    int FileInfoListModel::rowCount(const QModelIndex &parent) const
    {
        // For list models only the root node (an invalid parent) should return the list's size. For all
        // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
        if (parent.isValid())
            return 0;

        return m_fileInfoList.size();
    }

    QVariant FileInfoListModel::data(const QModelIndex &index, const int role) const {
        if (!index.isValid())
            return {};

        if (index.row() < 0 || index.row() >= m_fileInfoList.size())
            return {};

        if (index.column() < 0 || index.column() >= m_header.size())
            return {};

        const QFileInfo &fileInfo = m_fileInfoList.at(index.row());

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
                case 0:
                    return fileInfo.fileName();
                case 1:
                    return Files::format_bytes(fileInfo.size());
                case 2:
                    return fileInfo.isFile() ? "File" : "Folder";
                case 3:
                    return fileInfo.absoluteFilePath();
                default:
                    return {};
            }

        }

        return {};
    }

    QString FileInfoListModel::getAbsolutePath(const QModelIndex &index) {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_fileInfoList.size()) {
            return {};
        }

        return m_fileInfoList.at(index.row()).absoluteFilePath();
    }

    void FileInfoListModel::setQFileInfoList(const QList<QFileInfo> &fileInfos)
    {
        beginResetModel();
        m_fileInfoList = fileInfos;
        endResetModel();
    }
} // fairwindsk::ui::mydata
