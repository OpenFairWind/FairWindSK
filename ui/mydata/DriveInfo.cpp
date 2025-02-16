//
// Created by Raffaele Montella on 16/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_DriveInfo.h" resolved

#include "DriveInfo.hpp"
#include "ui_DriveInfo.h"

#include "Utils.hpp"

namespace fairwindsk::ui::mydata {
    DriveInfo::DriveInfo(QWidget *parent)
        : QWidget(parent)
        , ui(new Ui::DriveInfo)
    {
        ui->setupUi(this);
        info = nullptr;
    }

    DriveInfo::~DriveInfo()
    {
        delete ui;
        delete info;
    }

    void DriveInfo::refreshDrive(const QString &path)
    {
        if (info)
            delete info;
        info = new QStorageInfo(path);
        ui->name->setText(info->displayName());
        ui->filesys->setText(info->fileSystemType());
        ui->free_space->setText(Utils::format_bytes(info->bytesFree()));
        ui->size->setText(Utils::format_bytes(info->bytesTotal()));
    }
} // fairwindsk::ui::mydata
