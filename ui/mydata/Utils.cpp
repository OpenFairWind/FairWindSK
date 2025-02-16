//
// Created by Raffaele Montella on 16/02/25.
//

#include "Utils.hpp"

// The base source code is https://github.com/kitswas/File-Manager/

namespace fairwindsk::ui::mydata {

    QString Utils::format_bytes(qint64 bytes)
    {
        QString result;
        qint64 higher = bytes;
        int index = 0;
        QString units[] = {"B", "KB", "MB", "GB", "TB"};
        while (higher > 1000) {
            ++index;
            higher /= 1000;
        }
        result = QString::number(higher) + " " + units[index];
        return result;
    }

    void Utils::copyOrMoveDirectorySubtree(const QString &from,
                                    const QString &to,
                                    bool copyAndRemove,
                                    bool overwriteExistingFiles)
    {
        QDirIterator diritems(from, QDirIterator::Subdirectories);
        QDir fromDir(from);
        QDir toDir(to);

        const auto absSourcePathLength = fromDir.absolutePath().length();

        toDir.mkdir(fromDir.dirName());
        toDir.cd(fromDir.dirName());

        while (diritems.hasNext()) {
            const QFileInfo fileInfo = diritems.nextFileInfo();

            if (fileInfo.fileName().compare(".") == 0
                || fileInfo.fileName().compare("..") == 0) //filters dot and dotdot
                    continue;
            const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
            qDebug() << "subPathStructure: " << subPathStructure;
            const QString constructedAbsolutePath = toDir.canonicalPath() + subPathStructure;
            qDebug() << "constructedAbsolutePath: " << constructedAbsolutePath;

            if (fileInfo.isDir()) {
                //Create directory in target folder
                toDir.mkpath(constructedAbsolutePath);
            } else if (fileInfo.isFile()) {
                //Copy File to target directory

                if (overwriteExistingFiles) {
                    //Remove file at target location, if it exists, or QFile::copy will fail
                    QFile::remove(constructedAbsolutePath);
                }
                QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
            }
        }

        if (copyAndRemove)
            fromDir.removeRecursively();
    }
}