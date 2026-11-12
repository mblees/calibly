#pragma once

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QVariantMap>

#include "importqueuemodel.h"
#include "importworker.h"

class LibraryController;

// Imports dropped or picked files into the library (view D of the mockup).
//
// Every importFiles() call is one job. Jobs run one after the other in a
// background thread (ImportWorker), so files can be dropped while an import
// is still running. The queue shows each job while its files are read and
// then one row per work: a new work, or an existing one that gets formats.
// Each work is saved as soon as its files are copied, so it appears in the
// gallery right away. The files of an audiobook folder (e.g. 12 MP3) become
// one title, see groupAudioFolders().
//
// In QML (import Calibly.Controllers):
//   ImportController.importFiles(urls)            new works (or formats of
//                                                 matching works)
//   ImportController.importFiles(urls, workId)    formats of this work
//   ImportController.inspect(urls)  what the urls contain, for the drop hint:
//       { fileCount, ebookCount, audiobookCount, name, typesText ("27 MP3") }
//   ImportController.queue          ImportQueueModel
//   ImportController.busy           a job is running or waiting
// Signals:
//   importFinished(added, skipped, failed)  one job is done: added = files
//       added to the library, skipped = already there, failed = not copied
class ImportController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(ImportQueueModel *queue READ queue CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit ImportController(LibraryController &library, QQmlEngine *engine, QObject *parent = nullptr);
    ~ImportController() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static ImportController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    ImportQueueModel *queue() { return &m_queue; }
    bool busy() const { return m_running || !m_jobs.isEmpty(); }

    Q_INVOKABLE void importFiles(const QList<QUrl> &urls, const QString &targetWorkId = QString());
    Q_INVOKABLE QVariantMap inspect(const QList<QUrl> &urls) const;

signals:
    void busyChanged();
    void importFinished(int added, int skipped, int failed);

private:
    struct Job
    {
        QStringList paths;
        QString targetWorkId;
        int entryId = 0;    // the queue row of the job until its works are known
    };

    static QStringList localPaths(const QList<QUrl> &urls);
    void startNext();
    void onProgress(int done, int total, const QString &fileName);
    void onPlanned(const QList<ImportItem> &items, const QStringList &duplicates);
    void onItemStarted(int item);
    void onItemProgress(int item, int done, int total);
    void onWorkReady(int item, const Work &work, const QStringList &failed);
    void onFinished(const QList<Work> &works, int added, const QStringList &duplicates,
                    const QStringList &failed);

    LibraryController &m_library;
    QQmlEngine *m_engine = nullptr;
    ImportQueueModel m_queue;

    QThread m_thread;
    ImportWorker *m_worker = nullptr;
    QList<Job> m_jobs;               // waiting
    Job m_current;
    bool m_running = false;
    QList<ImportItem> m_items;       // works of the current job
    QList<int> m_itemEntries;        // their queue rows
    int m_saveFailures = 0;

    static inline ImportController *s_instance = nullptr;
};
