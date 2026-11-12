#include "importworker.h"

#include <QDirIterator>
#include <QHash>
#include <QFileInfo>
#include <QImage>
#include <QSet>

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

QString upperSuffix(const QString &path)
{
    return QFileInfo(path).suffix().toUpper();
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
        } else if (isSupportedFile(path, true)) {
            files.append(info.absoluteFilePath());
        }
    }
    files.sort(Qt::CaseInsensitive);
    files.removeDuplicates();
    return files;
}

PathSummary ImportWorker::inspectPaths(const QStringList &paths, int limit)
{
    PathSummary summary;
    if (!paths.isEmpty())
        summary.name = QFileInfo(paths.first()).fileName();

    int looked = 0;
    const auto count = [&summary](const QString &file, bool picked) {
        if (!isSupportedFile(file, picked))
            return;
        const QString suffix = upperSuffix(file);
        summary.countBySuffix[suffix] += 1;
        if (audiobookSuffixes().contains(suffix.toLower()))
            ++summary.audiobookCount;
        else
            ++summary.ebookCount;
    };
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext() && looked < limit) {
                count(it.next(), false);
                ++looked;
            }
        } else {
            count(path, true);
            ++looked;
        }
        if (looked >= limit)
            break;
    }
    return summary;
}

void ImportWorker::run(const QStringList &paths, const QList<Work> &existing, const QString &libraryRoot,
                       const QString &targetWorkId)
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

    // 2. Decide which work each file belongs to; the files of an audiobook
    //    folder become one work.
    groupAudioFolders(metadata);
    ImportPlan plan = planImport(metadata, existing, targetWorkId);

    // 3. One queue item per work that gets files, in the order of the plan.
    QSet<QString> existingIds;
    for (const Work &work : existing)
        existingIds.insert(work.id);
    QList<QList<int>> formatsOfWork(plan.works.size()); // work index -> indexes into plan.formats
    for (int i = 0; i < plan.formats.size(); ++i)
        formatsOfWork[plan.formats.at(i).workIndex].append(i);

    QList<ImportItem> items;
    QList<int> workOfItem;
    for (int w = 0; w < plan.works.size(); ++w) {
        const QList<int> &formats = formatsOfWork.at(w);
        if (formats.isEmpty())
            continue;
        ImportItem item;
        item.workId = plan.works.at(w).id;
        item.title = plan.works.at(w).title;
        item.newWork = !existingIds.contains(item.workId);
        item.fileCount = int(formats.size());
        item.needsReview = item.newWork;
        int audioFiles = 0;
        for (int f : formats) {
            const FileMetadata &file = plan.formats.at(f).metadata;
            const QString suffix = upperSuffix(file.sourcePath);
            if (!item.fileTypes.contains(suffix))
                item.fileTypes.append(suffix);
            if (file.kind == Format::Kind::Audiobook) {
                item.hasAudiobook = true;
                ++audioFiles;
            } else {
                item.hasEbook = true;
            }
            item.needsReview = item.needsReview && needsReview(file);
        }
        item.merged = audioFiles > 1;
        items.append(item);
        workOfItem.append(w);
    }
    emit planned(items, plan.duplicates);

    // 4. Copy the files into the library folder and save their covers.
    QStringList failed;
    QList<Work> works;
    int added = 0;
    int done = int(plan.duplicates.size());
    for (int i = 0; i < items.size(); ++i) {
        emit itemStarted(i);
        Work &work = plan.works[workOfItem.at(i)];
        const QList<int> &formats = formatsOfWork.at(workOfItem.at(i));
        // Audiobook format created in this import: further audio files of
        // the same work (an MP3 folder) are joined into it.
        int audiobookFormat = -1;
        QStringList itemFailed;
        for (int j = 0; j < formats.size(); ++j) {
            const FileMetadata &file = plan.formats.at(formats.at(j)).metadata;
            emit itemProgress(i, j, int(formats.size()));
            emit progress(done++, total, QFileInfo(file.sourcePath).fileName());

            QString error;
            const QString target = LibraryFolder::copyInto(file.sourcePath,
                                                           LibraryFolder::workFolder(libraryRoot, work), &error);
            if (target.isEmpty()) {
                qWarning("Import of %s failed: %s", qPrintable(file.sourcePath), qPrintable(error));
                itemFailed.append(file.sourcePath);
                continue;
            }

            if (file.kind == Format::Kind::Audiobook && audiobookFormat >= 0) {
                Format &format = work.formats[audiobookFormat];
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
            if (file.kind == Format::Kind::Audiobook)
                audiobookFormat = int(work.formats.size()) - 1;
            ++added;
        }
        emit itemProgress(i, int(formats.size()), int(formats.size()));
        failed += itemFailed;

        // A new work without any successfully copied format is not saved.
        const bool copiedAny = itemFailed.size() < formats.size();
        if (copiedAny && !work.formats.isEmpty())
            works.append(work);
        emit workReady(i, copiedAny ? work : Work(), itemFailed);
    }

    emit progress(total, total, QString());
    emit finished(works, added, plan.duplicates, failed);
}
