// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

// Reads title, author and series number from file names such as
//   "DCC 01 - Dungeon Crawler Carl - Matt Dinniman"   (series code, number)
//   "Der Marsianer - Andy Weir"                       (title - author)
//   "Moby Dick"                                       (title only)
// Used as a fallback when the file itself has no metadata.
struct FileNameInfo
{
    QString seriesCode;   // e.g. "DCC"
    int seriesIndex = 0;
    QString title;
    QString author;
};

FileNameInfo parseFileName(const QString &baseName);
