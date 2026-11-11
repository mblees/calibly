#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Plain data classes of calibly (no Qt Quick, no UI).
//
// A Work is one title independent of its format (e.g. "Moby Dick").
// It has one or more Formats: ebook or audiobook files with their own
// progress and their own cover (audiobook covers are usually square).
//
// The fields of Work are shared by all formats (title, authors, series …).
// Narrators only make sense for audiobooks, but are kept on the work, so the
// search and the views find them without looking at the formats.

// A chapter of an audiobook. Audiobooks from several files (e.g. an MP3
// folder) name the file of each chapter; a chapter without a file belongs
// to the format's own file. Times are relative to the start of that file.
struct Chapter
{
    QString title;              // empty = untitled, the UI shows "Kapitel n"
    QString filePath;           // empty = Format::filePath
    qint64 startMs = 0;
    qint64 durationMs = 0;

    bool operator==(const Chapter &other) const
    {
        return title == other.title && filePath == other.filePath && startMs == other.startMs
            && durationMs == other.durationMs;
    }
};

struct Format
{
    enum class Kind { Ebook, Audiobook };

    Kind kind = Kind::Ebook;
    QString filePath;
    double progress = 0.0;      // 0.0 – 1.0
    QString coverPath;          // empty = no cover, the UI shows a placeholder
    bool coverIsSquare = false;
    qint64 fileSize = 0;        // bytes, 0 = unknown
    int pages = 0;              // ebooks, 0 = unknown
    int durationSeconds = 0;    // audiobooks, 0 = unknown

    // Metadata that differs between the formats of a work, e.g. the publisher
    // of the book and the publisher of the audiobook.
    QString publisher;
    QString isbn;

    // Audiobooks: chapters in playing order (empty = one file without
    // chapters) and the saved listening position across all files.
    QList<Chapter> chapters;
    qint64 positionMs = 0;
};

struct Work
{
    QString id;
    QString title;
    QStringList authors;
    QStringList narrators;
    QString series;
    int seriesIndex = 0;
    QString description;
    QStringList tags;
    int rating = 0;             // 0 – 5 stars, 0 = not rated
    QString language;           // language code, e.g. "de"
    int publishedYear = 0;      // 0 = unknown
    QDateTime added;            // when the work was added to the library
    QDateTime lastOpened;
    QList<Format> formats;

    // First format of this kind, nullptr if there is none.
    const Format *format(Format::Kind kind) const;
    Format *format(Format::Kind kind);

    bool hasFormat(Format::Kind kind) const { return format(kind) != nullptr; }
    bool hasEbook() const { return hasFormat(Format::Kind::Ebook); }
    bool hasAudiobook() const { return hasFormat(Format::Kind::Audiobook); }

    // Highest progress of all formats of this kind, 0 if there is none.
    double progress(Format::Kind kind) const;
    double readProgress() const { return progress(Format::Kind::Ebook); }
    double listenProgress() const { return progress(Format::Kind::Audiobook); }

    // The format that represents the work when no format is selected:
    // the ebook if there is one, otherwise the audiobook.
    Format::Kind primaryKind() const { return hasEbook() ? Format::Kind::Ebook : Format::Kind::Audiobook; }

    // Cover of a format. A missing format or cover gives an empty path;
    // coverIsSquare() is then true for audiobooks, so their placeholder is
    // square like a real audiobook cover.
    QString coverPath(Format::Kind kind) const;
    bool coverIsSquare(Format::Kind kind) const;
};
