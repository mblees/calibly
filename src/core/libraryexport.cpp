#include "libraryexport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace LibraryExport {

namespace {

constexpr qint64 kChunkSize = 1024 * 1024;

// Windows file names ignore case: "A.mp3" and "a.mp3" are the same file.
QString nameKey(const QString &name)
{
#ifdef Q_OS_WIN
    return name.toCaseFolded();
#else
    return name;
#endif
}

// fileName, or "base (2).ext", "base (3).ext" … if that is taken.
QString freeName(const QString &fileName, const QSet<QString> &taken)
{
    if (!taken.contains(nameKey(fileName)))
        return fileName;
    const QFileInfo info(fileName);
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty() ? QString() : QLatin1Char('.') + info.suffix();
    for (int n = 2;; ++n) {
        const QString candidate = QStringLiteral("%1 (%2)%3").arg(base).arg(n).arg(suffix);
        if (!taken.contains(nameKey(candidate)))
            return candidate;
    }
}

// Copies source to target in pieces, so progress can be shown and the copy
// can be cancelled. False if it failed or was cancelled; target is removed
// then.
bool copyFile(const QString &source, const QString &target, qint64 &bytes, int files, const Progress &progress,
              const std::atomic_bool &cancel, bool &cancelled)
{
    QFile in(source);
    QFile out(target);
    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::NewOnly | QIODevice::WriteOnly))
        return false;
    QByteArray buffer;
    while (!in.atEnd()) {
        if (cancel) {
            cancelled = true;
            out.close();
            out.remove();
            return false;
        }
        buffer = in.read(kChunkSize);
        if (buffer.isEmpty() && in.error() != QFileDevice::NoError)
            break;
        if (out.write(buffer) != buffer.size()) {
            out.close();
            out.remove();
            return false;
        }
        bytes += buffer.size();
        if (progress)
            progress(bytes, files, QFileInfo(source).fileName());
    }
    if (in.error() != QFileDevice::NoError || !out.flush()) {
        out.close();
        out.remove();
        return false;
    }
    return true;
}

} // namespace

QString subfolderName(Format::Kind kind)
{
    return kind == Format::Kind::Audiobook ? QStringLiteral("Audiobooks") : QStringLiteral("Ebooks");
}

QString folderName(const QDate &date)
{
    return QStringLiteral("calibly-Export %1").arg(date.toString(Qt::ISODate));
}

QString uniqueFolder(const QString &parent, const QString &name)
{
    const QDir dir(parent);
    QString candidate = name;
    for (int n = 2; dir.exists(candidate); ++n)
        candidate = QStringLiteral("%1 (%2)").arg(name).arg(n);
    return QDir::cleanPath(dir.filePath(candidate));
}

QStringList formatFiles(const Format &format)
{
    QStringList files;
    QSet<QString> seen;
    const auto add = [&](const QString &path) {
        if (path.isEmpty())
            return;
        const QString clean = QDir::cleanPath(path);
        if (seen.contains(nameKey(clean)))
            return;
        seen.insert(nameKey(clean));
        files.append(clean);
    };
    add(format.filePath);
    for (const Chapter &chapter : format.chapters)
        add(chapter.filePath);
    return files;
}

Plan plan(const QList<Work> &works, const QString &folder)
{
    Plan result;
    result.folder = QDir::cleanPath(folder);
    QSet<QString> planned;                                  // sources
    QSet<QString> taken[2];                                 // names per subfolder
    for (const Work &work : works) {
        for (const Format &format : work.formats) {
            const int kindIndex = format.kind == Format::Kind::Audiobook ? 1 : 0;
            const QString subfolder = QDir(result.folder).filePath(subfolderName(format.kind));
            for (const QString &source : formatFiles(format)) {
                if (planned.contains(nameKey(source)))
                    continue;
                planned.insert(nameKey(source));
                const QFileInfo info(source);
                if (!info.isFile()) {
                    result.missing.append(source);
                    continue;
                }
                const QString name = freeName(info.fileName(), taken[kindIndex]);
                taken[kindIndex].insert(nameKey(name));
                result.files.append({ source, QDir(subfolder).filePath(name), info.size() });
                result.totalBytes += info.size();
            }
        }
    }
    return result;
}

Outcome run(const Plan &plan, const Progress &progress, const std::atomic_bool &cancel)
{
    Outcome outcome;
    const QFileInfo folderInfo(plan.folder);
    QDir parent = folderInfo.dir();
    if (folderInfo.exists() || !parent.mkpath(QStringLiteral(".")) || !parent.mkdir(folderInfo.fileName())) {
        outcome.result = Result::CannotCreateFolder;
        return outcome;
    }
    QDir folder(plan.folder);
    if (!folder.mkdir(subfolderName(Format::Kind::Ebook)) || !folder.mkdir(subfolderName(Format::Kind::Audiobook))) {
        folder.removeRecursively();
        outcome.result = Result::CannotCreateFolder;
        return outcome;
    }

    qint64 bytes = 0;
    for (const File &file : plan.files) {
        bool cancelled = false;
        if (cancel || !copyFile(file.source, file.target, bytes, outcome.copied, progress, cancel, cancelled)) {
            if (cancel || cancelled) {
                // Only files of this export are in the folder: it was new.
                folder.removeRecursively();
                outcome.result = Result::Cancelled;
            } else {
                outcome.result = Result::CannotCopy;
                outcome.failedFile = file.source;
            }
            return outcome;
        }
        ++outcome.copied;
        if (progress)
            progress(bytes, outcome.copied, QFileInfo(file.source).fileName());
    }
    return outcome;
}

} // namespace LibraryExport
