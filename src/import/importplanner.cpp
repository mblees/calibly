#include "importplanner.h"

#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

#include <algorithm>

#include "audiotimeline.h"
#include "filenameparser.h"
#include "textutils.h"

namespace {

QString simplifiedKey(const QString &text)
{
    QString folded = TextUtils::foldForSearch(text);
    QString key;
    for (const QChar c : folded) {
        if (c.isLetterOrNumber())
            key.append(c);
    }
    return key;
}

void appendUnique(QStringList &list, const QStringList &values)
{
    for (const QString &value : values) {
        if (!value.isEmpty() && !list.contains(value, Qt::CaseInsensitive))
            list.append(value);
    }
}

// Fills the empty fields of the work from the file; lists are merged.
void mergeInto(Work &work, const FileMetadata &file)
{
    if (work.title.isEmpty())
        work.title = file.title;
    if (work.authors.isEmpty())
        work.authors = file.authors;
    appendUnique(work.narrators, file.narrators);
    if (work.series.isEmpty()) {
        work.series = file.series;
        work.seriesIndex = file.seriesIndex;
    } else if (work.seriesIndex == 0) {
        work.seriesIndex = file.seriesIndex;
    }
    if (work.description.isEmpty())
        work.description = file.description;
    appendUnique(work.tags, file.tags);
    work.rating = std::max(work.rating, file.rating);
    if (work.language.isEmpty())
        work.language = file.language;
    if (work.publishedYear == 0)
        work.publishedYear = file.publishedYear;
}

bool hasDuplicate(const Work &work, const FileMetadata &file)
{
    const QString name = QFileInfo(file.sourcePath).fileName();
    return std::any_of(work.formats.cbegin(), work.formats.cend(), [&file, &name](const Format &format) {
        if (format.kind != file.kind)
            return false;
        if (format.fileSize == file.fileSize && file.fileSize > 0)
            return true;
        // Audiobooks of several files: the same file name is the same part.
        const bool severalFiles = std::any_of(format.chapters.cbegin(), format.chapters.cend(),
                                              [](const Chapter &chapter) { return !chapter.filePath.isEmpty(); });
        return severalFiles && std::any_of(format.chapters.cbegin(), format.chapters.cend(), [&](const Chapter &chapter) {
            const QString path = chapter.filePath.isEmpty() ? format.filePath : chapter.filePath;
            return QFileInfo(path).fileName() == name;
        });
    });
}

QList<Chapter> partChapters(const FileMetadata &file, const QString &filePath)
{
    if (file.chapters.isEmpty()) {
        const QString title = file.trackTitle.isEmpty() ? QFileInfo(file.sourcePath).completeBaseName()
                                                        : file.trackTitle;
        return { Chapter{ title, filePath, 0, file.durationMs } };
    }
    QList<Chapter> chapters = file.chapters;
    for (Chapter &chapter : chapters)
        chapter.filePath = filePath;
    return chapters;
}

// Folder that stands for the book: the folder of the file, or its parent for
// disc folders such as "CD 1", "Disc 2", "Teil 3".
QString bookFolder(const QString &filePath)
{
    static const QRegularExpression discFolder(QStringLiteral(R"(^(cd|disc|disk|teil|part)\s*\d+$)"),
                                               QRegularExpression::CaseInsensitiveOption);
    const QString folder = QFileInfo(filePath).absolutePath();
    if (discFolder.match(QFileInfo(folder).fileName().simplified()).hasMatch())
        return QFileInfo(folder).absolutePath(); // the parent; works without the folder existing
    return folder;
}

// Audio files that are usually one part of a longer audiobook.
bool isAudioPart(const FileMetadata &file)
{
    return file.kind == Format::Kind::Audiobook
        && QFileInfo(file.sourcePath).suffix().compare(QLatin1String("m4b"), Qt::CaseInsensitive) != 0;
}

} // namespace

QString workMatchKey(const QString &title, const QStringList &authors)
{
    // "Dungeon Crawler Carl: A LitRPG" and "Dungeon Crawler Carl" are one book.
    const QString mainTitle = title.section(u':', 0, 0);
    const QString author = authors.isEmpty() ? QString() : TextUtils::nameSortKey(authors.first());
    return simplifiedKey(mainTitle) + u'|' + simplifiedKey(author);
}

