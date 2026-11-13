// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "librarylock.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

LibraryLock::LibraryLock(const QString &folder, QObject *parent)
    : QObject(parent)
    , m_folder(folder)
    , m_lockFile(QDir(folder).filePath(QStringLiteral("calibly.lock")))
{
    // Only a lock of a process that is no longer running is stale, never
    // one that is merely old.
    m_lockFile.setStaleLockTime(0);
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            socket->disconnectFromServer();
            emit activationRequested();
        }
    });
}

LibraryLock::~LibraryLock()
{
    m_server.close();
    if (m_lockFile.isLocked())
        m_lockFile.unlock();
}

LibraryLock::Result LibraryLock::acquire()
{
    if (m_lockFile.isLocked())
        return Acquired;
    if (!m_lockFile.tryLock(0))
        return m_lockFile.error() == QLockFile::LockFailedError ? HeldElsewhere : Failed;
    const QString name = serverName(m_folder);
    QLocalServer::removeServer(name);
    if (!m_server.listen(name))
        qWarning("LibraryLock: cannot listen for other starts: %s", qPrintable(m_server.errorString()));
    return Acquired;
}

bool LibraryLock::activateOwner(const QString &folder)
{
#ifdef Q_OS_WIN
    // This process was just started by the user, so it may hand the right
    // to come to the front to the running calibly.
    AllowSetForegroundWindow(ASFW_ANY);
#endif
    QLocalSocket socket;
    socket.connectToServer(serverName(folder));
    if (!socket.waitForConnected(2000))
        return false;
    socket.waitForDisconnected(2000);
    return true;
}

QString LibraryLock::serverName(const QString &folder)
{
    // The same folder written differently ("C:/Bib", "c:\bib\") is one library.
    QString path = QDir::cleanPath(QFileInfo(folder).absoluteFilePath());
#ifdef Q_OS_WIN
    path = path.toLower();
#endif
    const QByteArray hash = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    return QStringLiteral("calibly-library-") + QString::fromLatin1(hash);
}
