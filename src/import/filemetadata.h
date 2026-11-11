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

    QImage cover;
    qint64 fileSize = 0;
    int pages = 0;
    int durationSeconds = 0;

    QString error;               // why the file could not be read, empty = fine
};
