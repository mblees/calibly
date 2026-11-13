// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

// Where the user stopped reading an ebook, stored per format
// (Format::readingPosition) as a short JSON text.
//
// EPUB (and text files): the chapter (index into the spine) and the
// character in it, so the place survives another window or font size, which
// changes the pages. label is the title of that chapter as the contents
// name it, shown in "Weitermachen".
// PDF: the page (0-based).
struct ReadingPosition
{
    int chapter = -1;   // -1 = no chapter position
    int offset = 0;     // character in the chapter
    int page = -1;      // PDF page, -1 = none
    QString label;

    bool isValid() const { return chapter >= 0 || page >= 0; }

    // "" for an invalid position.
    QString toString() const;
    // An invalid position for an empty or unreadable text.
    static ReadingPosition fromString(const QString &text);

    static ReadingPosition inChapter(int chapter, int offset, const QString &label = QString())
    {
        ReadingPosition position;
        position.chapter = chapter;
        position.offset = offset;
        position.label = label;
        return position;
    }
    static ReadingPosition onPage(int page)
    {
        ReadingPosition position;
        position.page = page;
        return position;
    }

    bool operator==(const ReadingPosition &other) const
    {
        return chapter == other.chapter && offset == other.offset && page == other.page && label == other.label;
    }
};
