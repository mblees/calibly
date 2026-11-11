#pragma once

#include <QList>
#include <QObject>
#include <QStringList>

#include "work.h"

// Imports files into the library folder. Runs in a background thread
// (see LibraryController), because reading and copying large audiobooks takes
// a while. It does not touch the database: the finished works are handed back
// and saved by the controller in the main thread.
class ImportWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // Imports the files (folders are searched for supported files).
    // existing: the works already in the library, to add formats to them.
    void run(const QStringList &paths, const QList<Work> &existing, const QString &libraryRoot);

    // Expands folders into the supported files they contain.
    static QStringList collectFiles(const QStringList &paths);

signals:
    void progress(int done, int total, const QString &fileName);
    // works: new works and existing ones with added formats, ready to save.
    void finished(const QList<Work> &works, int addedFormats, const QStringList &duplicates,
                  const QStringList &failed);
};
