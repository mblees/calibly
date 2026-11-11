#include "importplanner.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QUuid>

#include <algorithm>

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
    return std::any_of(work.formats.cbegin(), work.formats.cend(), [&file](const Format &format) {
        return format.kind == file.kind && format.fileSize == file.fileSize && file.fileSize > 0;
    });
}

} // namespace

QString workMatchKey(const QString &title, const QStringList &authors)
{
    // "Dungeon Crawler Carl: A LitRPG" and "Dungeon Crawler Carl" are one book.
    const QString mainTitle = title.section(u':', 0, 0);
    const QString author = authors.isEmpty() ? QString() : TextUtils::nameSortKey(authors.first());
    return simplifiedKey(mainTitle) + u'|' + simplifiedKey(author);
}

ImportPlan planImport(QList<FileMetadata> files, const QList<Work> &existing)
{
    std::stable_sort(files.begin(), files.end(), [](const FileMetadata &a, const FileMetadata &b) {
        return a.kind == Format::Kind::Ebook && b.kind == Format::Kind::Audiobook;
    });

    QHash<QString, int> existingByKey;
    for (int i = 0; i < existing.size(); ++i)
        existingByKey.insert(workMatchKey(existing.at(i).title, existing.at(i).authors), i);

    ImportPlan plan;
    QHash<QString, int> plannedByKey;      // match key -> index in plan.works
    QHash<QString, int> plannedByBaseName; // folded base name -> index in plan.works
    QHash<int, int> plannedFromExisting;   // index in existing -> index in plan.works

    for (const FileMetadata &file : std::as_const(files)) {
        const QString key = workMatchKey(file.title, file.authors);
        const QString baseName = simplifiedKey(QFileInfo(file.sourcePath).completeBaseName());

        int workIndex = plannedByBaseName.value(baseName, -1);
        if (workIndex < 0)
            workIndex = plannedByKey.value(key, -1);
        if (workIndex < 0 && existingByKey.contains(key)) {
            const int existingIndex = existingByKey.value(key);
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
        plannedByBaseName.insert(baseName, workIndex);
        plan.formats.append({ workIndex, file });
    }
    return plan;
}
