#include "librarycontroller.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

#include "librarybackend.h"

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
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(format->filePath).absoluteFilePath()));
}
