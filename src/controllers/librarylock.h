#pragma once

#include <QLocalServer>
#include <QLockFile>
#include <QObject>
#include <QString>

// Makes sure only one calibly works with a library folder at a time, so two
// running copies never write to the same calibly.db.
// The lock is the file calibly.lock in the library folder (removed on exit;
// a lock left behind by a crashed calibly is taken over). While it holds the
// lock, calibly listens for a second start with the same library, which
// then asks it to show its window (activateOwner()) and ends.
class LibraryLock : public QObject
{
    Q_OBJECT

public:
    enum Result { Acquired, HeldElsewhere, Failed };

    // The folder must exist.
    explicit LibraryLock(const QString &folder, QObject *parent = nullptr);
    ~LibraryLock() override;

    Result acquire();

    // Asks the calibly that holds the lock of this folder to bring its
    // window to the front. False if it does not answer.
    static bool activateOwner(const QString &folder);

signals:
    // Another start of calibly with this library.
    void activationRequested();

private:
    static QString serverName(const QString &folder);

    QString m_folder;
    QLockFile m_lockFile;
    QLocalServer m_server;
};
