// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "workmerge.h"

#include <QCollator>

#include <algorithm>
#include <cmath>

#include <climits>

#include "audiotimeline.h"
#include "libraryexport.h"

namespace {

constexpr qint64 kFinishedMarginMs = 1000; // this close to the end counts as finished

QCollator naturalCollator()
{
    QCollator collator;
    collator.setNumericMode(true); // "Teil 2" before "Teil 10"
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    return collator;
}

// Sort key of a part as a volume: its series number, unknown ones last.
int volumeKey(const WorkPart &part)
{
    return part.seriesIndex > 0 ? part.seriesIndex : INT_MAX;
}

// Index of the part of a format (its file), parts that are not found last.
int partKey(const Work &work, const QString &filePath)
{
    const int part = work.partOf(filePath);
    return part >= 0 ? part : INT_MAX;
}

// A new work for a part: its metadata, no formats yet.
Work workFromPart(const WorkPart &part)
{
    Work work;
    work.id = part.id;
    work.title = part.title;
    work.authors = part.authors;
    work.narrators = part.narrators;
    work.series = part.series;
    work.seriesIndex = part.seriesIndex;
    work.description = part.description;
    work.tags = part.tags;
    work.rating = part.rating;
    work.language = part.language;
    work.publishedYear = part.publishedYear;
    work.added = part.added;
    return work;
}

// The metadata of from, without id, formats and parts.
void copyMetadata(Work &to, const Work &from)
{
    to.title = from.title;
    to.authors = from.authors;
    to.narrators = from.narrators;
    to.series = from.series;
    to.seriesIndex = from.seriesIndex;
    to.description = from.description;
    to.tags = from.tags;
    to.rating = from.rating;
    to.language = from.language;
    to.publishedYear = from.publishedYear;
    to.added = from.added;
}

// Cuts a joined audiobook into the files of each part: one format per run
// of chapters whose files belong to the same part. Returns pairs of part
// index (-1 = no part) and format.
QList<std::pair<int, Format>> cutAudiobook(const Work &work, const Format &format)
{
    QList<std::pair<int, Format>> pieces;
    const AudioTimeline timeline(format);
    if (format.chapters.isEmpty() || timeline.chapters().size() != format.chapters.size()) {
        pieces.append({ work.partOf(format.filePath), format });
        return pieces;
    }
    const qint64 positionMs = format.finished() ? timeline.durationMs() : format.positionMs;
    for (int i = 0; i < format.chapters.size(); ++i) {
        const AudioTimeline::Entry &entry = timeline.chapters().at(i);
        const QString file = timeline.parts().at(entry.part).filePath;
        const int part = work.partOf(file);
        if (pieces.isEmpty() || pieces.last().first != part) {
            Format piece = format;
            piece.filePath = file;
            piece.chapters.clear();
            piece.durationSeconds = 0;
            piece.positionMs = entry.startMs; // start on the joined axis, made relative below
            piece.progress = 0.0;
            piece.fileSize = 0;               // unknown: the files are not looked at here
            piece.lastOpened = QDateTime();
            pieces.append({ part, piece });
        }
        Format &piece = pieces.last().second;
        Chapter chapter = format.chapters.at(i);
        chapter.filePath = file == piece.filePath ? QString() : file;
        piece.chapters.append(chapter);
        piece.durationSeconds += int((entry.durationMs + 500) / 1000); // corrected below
    }
    for (auto &[part, piece] : pieces) {
        const qint64 startMs = piece.positionMs;
        qint64 durationMs = 0;
        for (const Chapter &chapter : std::as_const(piece.chapters))
            durationMs += chapter.durationMs;
        piece.durationSeconds = int((durationMs + 500) / 1000);
        // A file without chapters of its own was one chapter, untitled or
        // with the title of its part (see joinAudiobooks()).
        const QString partTitle = part >= 0 ? work.parts.at(part).title : QString();
        if (piece.chapters.size() == 1 && piece.chapters.first().filePath.isEmpty()
            && piece.chapters.first().startMs == 0
            && (piece.chapters.first().title.isEmpty() || piece.chapters.first().title == partTitle))
            piece.chapters.clear();
        if (positionMs >= startMs + durationMs && positionMs > 0) {
            piece.positionMs = durationMs;
            piece.progress = 1.0;
            piece.lastOpened = format.lastOpened;
        } else if (positionMs > startMs) {
            piece.positionMs = positionMs - startMs;
            piece.progress = durationMs > 0 ? double(piece.positionMs) / double(durationMs) : 0.0;
            piece.lastOpened = format.lastOpened;
        } else {
            piece.positionMs = 0;
        }
    }
    return pieces;
}

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

