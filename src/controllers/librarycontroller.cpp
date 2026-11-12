#include "librarycontroller.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <algorithm>

#include "librarybackend.h"
#include "libraryfolder.h"
#include "librarylock.h"
#include "metadatareader.h"
#include "sqlitebackend.h"
#include "workmerge.h"

LibraryController::LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_backend(&backend)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    m_libraryPath = backend.libraryRoot();
    reload();
}

LibraryController::LibraryController(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

LibraryController::~LibraryController()
{
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
    return !libraryRoot().isEmpty();
}

QString LibraryController::libraryRoot() const
{
    return m_backend ? m_backend->libraryRoot() : QString();
}

LibraryController::LibraryError LibraryController::openLibrary(const QString &folder, bool create)
{
    const QString path = QDir::cleanPath(QFileInfo(folder).absoluteFilePath());
    if (m_ownedBackend && QFileInfo(path) == QFileInfo(libraryRoot()))
        return NoError; // already open

    // Fails without changing anything: kept if a library is open, shown
    // otherwise.
    const auto failed = [this, &path](LibraryError error) {
        if (!m_backend) {
            m_libraryError = error;
            m_libraryPath = path;
            emit libraryChanged();
        }
        return error;
    };

    const bool exists = QFileInfo::exists(SqliteBackend::databasePath(path));
    if (!exists && !create)
        return failed(NotFound);
    if (!QDir().mkpath(path))
        return failed(CannotCreate);
    auto lock = std::make_unique<LibraryLock>(path);
    switch (lock->acquire()) {
    case LibraryLock::Acquired:
        break;
    case LibraryLock::HeldElsewhere:
        return failed(InUse);
    case LibraryLock::Failed:
        return failed(exists ? InUse : CannotCreate);
    }

    auto backend = std::make_unique<SqliteBackend>(path);
    if (!backend->open(nullptr, create)) {
        switch (backend->openError()) {
        case SqliteBackend::OpenError::NotFound:
            return failed(NotFound);
        case SqliteBackend::OpenError::Damaged:
            return failed(Damaged);
        case SqliteBackend::OpenError::NewerVersion:
            return failed(NewerVersion);
        case SqliteBackend::OpenError::CannotUpdate:
            return failed(CannotUpdate);
        case SqliteBackend::OpenError::CannotCreate:
        case SqliteBackend::OpenError::None:
            return failed(CannotCreate);
        }
    }

    // Whoever uses a work of the old library (player, reader) lets go first.
    QStringList ids;
    for (int row = 0; row < m_works.rowCount(); ++row)
        ids.append(m_works.workAt(row)->id);
    if (!ids.isEmpty())
        emit worksAboutToChange(ids);

    connect(lock.get(), &LibraryLock::activationRequested, this, &LibraryController::activationRequested);
    m_ownedBackend = std::move(backend);
    m_backend = m_ownedBackend.get();
    m_lock = std::move(lock);
    m_libraryError = NoError;
    m_libraryPath = path;
    if (m_rememberLibrary)
        QSettings().setValue(QStringLiteral("library/path"), path);
    reload();
    emit libraryChanged();
    return NoError;
}

LibraryController::LibraryError LibraryController::openLibraryUrl(const QUrl &folder, bool create)
{
    return openLibrary(folder.toLocalFile(), create);
}

LibraryController::LibraryError LibraryController::retryLibrary()
{
    return openLibrary(m_libraryPath, false);
}

QStringList LibraryController::fileNameFilters() const
{
    QStringList patterns;
    for (const QString &suffix : ebookSuffixes() + textSuffixes() + audiobookSuffixes())
        patterns.append(QStringLiteral("*.") + suffix);
    return patterns;
}

void LibraryController::reload()
{
    m_works.setWorks(m_backend ? m_backend->loadWorks() : QList<Work>());
}

bool LibraryController::openFormat(const QString &workId, const QString &kind)
{
    const Work *work = m_works.workAt(m_works.rowOf(workId));
    if (!work)
        return false;
    const int index = work->formatIndex(kind == QLatin1String("audiobook") ? Format::Kind::Audiobook
                                                                           : Format::Kind::Ebook);
    if (index < 0)
        return false;
    const QString filePath = work->formats.at(index).filePath;
    if (!QFileInfo::exists(filePath)) {
        setFileMissing(workId, index, true);
        return false;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath())))
        return false;

    // Remember for "recently opened" and "Continue".
    Work opened = *work;
    opened.markOpened(index, QDateTime::currentDateTime());
    opened.formats[index].fileMissing = false;
    saveWork(opened);
    return true;
}

