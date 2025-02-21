//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_DirItemInfo.h" resolved

#include "DirItemInfo.hpp"
#include "ui_DirItemInfo.h"



#include <QMimeDatabase>

// The base source code is https://github.com/kitswas/File-Manager/

#ifdef Q_OS_WIN
    // See https://doc.qt.io/qt-6/qfileinfo.html#details
    extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#else
int qt_ntfs_permission_lookup = 0; //dummy
#endif

namespace fairwindsk::ui::mydata {


    DirItemInfo::DirItemInfo(QFileInfo *info, QWidget *parent)
        : QWidget(parent)
        , ui(new Ui::DirItemInfo)
        , info(info)
    {
        ui->setupUi(this);
    }

    DirItemInfo::~DirItemInfo()
    {
        delete ui;
    }

    void DirItemInfo::refresh()
    {
        ui->name->setText(info->fileName());
        ui->path->setText(info->filePath());
        QString permissions = "";
        //	qDebug() << (new QStorageInfo(QDir::current()))->fileSystemType();
        bool isNTFS = (new QStorageInfo(QDir::current()))->fileSystemType().compare("NTFS") == 0;
        if (isNTFS)
            qt_ntfs_permission_lookup++; // turn checking on, Windows only
        if (info->isReadable())
            permissions += "Read ";
        if (info->isWritable())
            permissions += "Write ";
        if (info->isExecutable())
            permissions += "Execute ";
        if (isNTFS)
            qt_ntfs_permission_lookup--; // turn it off again, Windows only
        ui->permissions->setText(permissions.trimmed());
        //ui->size->setText(Utils::format_bytes(info->size()));

        QMimeDatabase mimedb;
        ui->mimetype->setText(mimedb.mimeTypeForFile(*info).comment());
    }
} // fairwindsk::ui::mydata
