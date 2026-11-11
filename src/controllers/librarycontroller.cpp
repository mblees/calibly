#include "librarycontroller.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>

#include "importworker.h"
#include "librarybackend.h"
#include "metadatareader.h"

LibraryController::LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    // The worker lives in its own thread; results come back as queued signals.
    m_importWorker = new ImportWorker;
    m_importWorker->moveToThread(&m_importThread);
    connect(&m_importThread, &QThread::finished, m_importWorker, &QObject::deleteLater);
    connect(m_importWorker, &ImportWorker::progress, this, &LibraryController::onImportProgress);
    connect(m_importWorker, &ImportWorker::finished, this, &LibraryController::onImportFinished);
    m_importThread.setObjectName(QStringLiteral("calibly import"));
    m_importThread.start();

    reload();
}

LibraryController::~LibraryController()
{
    // A running import finishes its current step; files already copied stay.
    m_importThread.quit();
    m_importThread.wait();
    s_instance = nullptr;
}

LibraryController *LibraryController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool LibraryController::canImport() const
{
    return !m_backend.libraryRoot().isEmpty();
}

QString LibraryController::libraryRoot() const
{
    return m_backend.libraryRoot();
}

QStringList LibraryController::fileNameFilters() const
{
    QStringList patterns;
    for (const QString &suffix : ebookSuffixes() + audiobookSuffixes())
        patterns.append(QStringLiteral("*.") + suffix);
    return patterns;
}

void LibraryController::reload()
{
    m_works.setWorks(m_backend.loadWorks());
}

bool LibraryController::openFormat(const QString &workId, const QString &kind)
{
    const Work *work = m_works.workAt(m_works.rowOf(workId));
    if (!work)
        return false;
    const Format *format = work->format(kind == QLatin1String("audiobook") ? Format::Kind::Audiobook
                                                                           : Format::Kind::Ebook);
    if (!format || !QFileInfo::exists(format->filePath))
        return false;
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(format->filePath).absoluteFilePath())))
        return false;

    // Remember for "recently opened".
    Work opened = *work;
    opened.lastOpened = QDateTime::currentDateTime();
    saveWork(opened);
    return true;
}

bool LibraryController::saveWork(const Work &work)
{
    if (!m_backend.saveWork(work)) {
        qWarning("Could not save %s to the library", qPrintable(work.title));
        return false;
    }
    m_works.updateWork(work);
    return true;
}

void LibraryController::importFiles(const QList<QUrl> &urls)
{
    if (!canImport() || m_importing || urls.isEmpty())
        return;
    QStringList paths;
    for (const QUrl &url : urls)
        paths.append(url.isLocalFile() ? url.toLocalFile() : url.toString());

    m_importing = true;
    m_importDone = 0;
    m_importTotal = 0;
    m_importFileName.clear();
    emit importStateChanged();

    const QList<Work> existing = m_backend.loadWorks();
    const QString root = m_backend.libraryRoot();
    QMetaObject::invokeMethod(m_importWorker, [worker = m_importWorker, paths, existing, root] {
        worker->run(paths, existing, root);
    }, Qt::QueuedConnection);
}

void LibraryController::onImportProgress(int done, int total, const QString &fileName)
{
    m_importDone = done;
    m_importTotal = total;
    m_importFileName = fileName;
    emit importStateChanged();
}

void LibraryController::onImportFinished(const QList<Work> &works, int added, const QStringList &duplicates,
                                         const QStringList &failed)
{
    int failedCount = int(failed.size());
    for (const Work &work : works) {
        if (!m_backend.saveWork(work)) {
            qWarning("Could not save %s to the library", qPrintable(work.title));
            ++failedCount;
        }
    }
    reload();

    m_importing = false;
    emit importStateChanged();
    emit importFinished(added, int(duplicates.size()), failedCount);
}
