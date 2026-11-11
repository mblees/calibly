#pragma once

#include <QImage>
#include <QString>
#include <QStringList>

#include "work.h"

// Everything calibly can read from one ebook or audiobook file.
// Empty / 0 fields are unknown.
struct FileMetadata
{
    QString sourcePath;          // the file the user picked
    Format::Kind kind = Format::Kind::Ebook;

    QString title;
    QStringList authors;
    QStringList narrators;
    QString series;
    int seriesIndex = 0;
    QString description;
    QStringList tags;
    int rating = 0;              // 0 – 5
    QString language;            // language code, e.g. "de"
    int publishedYear = 0;
    QString publisher;
    QString isbn;

    QImage cover;
    qint64 fileSize = 0;
    int pages = 0;
    int durationSeconds = 0;

    // Audio files: exact length, the file's own title and track number (for
    // audiobooks made of several files) and the chapters inside the file
    // (times relative to the file, without file path).
    qint64 durationMs = 0;
    QString trackTitle;
    int trackNumber = 0;
    QList<Chapter> chapters;

    QString error;               // why the file could not be read, empty = fine
};