    // The parts, in volume order; a combined work passes on its own.
    QList<WorkPart> parts;
    for (const Work &work : works) {
        if (work.parts.size() > 1)
            parts += work.parts;
        else
            parts.append(partFromWork(work));
    }
    if (parts.size() > 1) {
        const QCollator collator = naturalCollator();
        std::stable_sort(parts.begin(), parts.end(), [&collator](const WorkPart &a, const WorkPart &b) {
            if (volumeKey(a) != volumeKey(b))
                return volumeKey(a) < volumeKey(b);
            if (const int byTitle = collator.compare(a.title, b.title); byTitle != 0)
                return byTitle < 0;
            // The same title (or none): by their files, as before parts.
            return collator.compare(a.files.value(0), b.files.value(0)) < 0;
        });
        merged.parts = parts;
        // Ebooks first, in volume order; the audiobooks follow (joined below).
        std::stable_sort(merged.formats.begin(), merged.formats.end(), [&merged](const Format &a, const Format &b) {
            if (a.kind != b.kind)
                return a.kind == Format::Kind::Ebook;
            return partKey(merged, a.filePath) < partKey(merged, b.filePath);
        });
    }
    joinAudiobooks(merged);
    return merged;
}

WorkPart partFromWork(const Work &work)
{
    WorkPart part;
    part.id = work.id;
    part.title = work.title;
    part.authors = work.authors;
    part.narrators = work.narrators;
    part.series = work.series;
    part.seriesIndex = work.seriesIndex;
    part.description = work.description;
    part.tags = work.tags;
    part.rating = work.rating;
    part.language = work.language;
    part.publishedYear = work.publishedYear;
    part.added = work.added;
    for (const Format &format : work.formats)
        part.files += LibraryExport::formatFiles(format);
    return part;
}

QList<Work> splitWork(const Work &work, const std::function<QString()> &newId)
{
    if (!work.canSplit())
        return { work };

    QList<Work> result;
    if (work.parts.size() > 1) {
        // Every part comes back; the one with the work's id keeps its metadata.
        int keep = -1;
        for (int i = 0; i < work.parts.size(); ++i) {
            result.append(workFromPart(work.parts.at(i)));
            if (work.parts.at(i).id == work.id)
                keep = i;
        }
        if (keep < 0) {
            keep = 0;
            result[0].id = work.id;
        }
        copyMetadata(result[keep], work);
        result[keep].added = work.parts.at(keep).added.isValid() ? work.parts.at(keep).added : work.added;

        for (const Format &format : work.formats) {
            QList<std::pair<int, Format>> pieces;
            if (format.kind == Format::Kind::Audiobook)
                pieces = cutAudiobook(work, format);
            else
                pieces.append({ work.partOf(format.filePath), format });
            for (const auto &[part, piece] : pieces)
                result[part >= 0 ? part : keep].formats.append(piece);
        }
    } else {
        // Merged before parts were stored: one work per format.
        for (int i = 0; i < work.formats.size(); ++i) {
            Work single;
            copyMetadata(single, work);
            single.id = result.isEmpty() ? work.id : newId();
            if (!result.isEmpty())
                single.title = work.formatTitle(i);
            single.formats.append(work.formats.at(i));
            result.append(single);
        }
    }

    // Parts without formats (their files were removed) are left out; "last
    // opened" follows the formats.
    QList<Work> works;
    for (Work &single : result) {
        if (single.formats.isEmpty())
            continue;
        for (const Format &format : std::as_const(single.formats)) {
            if (format.lastOpened.isValid() && (!single.lastOpened.isValid() || format.lastOpened > single.lastOpened))
                single.lastOpened = format.lastOpened;
        }
        works.append(single);
    }
    return works;
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

    const QCollator collator = naturalCollator();
    std::stable_sort(parts.begin(), parts.end(), [&collator, &work](const Format &a, const Format &b) {
        const int partA = partKey(work, a.filePath);
        const int partB = partKey(work, b.filePath);
        if (partA != partB)
            return partA < partB;
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
        // A file without chapters of a combined work: titled like its part,
        // so the chapter list names the volumes.
        if (part.chapters.isEmpty()) {
            if (const int index = work.partOf(part.filePath); index >= 0 && work.isCombined())
                joined.chapters.last().title = work.parts.at(index).title;
        }
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
