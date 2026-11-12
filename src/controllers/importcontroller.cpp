#include "importcontroller.h"

#include <QFileInfo>

#include "librarycontroller.h"

ImportController::ImportController(LibraryController &library, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_library(library)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    // The worker lives in its own thread; results come back as queued signals.
    m_worker = new ImportWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ImportWorker::progress, this, &ImportController::onProgress);
    connect(m_worker, &ImportWorker::planned, this, &ImportController::onPlanned);
    connect(m_worker, &ImportWorker::itemStarted, this, &ImportController::onItemStarted);
    connect(m_worker, &ImportWorker::itemProgress, this, &ImportController::onItemProgress);
    connect(m_worker, &ImportWorker::workReady, this, &ImportController::onWorkReady);
    connect(m_worker, &ImportWorker::finished, this, &ImportController::onFinished);
    m_thread.setObjectName(QStringLiteral("calibly import"));
    m_thread.start();
}

ImportController::~ImportController()
{
    // A running import finishes its current step; files already copied stay.
    // Waiting jobs are dropped.
    m_thread.quit();
    m_thread.wait();
    s_instance = nullptr;
}

ImportController *ImportController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QStringList ImportController::localPaths(const QList<QUrl> &urls)
{
    QStringList paths;
    for (const QUrl &url : urls)
        paths.append(url.isLocalFile() ? url.toLocalFile() : url.toString());
    return paths;
}

void ImportController::importFiles(const QList<QUrl> &urls, const QString &targetWorkId)
{
    if (!m_library.canImport() || urls.isEmpty())
        return;

    Job job;
    job.paths = localPaths(urls);
    job.targetWorkId = targetWorkId;

    ImportQueueModel::Entry entry;
    entry.title = QFileInfo(job.paths.first()).fileName();
    entry.fileCount = int(job.paths.size());
    if (const Work *target = m_library.works()->workAt(m_library.works()->rowOf(targetWorkId))) {
        entry.title = target->title;
        entry.newWork = false;
        entry.workId = target->id;
    }
    job.entryId = m_queue.append(entry);

    const bool wasBusy = busy();
    m_jobs.append(job);
    if (!wasBusy)
        emit busyChanged();
    startNext();
}

QVariantMap ImportController::inspect(const QList<QUrl> &urls) const
{
    const PathSummary summary = ImportWorker::inspectPaths(localPaths(urls));
    QStringList types;
    for (auto it = summary.countBySuffix.cbegin(); it != summary.countBySuffix.cend(); ++it)
        types.append(QStringLiteral("%1 %2").arg(it.value()).arg(it.key()));
    return {
        { QStringLiteral("fileCount"), summary.fileCount() },
        { QStringLiteral("ebookCount"), summary.ebookCount },
        { QStringLiteral("audiobookCount"), summary.audiobookCount },
        { QStringLiteral("name"), summary.name },
        { QStringLiteral("typesText"), types.join(QStringLiteral(", ")) },
    };
}

void ImportController::startNext()
{
    if (m_running || m_jobs.isEmpty())
        return;
    m_current = m_jobs.takeFirst();
    m_running = true;
    m_items.clear();
    m_itemEntries.clear();
    m_saveFailures = 0;
    m_queue.update(m_current.entryId, [](ImportQueueModel::Entry &entry) {
        entry.status = ImportQueueModel::Reading;
    });

    // The works as they are now, including those of earlier jobs.
    const QList<Work> existing = m_library.works()->works();
    const QString root = m_library.libraryRoot();
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, job = m_current, existing, root] {
        worker->run(job.paths, existing, root, job.targetWorkId);
    }, Qt::QueuedConnection);
}

void ImportController::onProgress(int done, int total, const QString &)
{
    if (!m_itemEntries.isEmpty())
        return; // the works are known, their rows show the progress
    m_queue.update(m_current.entryId, [done, total](ImportQueueModel::Entry &entry) {
        entry.fileCount = total;
        entry.progress = total > 0 ? double(done) / total : 0.0;
    });
}

void ImportController::onPlanned(const QList<ImportItem> &items, const QStringList &duplicates)
{
    const ImportQueueModel::Entry *placeholder = m_queue.entry(m_current.entryId);
    const QString jobTitle = placeholder ? placeholder->title : QString();

    QList<ImportQueueModel::Entry> entries;
    for (const ImportItem &item : items) {
        ImportQueueModel::Entry entry;
        entry.title = item.title;
        entry.status = ImportQueueModel::Waiting;
        entry.newWork = item.newWork;
        entry.fileTypes = item.fileTypes;
        entry.fileCount = item.fileCount;
        entry.merged = item.merged;
        entry.hasEbook = item.hasEbook;
        entry.hasAudiobook = item.hasAudiobook;
        entry.workId = item.workId;
        entries.append(entry);
    }
    // Files that are already in the library, and drops without any
    // supported file, get one "skipped" row.
    if (!duplicates.isEmpty() || items.isEmpty()) {
        ImportQueueModel::Entry skipped;
        skipped.title = jobTitle;
        skipped.status = ImportQueueModel::Skipped;
        skipped.fileCount = int(duplicates.size());
        skipped.progress = 1.0;
        for (const QString &path : duplicates) {
            const QString suffix = QFileInfo(path).suffix().toUpper();
            if (!skipped.fileTypes.contains(suffix))
                skipped.fileTypes.append(suffix);
        }
        entries.append(skipped);
    }

    m_items = items;
    m_itemEntries = m_queue.replace(m_current.entryId, entries);
    m_itemEntries.resize(items.size()); // without the "skipped" row
}

void ImportController::onItemStarted(int item)
{
    m_queue.update(m_itemEntries.value(item), [](ImportQueueModel::Entry &entry) {
        entry.status = ImportQueueModel::Copying;
    });
}

void ImportController::onItemProgress(int item, int done, int total)
{
    m_queue.update(m_itemEntries.value(item), [done, total](ImportQueueModel::Entry &entry) {
        entry.progress = total > 0 ? double(done) / total : 1.0;
    });
}

void ImportController::onWorkReady(int item, const Work &work, const QStringList &failed)
{
    bool saved = false;
    if (!work.id.isEmpty()) {
        saved = m_library.saveWork(work);
        if (!saved)
            ++m_saveFailures;
    }
    const bool review = m_items.value(item).needsReview;
    m_queue.update(m_itemEntries.value(item), [&](ImportQueueModel::Entry &entry) {
        entry.progress = 1.0;
        entry.title = work.id.isEmpty() ? entry.title : work.title;
        if (!saved || !failed.isEmpty())
            entry.status = ImportQueueModel::Failed;
        else
            entry.status = review ? ImportQueueModel::NeedsReview : ImportQueueModel::Done;
        // The cover of what was added: the audiobook's if only audio came.
        if (saved) {
            const Format::Kind kind = entry.hasEbook || !entry.hasAudiobook ? work.primaryKind()
                                                                            : Format::Kind::Audiobook;
            const QString cover = work.coverPath(kind);
            entry.coverUrl = cover.isEmpty() ? QUrl() : QUrl::fromLocalFile(cover);
            entry.coverIsSquare = work.coverIsSquare(kind);
        }
    });
}

void ImportController::onFinished(const QList<Work> &, int added, const QStringList &duplicates,
                                  const QStringList &failed)
{
    const int failedCount = int(failed.size()) + m_saveFailures;
    m_running = false;
    m_items.clear();
    m_itemEntries.clear();
    emit importFinished(added, int(duplicates.size()), failedCount);
    if (m_jobs.isEmpty())
        emit busyChanged();
    else
        startNext();
}
