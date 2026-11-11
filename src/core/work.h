#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Plain data classes of calibly (no Qt Quick, no UI).
//
// A Work is one title independent of its format (e.g. "Moby Dick").
// It has one or more Formats: ebook or audiobook files with their own progress.

struct Format
{
    enum class Kind { Ebook, Audiobook };

    Kind kind = Kind::Ebook;
    QString filePath;
    double progress = 0.0; // 0.0 – 1.0
};

struct Work
{
    QString id;
    QString title;
    QStringList authors;
    QStringList narrators;
    QString series;
    int seriesIndex = 0;
    QString coverPath;          // empty = no cover, UI shows a placeholder
    bool coverIsSquare = false; // audiobook covers are usually square
    QDateTime lastOpened;
    QList<Format> formats;

    bool hasFormat(Format::Kind kind) const;
    bool hasEbook() const { return hasFormat(Format::Kind::Ebook); }
    bool hasAudiobook() const { return hasFormat(Format::Kind::Audiobook); }

    // Highest progress of all formats of this kind, 0 if there is none.
    double progress(Format::Kind kind) const;
    double readProgress() const { return progress(Format::Kind::Ebook); }
    double listenProgress() const { return progress(Format::Kind::Audiobook); }
};
