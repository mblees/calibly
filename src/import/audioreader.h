// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "filemetadata.h"

// Reads the tags of an audio file with TagLib (M4B, M4A, MP3, ...): title,
// author, narrator, series, year, description, length, embedded cover,
// track title and number and the chapters (M4B/M4A: QuickTime or Nero
// chapters, MP3: ID3v2 CHAP frames).
// Audiobook conventions: the album is the book title, album artist / artist
// the author and composer (or a NARRATOR tag) the narrator.
void readAudio(const QString &path, FileMetadata &metadata);
