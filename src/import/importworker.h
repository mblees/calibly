#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>

#include "work.h"

// One work of an import as the import queue shows it: a new work, or an
// existing one that gets new formats.
struct ImportItem
{
    QString workId;
    QString title;
    bool newWork = true;         // false: formats are added to a work of the library
    QStringList fileTypes;       // upper-case suffixes of the files, e.g. {"MP3"}
    int fileCount = 0;
    bool merged = false;         // several audio files become one audiobook
    bool hasEbook = false;       // the files bring an ebook / an audiobook
    bool hasAudiobook = false;
    bool needsReview = false;    // a new work whose title is only the file name
};

// What a set of dropped files and folders contains, for the drop hint.
struct PathSummary
{
    QString name;                // file or folder name of the first path
    int ebookCount = 0;
    int audiobookCount = 0;
    QMap<QString, int> countBySuffix; // upper-case suffix -> number of files

    int fileCount() const { return ebookCount + audiobookCount; }
};

// Imports files into the library folder. Runs in a background thread
// (see ImportController), because reading and copying large audiobooks takes
// a while. It does not touch the database: every finished work is handed
// back (workReady) and saved by the controller in the main thread.
//
// Order of the signals of one run():
//   progress(…)                 while the files are read
//   planned(items, duplicates)  which works the files become
//   itemStarted(i), itemProgress(i, …), workReady(i, …)   per item
//   finished(…)
class ImportWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // Imports the files (folders are searched for supported files).
    // existing: the works already in the library, to add formats to them.
    // targetWorkId: the files were dropped onto this work and become its formats.
    void run(const QStringList &paths, const QList<Work> &existing, const QString &libraryRoot,
             const QString &targetWorkId = QString());

    // Expands folders into the supported files they contain.
    static QStringList collectFiles(const QStringList &paths);

    // Counts the supported files below the paths, looking at no more than
    // limit files (folders can be large; it is called while dragging).
    static PathSummary inspectPaths(const QStringList &paths, int limit = 5000);

signals:
    void progress(int done, int total, const QString &fileName);
    void planned(const QList<ImportItem> &items, const QStringList &duplicates);
    void itemStarted(int item);
    void itemProgress(int item, int done, int total);
    // work: ready to save, with all formats copied so far; failed: source
    // paths of this item that could not be copied.
    void workReady(int item, const Work &work, const QStringList &failed);
    // works: all works of workReady() again, ready to save.
    void finished(const QList<Work> &works, int addedFormats, const QStringList &duplicates,
                  const QStringList &failed);
};
