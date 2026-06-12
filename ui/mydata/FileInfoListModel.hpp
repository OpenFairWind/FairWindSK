//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef FILEINFOLISTMODEL_H
#define FILEINFOLISTMODEL_H

#include <QAbstractTableModel>
#include <QFileInfo>


// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {

    class FileInfoListModel final : public QAbstractTableModel {

        Q_OBJECT

    public:
        explicit FileInfoListModel(QObject *parent = nullptr);
        ~FileInfoListModel() override;

        // Header:

        QVariant headerData(int section,
                            Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;

        // Basic functionality:

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        QString getAbsolutePath(const QModelIndex &index);

        /*!
         * \brief QFileInfoListModel::setQFileInfoList
         * \param fileInfos
         *
         * Sets the model's internal QFileInfo list to `fileInfos`.
         * The model will notify any attached views that its underlying data has changed.
         */
        void setQFileInfoList(const QList<QFileInfo> &fileInfos);

    private:
        QStringList m_header;
        QList<QFileInfo> m_fileInfoList;

    };



} // fairwindsk::ui::mydata

#endif //FILESYSTEMMODEL_H
