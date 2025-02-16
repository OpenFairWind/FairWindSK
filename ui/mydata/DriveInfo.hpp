//
// Created by Raffaele Montella on 16/02/25.
//

#ifndef DRIVEINFO_H
#define DRIVEINFO_H

#include <QStorageInfo>
#include <QWidget>

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {
QT_BEGIN_NAMESPACE
namespace Ui { class DriveInfo; }
QT_END_NAMESPACE

    class DriveInfo : public QWidget
    {
        Q_OBJECT

      public:
        explicit DriveInfo(QWidget *parent = nullptr);
        ~DriveInfo() override;
        void refreshDrive(const QString &path = QDir::currentPath());

        private:
        Ui::DriveInfo *ui;
        QStorageInfo *info;
    };
} // fairwindsk::ui::mydata

#endif //DRIVEINFO_H
