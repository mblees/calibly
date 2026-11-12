#include "workmerge.h"

#include <QCollator>

#include <algorithm>
#include <cmath>

#include "audiotimeline.h"

namespace {

constexpr qint64 kFinishedMarginMs = 1000; // this close to the end counts as finished

void appendUnique(QStringList &list, const QStringList &values)
{
    for (const QString &value : values) {
        if (!value.isEmpty() && !list.contains(value, Qt::CaseInsensitive))
            list.append(value);
    }
}

// The chapters of the format for a joined audiobook: every chapter names its
// file, and a format without chapters is one chapter.
QList<Chapter> partChapters(const Format &format, bool firstPart)
{
    QList<Chapter> chapters = format.chapters;
    if (chapters.isEmpty())
        chapters.append({ QString(), QString(), 0, qint64(format.durationSeconds) * 1000 });
    // The chapters of the first part belong to the file of the joined format.
    if (!firstPart) {
        for (Chapter &chapter : chapters) {
            if (chapter.filePath.isEmpty())
                chapter.filePath = format.filePath;
        }
    }
    return chapters;
}

// Where the format was stopped, in ms from its start; 0 = never played.
qint64 positionOf(const Format &format, qint64 durationMs)
{
    if (format.finished())
        return durationMs;
    if (format.positionMs > 0)
        return std::min(format.positionMs, durationMs);
    return qint64(std::llround(format.progress * double(durationMs)));
}

} // namespace

int mergeTarget(const QList<Work> &works)
{
    for (int i = 0; i < works.size(); ++i) {
        if (works.at(i).hasEbook())
            return i;
    }
    return works.isEmpty() ? -1 : 0;
}

Work mergeWorks(const QList<Work> &works)
{
    const int target = mergeTarget(works);
    if (target < 0)
        return Work();

    Work merged = works.at(target);
    for (int i = 0; i < works.size(); ++i) {
        if (i == target)
            continue;
        const Work &other = works.at(i);
        if (merged.title.isEmpty())
            merged.title = other.title;
        if (merged.authors.isEmpty())
            merged.authors = other.authors;
        appendUnique(merged.narrators, other.narrators);
        if (merged.series.isEmpty()) {
            merged.series = other.series;
            merged.seriesIndex = other.seriesIndex;
        } else if (merged.seriesIndex == 0 && merged.series.compare(other.series, Qt::CaseInsensitive) == 0) {
            merged.seriesIndex = other.seriesIndex;
        }
        if (merged.description.isEmpty())
            merged.description = other.description;
        appendUnique(merged.tags, other.tags);
        merged.rating = std::max(merged.rating, other.rating);
        if (merged.language.isEmpty())
            merged.language = other.language;
        if (merged.publishedYear == 0)
            merged.publishedYear = other.publishedYear;
        if (other.added.isValid() && (!merged.added.isValid() || other.added < merged.added))
            merged.added = other.added;
        if (other.lastOpened.isValid() && (!merged.lastOpened.isValid() || other.lastOpened > merged.lastOpened))
            merged.lastOpened = other.lastOpened;
        merged.formats += other.formats;
    }
    joinAudiobooks(merged);
    return merged;
}

bool joinAudiobooks(Work &work)
{
    QList<Format> parts;
    int firstIndex = -1;
    for (int i = 0; i < work.formats.size(); ++i) {
        if (work.formats.at(i).kind != Format::Kind::Audiobook)
            continue;
        if (firstIndex < 0)
            firstIndex = i;
        parts.append(work.formats.at(i));
    }
    if (parts.size() < 2)
        return false;

    QCollator collator;
    collator.setNumericMode(true); // "Teil 2" before "Teil 10"
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(parts.begin(), parts.end(), [&collator](const Format &a, const Format &b) {
        return collator.compare(a.filePath, b.filePath) < 0;
    });

    Format joined = parts.first();
    joined.chapters.clear();
    joined.fileSize = 0;
    joined.fileMissing = false;
    qint64 offsetMs = 0;   // start of the current part in the joined audiobook
    qint64 positionMs = 0;
    for (int i = 0; i < parts.size(); ++i) {
        const Format &part = parts.at(i);
        const qint64 durationMs = AudioTimeline(part).durationMs();
        joined.chapters += partChapters(part, i == 0);
        joined.fileSize += part.fileSize;
        joined.fileMissing = joined.fileMissing || part.fileMissing;
        if (joined.coverPath.isEmpty() && !part.coverPath.isEmpty()) {
            joined.coverPath = part.coverPath;
            joined.coverIsSquare = part.coverIsSquare;
        }
        if (joined.publisher.isEmpty())
            joined.publisher = part.publisher;
        if (joined.isbn.isEmpty())
            joined.isbn = part.isbn;
        if (part.lastOpened.isValid() && (!joined.lastOpened.isValid() || part.lastOpened > joined.lastOpened))
            joined.lastOpened = part.lastOpened;
        // The last part that was listened to decides where to continue.
        if (const qint64 position = positionOf(part, durationMs); position > 0)
            positionMs = offsetMs + position;
        offsetMs += durationMs;
    }

    const qint64 totalMs = AudioTimeline(joined).durationMs();
    joined.durationSeconds = int((totalMs + 500) / 1000);
    joined.positionMs = std::clamp<qint64>(positionMs, 0, totalMs);
    joined.progress = totalMs <= 0 ? 0.0
                    : joined.positionMs > 0 && joined.positionMs >= totalMs - kFinishedMarginMs ? 1.0
                    : double(joined.positionMs) / double(totalMs);

    // The joined audiobook takes the place of the first one.
    QList<Format> formats;
    for (int i = 0; i < work.formats.size(); ++i) {
        if (i == firstIndex)
            formats.append(joined);
        else if (work.formats.at(i).kind != Format::Kind::Audiobook)
            formats.append(work.formats.at(i));
    }
    work.formats = formats;
    return true;
}
