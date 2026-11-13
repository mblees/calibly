// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "appinfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "logfile.h"

#ifndef CALIBLY_RELEASES_URL
#define CALIBLY_RELEASES_URL "https://github.com/mblees/calibly/releases"
#endif
#ifndef CALIBLY_SOURCE_URL
#define CALIBLY_SOURCE_URL "https://github.com/mblees/calibly"
#endif
#ifndef CALIBLY_COPYRIGHT
#define CALIBLY_COPYRIGHT "Copyright (C) 2026 mblees"
#endif

AppInfo::AppInfo(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

AppInfo::~AppInfo()
{
    s_instance = nullptr;
}

AppInfo *AppInfo::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QString AppInfo::version() const
{
    return QCoreApplication::applicationVersion();
}

QString AppInfo::copyright() const
{
    return QStringLiteral(CALIBLY_COPYRIGHT);
}

QUrl AppInfo::releasesUrl() const
{
    return QUrl(QStringLiteral(CALIBLY_RELEASES_URL));
}

QUrl AppInfo::sourceUrl() const
{
    return QUrl(QStringLiteral(CALIBLY_SOURCE_URL));
}

QUrl AppInfo::noticesUrl() const
{
    return besideProgram(QStringLiteral("THIRD-PARTY-NOTICES.txt"));
}

QUrl AppInfo::logFolderUrl() const
{
    const QString folder = LogFile::directory();
    return folder.isEmpty() ? QUrl() : QUrl::fromLocalFile(folder);
}

QUrl AppInfo::licenseUrl() const
{
    return besideProgram(QStringLiteral("LICENSE.txt"));
}

QUrl AppInfo::folderUrl(const QString &path) const
{
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QUrl AppInfo::besideProgram(const QString &fileName) const
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(fileName);
    return QFileInfo::exists(path) ? QUrl::fromLocalFile(path) : QUrl();
}
