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

    // Ebooks: where the reader stopped, see ReadingPosition; "" = never read
    // in calibly.
    QString readingPosition;

    // When this format was last read / listened to; invalid = never.
    QDateTime lastOpened;
    // The file (or one file of an audiobook of several files) was not found.
    // Not stored: the backend sets it when it loads the library.
    bool fileMissing = false;

    // Read or listened to the end (or marked as finished by the user).
    bool finished() const { return progress >= 1.0; }
};

// One of the works a combined work was made of ("Kombinieren", phase 12):
// its own metadata and the files of its formats, so "Kombination auflösen"
// can give it back.
struct WorkPart
{
    QString id;
    QString title;
    QStringList authors;
    QStringList narrators;
    QString series;
    int seriesIndex = 0;
    QString description;
    QStringList tags;
    int rating = 0;
    QString language;
    int publishedYear = 0;
    QDateTime added;
    QStringList files;          // its format files and the files of their chapters

};

struct Work
{
    // What the cover says about the work besides formats and progress
    // (view F of the mockup). Only one applies, in this order:
    //   FileMissing  a file of a format was not found
    //   Finished     a format was read / listened to the end
    //   New          never opened since it was added
    enum class State { Normal, New, Finished, FileMissing };

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
    // A combined work: the works it was made of, in volume order. Empty for
    // works that were never combined (and for works merged before phase 12).
    QList<WorkPart> parts;

    // First format of this kind, nullptr if there is none.
    const Format *format(Format::Kind kind) const;
    Format *format(Format::Kind kind);
    // Its index in formats, -1 if there is none.
    int formatIndex(Format::Kind kind) const;

    bool hasFormat(Format::Kind kind) const { return format(kind) != nullptr; }
    bool hasEbook() const { return hasFormat(Format::Kind::Ebook); }
    bool hasAudiobook() const { return hasFormat(Format::Kind::Audiobook); }

    // Progress of all formats of this kind, 0 if there is none. The volumes
    // of a combined work: by pages if all know them, otherwise the average.
    // Several formats of a work that was not combined are the same book in
    // other files (e.g. EPUB and PDF): the highest progress.
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

    // Made of several works with "Kombinieren": its ebook formats are
    // volumes, in their order.
    bool isCombined() const { return parts.size() > 1; }
    // Formats of this kind.
    int formatCount(Format::Kind kind) const;
    // The format of this kind to continue with: the most recently opened
    // one if it is not finished; if it is, the next unfinished one after it,
    // else the first unfinished one; if none was opened, the first
    // unfinished one; if all are finished, the first. -1 if there is none.
    int currentFormat(Format::Kind kind) const;
    // The part a file belongs to, -1 if none.
    int partOf(const QString &filePath) const;
    // Title of the format as a volume: the title of its part, else its file
    // name without extension.
    QString formatTitle(int formatIndex) const;
    // The work can be dissolved: it was combined, or has several formats
    // (merged before phase 12).
    bool canSplit() const { return parts.size() > 1 || formats.size() > 1; }

    // State of the work, looking at all its formats. A kind counts as
    // finished when all its formats are.
    State state() const;
    // State of the work presented as this format only (format selection
    // "Ebooks" / "Audiobooks"): the other format is ignored.
    State state(Format::Kind kind) const;

    // Remembers that this format was opened now: for the format ("Continue")
    // and for the work (sorting by "last opened", state New).
    void markOpened(int formatIndex, const QDateTime &when);

    // Libraries from before formats had their own lastOpened: the formats
    // with progress (or, if none has any, the first format) get the work's
    // lastOpened. Does nothing if a format already has one.
    void inheritLastOpened();
};
