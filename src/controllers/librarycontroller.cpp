#include "librarycontroller.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>

#include "librarybackend.h"
#include "metadatareader.h"

LibraryController::LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    reload();
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
