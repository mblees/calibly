#include "importworker.h"

#include <QDirIterator>
#include <QHash>
#include <QFileInfo>
#include <QImage>

#include "importplanner.h"
#include "libraryfolder.h"
#include "metadatareader.h"

namespace {

void setCover(Format &format, const FileMetadata &file, const QString &target)
{
    format.coverPath = LibraryFolder::saveCover(file.cover, target);
    if (!format.coverPath.isEmpty()) {
        const double ratio = double(file.cover.width()) / std::max(1, file.cover.height());
        format.coverIsSquare = ratio > 0.9 && ratio < 1.1;
    }
}

} // namespace

QStringList ImportWorker::collectFiles(const QStringList &paths)
{
    QStringList files;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString file = it.next();
                if (isSupportedFile(file))
                    files.append(file);
            }
        } else if (isSupportedFile(path)) {
            files.append(info.absoluteFilePath());
        }
    }
    files.sort(Qt::CaseInsensitive);
    files.removeDuplicates();
    return files;
}

void ImportWorker::run(const QStringList &paths, const QList<Work> &existing, const QString &libraryRoot)
{
    const QStringList files = collectFiles(paths);
    const int total = int(files.size());

    // 1. Read what the files tell about themselves.
    QList<FileMetadata> metadata;
    metadata.reserve(total);
    for (int i = 0; i < total; ++i) {
        emit progress(i, total, QFileInfo(files.at(i)).fileName());
        metadata.append(readMetadata(files.at(i)));
    }

    // 2. Decide which work each file belongs to.
    ImportPlan plan = planImport(metadata, existing);

    // 3. Copy the files into the library folder and save their covers.
    QStringList failed;
    int added = 0;
    int done = int(plan.duplicates.size());
    // Audiobook format created in this import, per work: further audio files
    // of the same work (an MP3 folder) are joined into it.
    QHash<int, int> audiobookOfWork; // work index -> index in work.formats
    for (const ImportPlan::PlannedFormat &planned : std::as_const(plan.formats)) {
        const FileMetadata &file = planned.metadata;
        emit progress(done++, total, QFileInfo(file.sourcePath).fileName());

        Work &work = plan.works[planned.workIndex];
        QString error;
        const QString target = LibraryFolder::copyInto(file.sourcePath,
                                                       LibraryFolder::workFolder(libraryRoot, work), &error);
        if (target.isEmpty()) {
            qWarning("Import of %s failed: %s", qPrintable(file.sourcePath), qPrintable(error));
            failed.append(file.sourcePath);
            continue;
        }

        const bool isAudiobook = file.kind == Format::Kind::Audiobook;
        if (isAudiobook && audiobookOfWork.contains(planned.workIndex)) {
            Format &format = work.formats[audiobookOfWork.value(planned.workIndex)];
            appendAudioPart(format, file, target);
            if (format.coverPath.isEmpty())
                setCover(format, file, target);
            ++added;
            continue;
        }

        Format format = formatFromFile(file, target);
        format.fileSize = QFileInfo(target).size();
        setCover(format, file, target);
        work.formats.append(format);
        if (isAudiobook)
            audiobookOfWork.insert(planned.workIndex, int(work.formats.size()) - 1);
        ++added;
    }

    // Works without any successfully copied format are not saved.
    QList<Work> works;
    for (const Work &work : std::as_const(plan.works)) {
        if (!work.formats.isEmpty())
            works.append(work);
    }
    emit progress(total, total, QString());
    emit finished(works, added, plan.duplicates, failed);
}
