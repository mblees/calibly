// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QString>

#include "work.h"

// One time axis for an audiobook format: its files ("parts") one after
// another and its chapters on that axis. All times are milliseconds from
// the start of the audiobook. Used by the player and the chapter lists.
//
// A format without chapters gives one part and one untitled chapter that
// cover the whole file. With chapters, a new part starts wherever the file
// of a chapter changes (an MP3 folder: one part per file).
class AudioTimeline
{
public:
    struct Part
    {
        QString filePath;
        qint64 startMs = 0;
        qint64 durationMs = 0;
    };
    struct Entry
    {
        QString title;          // empty = untitled
        qint64 startMs = 0;
        qint64 durationMs = 0;
        int part = 0;           // index in parts()
    };

    AudioTimeline() = default;
    explicit AudioTimeline(const Format &format);

    bool isEmpty() const { return m_parts.isEmpty(); }
    qint64 durationMs() const { return m_durationMs; }
    const QList<Part> &parts() const { return m_parts; }
    const QList<Entry> &chapters() const { return m_chapters; }

    // Chapter / part at this time, clamped to the first and last one;
    // -1 if the timeline is empty.
    int chapterAt(qint64 ms) const;
    int partAt(qint64 ms) const;

private:
    QList<Part> m_parts;
    QList<Entry> m_chapters;
    qint64 m_durationMs = 0;
};