bool LibraryController::setFinished(const QString &workId, const QString &kind, bool finished)
{
    const Work *work = m_works.workAt(m_works.rowOf(workId));
    const int index = work ? work->formatIndex(kind == QLatin1String("audiobook") ? Format::Kind::Audiobook
                                                                                  : Format::Kind::Ebook)
                           : -1;
    if (index < 0)
        return false;
    if (work->formats.at(index).finished() == finished)
        return true;

    Work changed = *work;
    Format &format = changed.formats[index];
    if (finished) {
        format.progress = 1.0;
    } else {
        // Back to where it was: the listening position, or the start if
        // the position is the end (or unknown, as for ebooks).
        const qint64 durationMs = qint64(format.durationSeconds) * 1000;
        const double progress = durationMs > 0 ? double(format.positionMs) / double(durationMs) : 0.0;
        if (progress >= 0.999) {
            format.progress = 0.0;
            format.positionMs = 0;
        } else {
            format.progress = std::clamp(progress, 0.0, 1.0);
        }
    }
    return saveWork(changed);
}

QList<Work> LibraryController::worksById(const QStringList &workIds) const
{
    QList<Work> works;
    for (const QString &id : workIds) {
        const Work *work = m_works.workAt(m_works.rowOf(id));
        const bool listed = std::any_of(works.cbegin(), works.cend(), [&id](const Work &w) { return w.id == id; });
        if (work && !listed)
            works.append(*work);
    }
    return works;
}

QString LibraryController::mergeTitle(const QStringList &workIds) const
{
    const QList<Work> works = worksById(workIds);
    const int target = mergeTarget(works);
    return target < 0 ? QString() : works.at(target).title;
}

QString LibraryController::mergeWorks(const QStringList &workIds)
{
    if (worksById(workIds).isEmpty())
        return QString();
    // E.g. the player stores its position and unloads the audiobook, so the
    // works are read only afterwards.
    emit worksAboutToChange(workIds);

    const QList<Work> works = worksById(workIds);
    const Work merged = ::mergeWorks(works);
    if (merged.id.isEmpty() || !saveWork(merged))
        return QString();

    for (const Work &work : works) {
        if (work.id == merged.id)
            continue;
        // The files stay: they now belong to the formats of the merged work.
        if (!m_backend->removeWork(work.id))
            qWarning("Could not remove %s from the library", qPrintable(work.title));
        m_works.removeWork(work.id);
    }
    return merged.id;
}

int LibraryController::removeWorks(const QStringList &workIds)
{
    if (worksById(workIds).isEmpty())
        return 0;
    emit worksAboutToChange(workIds);

    int removed = 0;
    for (const Work &work : worksById(workIds)) {
        if (!m_backend->removeWork(work.id)) {
            qWarning("Could not remove %s from the library", qPrintable(work.title));
            continue;
        }
        m_works.removeWork(work.id);
        ++removed;
        // Only a real library owns its files.
        const QStringList failed = LibraryFolder::removeWorkFiles(m_backend->libraryRoot(), work, m_trashFiles);
        for (const QString &path : failed)
            qWarning("Could not remove the file %s", qPrintable(path));
    }
    return removed;
}

void LibraryController::setFileMissing(const QString &workId, int formatIndex, bool missing)
{
    const Work *work = m_works.workAt(m_works.rowOf(workId));
    if (!canImport() || !work || formatIndex < 0 || formatIndex >= work->formats.size()
        || work->formats.at(formatIndex).fileMissing == missing)
        return;
    Work changed = *work;
    changed.formats[formatIndex].fileMissing = missing;
    m_works.updateWork(changed); // not stored, see Format::fileMissing
}

bool LibraryController::saveWork(const Work &work)
{
    if (!m_backend || !m_backend->saveWork(work)) {
        qWarning("Could not save %s to the library", qPrintable(work.title));
        return false;
    }
    m_works.updateWork(work);
    return true;
}