void groupAudioFolders(QList<FileMetadata> &files)
{
    QHash<QString, QList<int>> partsByFolder;
    QStringList folders; // in order of appearance, so the result does not depend on hashing
    for (int i = 0; i < files.size(); ++i) {
        if (!isAudioPart(files.at(i)))
            continue;
        const QString folder = bookFolder(files.at(i).sourcePath);
        if (!partsByFolder.contains(folder))
            folders.append(folder);
        partsByFolder[folder].append(i);
    }

    for (const QString &folder : std::as_const(folders)) {
        const QList<int> parts = partsByFolder.value(folder);
        if (parts.size() < 2)
            continue;

        // The album tags (the title of tagged audio files) must agree.
        QSet<QString> albums;
        int tagged = -1;
        for (int i : parts) {
            if (!files.at(i).titleFromFileName && !files.at(i).titleFromTrack) {
                albums.insert(simplifiedKey(files.at(i).title));
                if (tagged < 0)
                    tagged = i;
            }
        }
        if (albums.size() > 1)
            continue;

        const FileNameInfo fromFolder = parseFileName(QDir(folder).dirName());
        QString title = fromFolder.title;
        bool titleFromFileName = true;
        if (tagged >= 0) {
            title = files.at(tagged).title;
            titleFromFileName = false;
        }

        QStringList authors;
        bool authorsFromFileName = false;
        for (int i : parts) {
            if (!files.at(i).authorsFromFileName && !files.at(i).authors.isEmpty()) {
                authors = files.at(i).authors;
                break;
            }
        }
        if (authors.isEmpty() && !fromFolder.author.isEmpty()) {
            authors = { fromFolder.author };
            authorsFromFileName = true;
        }

        for (int i : parts) {
            FileMetadata &file = files[i];
            file.title = title;
            file.titleFromFileName = titleFromFileName;
            file.titleFromTrack = false;
            file.authors = authors;
            file.authorsFromFileName = authorsFromFileName;
            if (file.series.isEmpty() && !fromFolder.seriesCode.isEmpty()) {
                file.series = fromFolder.seriesCode;
                file.seriesIndex = fromFolder.seriesIndex;
            }
        }
    }
}

bool needsReview(const FileMetadata &file)
{
    return file.titleFromFileName && file.authors.isEmpty();
}

ImportPlan planImport(QList<FileMetadata> files, const QList<Work> &existing, const QString &targetWorkId)
{
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(files.begin(), files.end(), [&collator](const FileMetadata &a, const FileMetadata &b) {
        if (a.kind != b.kind)
            return a.kind == Format::Kind::Ebook;
        if (a.kind == Format::Kind::Ebook)
            return false; // keep the order
        const QFileInfo infoA(a.sourcePath);
        const QFileInfo infoB(b.sourcePath);
        if (const int folder = collator.compare(infoA.path(), infoB.path()); folder != 0)
            return folder < 0;
        if (a.trackNumber != b.trackNumber && a.trackNumber > 0 && b.trackNumber > 0)
            return a.trackNumber < b.trackNumber;
        return collator.compare(infoA.fileName(), infoB.fileName()) < 0;
    });

    QHash<QString, int> existingByKey;
    int targetIndex = -1; // index in existing of the work the files were dropped onto
    for (int i = 0; i < existing.size(); ++i) {
        existingByKey.insert(workMatchKey(existing.at(i).title, existing.at(i).authors), i);
        if (!targetWorkId.isEmpty() && existing.at(i).id == targetWorkId)
            targetIndex = i;
    }

    ImportPlan plan;
    QHash<QString, int> plannedByKey;      // match key -> index in plan.works
    QHash<QString, int> plannedByBaseName; // folded base name -> index in plan.works
    QHash<int, int> plannedFromExisting;   // index in existing -> index in plan.works

    for (const FileMetadata &file : std::as_const(files)) {
        const QString key = workMatchKey(file.title, file.authors);
        // The same base name ("DCC 01 - … .epub" / ".m4b") is the same book,
        // but not for parts of an audiobook ("01.mp3" is in many folders).
        const QString baseName = isAudioPart(file) ? QString()
                                                   : simplifiedKey(QFileInfo(file.sourcePath).completeBaseName());

        int workIndex = targetIndex >= 0 || baseName.isEmpty() ? -1 : plannedByBaseName.value(baseName, -1);
        if (workIndex < 0 && targetIndex < 0)
            workIndex = plannedByKey.value(key, -1);
        if (workIndex < 0 && (targetIndex >= 0 || existingByKey.contains(key))) {
            const int existingIndex = targetIndex >= 0 ? targetIndex : existingByKey.value(key);
            workIndex = plannedFromExisting.value(existingIndex, -1);
            if (workIndex < 0) {
                plan.works.append(existing.at(existingIndex));
                workIndex = int(plan.works.size()) - 1;
                plannedFromExisting.insert(existingIndex, workIndex);
            }
        }
        if (workIndex >= 0 && hasDuplicate(plan.works.at(workIndex), file)) {
            plan.duplicates.append(file.sourcePath);
            continue;
        }
        if (workIndex < 0) {
            Work work;
            work.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            work.added = QDateTime::currentDateTime();
            plan.works.append(work);
            workIndex = int(plan.works.size()) - 1;
        }

        mergeInto(plan.works[workIndex], file);
        plannedByKey.insert(key, workIndex);
        plannedByKey.insert(workMatchKey(plan.works.at(workIndex).title, plan.works.at(workIndex).authors), workIndex);
        if (!baseName.isEmpty())
            plannedByBaseName.insert(baseName, workIndex);
        plan.formats.append({ workIndex, file });
    }
    return plan;
}

Format formatFromFile(const FileMetadata &file, const QString &target)
{
    Format format;
    format.kind = file.kind;
    format.filePath = target;
    format.fileSize = file.fileSize;
    format.pages = file.pages;
    format.durationSeconds = file.durationSeconds;
    format.publisher = file.publisher;
    format.isbn = file.isbn;
    if (file.kind == Format::Kind::Audiobook)
        format.chapters = partChapters(file, QString()); // the format's own file
    return format;
}

void appendAudioPart(Format &format, const FileMetadata &file, const QString &target)
{
    format.chapters += partChapters(file, target);
    format.fileSize += file.fileSize;
    const qint64 totalMs = AudioTimeline(format).durationMs();
    format.durationSeconds = int((totalMs + 500) / 1000);
}
