#include "importworker.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QImage>

#include "importplanner.h"
#include "libraryfolder.h"
#include "metadatareader.h"

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

        Format format;
        format.kind = file.kind;
        format.filePath = target;
        format.fileSize = QFileInfo(target).size();
        format.pages = file.pages;
        format.durationSeconds = file.durationSeconds;
        format.publisher = file.publisher;
        format.isbn = file.isbn;
        format.coverPath = LibraryFolder::saveCover(file.cover, target);
        if (!format.coverPath.isEmpty()) {
            const double ratio = double(file.cover.width()) / std::max(1, file.cover.height());
            format.coverIsSquare = ratio > 0.9 && ratio < 1.1;
        }
        work.formats.append(format);
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
