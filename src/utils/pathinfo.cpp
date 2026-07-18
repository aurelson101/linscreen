// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "pathinfo.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>

const QString PathInfo::whiteIconPath()
{
    return QStringLiteral(":/img/material/white/");
}

const QString PathInfo::blackIconPath()
{
    return QStringLiteral(":/img/material/black/");
}

QStringList PathInfo::translationsPaths()
{
    QString binaryPath =
      QFileInfo(qApp->applicationDirPath()).absoluteFilePath();
    QString trPath = QDir::toNativeSeparators(binaryPath + "/translations");
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    const QString bundledSharePath = QDir(binaryPath).absoluteFilePath(
      QStringLiteral("../share/linscreen/translations"));
    return QStringList()
           << bundledSharePath
           << QStringLiteral(APP_PREFIX) + "/share/linscreen/translations"
           << trPath << QStringLiteral("/usr/share/linscreen/translations")
           << QStringLiteral("/usr/local/share/linscreen/translations");
#endif
    return QStringList() << trPath;
}
