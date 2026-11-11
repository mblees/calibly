#include "libraryfolder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace LibraryFolder {

QString safeName(const QString &name)
{
    QString result;
    for (const QChar c : name) {
        if (c.unicode() < 32 || QStringLiteral("<>:\"/\\|?*").contains(c))
            result.append(u'_');
        else
            result.append(c);
    }
    result = result.simplified();
    // Windows does not allow trailing dots or spaces; keep names reasonably short.
    while (result.endsWith(u'.') || result.endsWith(u' '))
        result.chop(1);
    result = result.left(80).trimmed();
    return result.isEmpty() ? QStringLiteral("_") : result;
}

QString workFolder(const QString &libraryRoot, const Work &work)
{
    for (const Format &format : work.formats) {
        if (!format.filePath.isEmpty())
            return QFileInfo(format.filePath).absolutePath();
    }
    const QString author = work.authors.isEmpty() ? QStringLiteral("Unbekannt") : work.authors.first();
    return QDir(libraryRoot).filePath(safeName(author) + u'/' + safeName(work.title));
}

QString copyInto(const QString &sourcePath, const QString &folder, QString *error)
{
    if (!QDir().mkpath(folder)) {
        if (error)
            *error = QStringLiteral("cannot create folder %1").arg(folder);
        return QString();
    }
    const QFileInfo source(sourcePath);
    const QString baseName = safeName(source.completeBaseName());
    const QString suffix = source.suffix();

    for (int number = 1; number < 1000; ++number) {
        const QString name = number == 1 ? baseName : QStringLiteral("%1 (%2)").arg(baseName).arg(number);
        const QString target = QDir(folder).filePath(suffix.isEmpty() ? name : name + u'.' + suffix);
        const QFileInfo existing(target);
        if (existing.exists()) {
            if (existing.size() == source.size())
                return target; // already copied before
            continue;
        }
        if (QFile::copy(sourcePath, target))
            return target;
        if (error)
            *error = QStringLiteral("cannot copy to %1").arg(target);
        return QString();
    }
    if (error)
        *error = QStringLiteral("too many files with the same name");
    return QString();
}

QString saveCover(const QImage &cover, const QString &formatFilePath)
{
    if (cover.isNull())
        return QString();
    // Large enough for the detail view, small enough to load quickly.
    constexpr int maxSide = 1200;
    const QImage scaled = cover.width() > maxSide || cover.height() > maxSide
                              ? cover.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                              : cover;
    const QString path = formatFilePath + QStringLiteral(".cover.jpg");
    return scaled.convertToFormat(QImage::Format_RGB32).save(path, "JPG", 90) ? path : QString();
}

} // namespace LibraryFolder
