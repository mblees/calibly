// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "audioreader.h"

#include <QFile>

#include "textutils.h"

#include <chapterframe.h>
#include <fileref.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <tpropertymap.h>
#include <tvariant.h>

#include <algorithm>

namespace {

QString toQString(const TagLib::String &string)
{
    return QString::fromStdWString(string.toWString()).simplified();
}

// First value of the first of these tag names that is set.
QString firstValue(const TagLib::PropertyMap &properties, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const auto it = properties.find(key);
        if (it != properties.end() && !it->second.isEmpty()) {
            const QString value = toQString(it->second.front());
            if (!value.isEmpty())
                return value;
        }
    }
    return QString();
}

// Chapters of M4B / M4A files (QuickTime chapter track, else Nero chapters)
// and of MP3 files (ID3v2 CHAP frames), sorted by start. The durations are
// derived from the next chapter's start and the file length.
QList<Chapter> readChapters(TagLib::File *file, qint64 fileDurationMs)
{
    QList<Chapter> chapters;
    if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
        TagLib::MP4::ChapterList list = mp4->qtChapters();
        if (list.isEmpty())
            list = mp4->neroChapters();
        for (const TagLib::MP4::Chapter &chapter : list)
            chapters.append({ toQString(chapter.title()), QString(), chapter.startTime(), 0 });
    } else if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
        if (mpeg->hasID3v2Tag()) {
            for (const TagLib::ID3v2::Frame *frame : mpeg->ID3v2Tag()->frameList("CHAP")) {
                const auto *chapter = dynamic_cast<const TagLib::ID3v2::ChapterFrame *>(frame);
                if (!chapter)
                    continue;
                const TagLib::ID3v2::FrameList titles = chapter->embeddedFrameList("TIT2");
                chapters.append({ titles.isEmpty() ? QString() : toQString(titles.front()->toString()),
                                  QString(), qint64(chapter->startTime()), 0 });
            }
        }
    }

    std::stable_sort(chapters.begin(), chapters.end(),
                     [](const Chapter &a, const Chapter &b) { return a.startMs < b.startMs; });
    for (int i = 0; i < chapters.size(); ++i) {
        const qint64 end = i + 1 < chapters.size() ? chapters.at(i + 1).startMs : fileDurationMs;
        chapters[i].durationMs = std::max<qint64>(0, end - chapters.at(i).startMs);
    }
    // A single chapter at 0 says nothing the file does not say itself.
    if (chapters.size() == 1 && chapters.first().startMs == 0)
        chapters.clear();
    return chapters;
}

} // namespace

void readAudio(const QString &path, FileMetadata &metadata)
{
#ifdef Q_OS_WIN
    TagLib::FileRef file(reinterpret_cast<const wchar_t *>(path.utf16()));
#else
    TagLib::FileRef file(QFile::encodeName(path).constData());
#endif
    if (file.isNull()) {
        metadata.error = QStringLiteral("not a readable audio file");
        return;
    }

    if (const TagLib::AudioProperties *audio = file.audioProperties()) {
        metadata.durationMs = audio->lengthInMilliseconds();
        metadata.durationSeconds = int((metadata.durationMs + 500) / 1000);
    }

    const TagLib::PropertyMap properties = file.properties();
    metadata.trackTitle = firstValue(properties, { "TITLE" });
    metadata.title = firstValue(properties, { "ALBUM" });
    if (metadata.title.isEmpty() && !metadata.trackTitle.isEmpty()) {
        metadata.title = metadata.trackTitle;
        metadata.titleFromTrack = true;
    }
    metadata.trackNumber = firstValue(properties, { "TRACKNUMBER" }).section(u'/', 0, 0).toInt(); // "3/12"
    metadata.chapters = readChapters(file.file(), metadata.durationMs);
    metadata.authors = TextUtils::splitNames(firstValue(properties, { "ALBUMARTIST", "ARTIST" }));
    metadata.narrators = TextUtils::splitNames(firstValue(properties, { "NARRATOR", "NARRATEDBY", "COMPOSER" }));
    metadata.series = firstValue(properties, { "SERIES", "MOVEMENTNAME" });
    metadata.seriesIndex = int(firstValue(properties, { "SERIES-PART", "SERIESPART", "MOVEMENT", "MOVEMENTNUMBER" }).toDouble());
    metadata.description = firstValue(properties, { "DESCRIPTION", "LONGDESCRIPTION", "COMMENT" });
    metadata.publisher = firstValue(properties, { "LABEL", "PUBLISHER" });
    metadata.isbn = firstValue(properties, { "ISBN" });
    metadata.publishedYear = firstValue(properties, { "DATE", "ORIGINALDATE" }).left(4).toInt();
    metadata.language = firstValue(properties, { "LANGUAGE" }).section(u'-', 0, 0).toLower().left(3);
    const QString genre = firstValue(properties, { "GENRE" });
    if (!genre.isEmpty() && genre.compare(QLatin1String("Audiobook"), Qt::CaseInsensitive) != 0
        && genre.compare(QStringLiteral("Hörbuch"), Qt::CaseInsensitive) != 0)
        metadata.tags.append(genre);

    // Embedded cover (TagLib 2: generic "PICTURE" complex property).
    const auto pictures = file.complexProperties("PICTURE");
    if (!pictures.isEmpty()) {
        const TagLib::ByteVector data = pictures.front().value("data").toByteVector();
        metadata.cover.loadFromData(reinterpret_cast<const uchar *>(data.data()), int(data.size()));
    }
}
