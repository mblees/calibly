#pragma once

#include "filemetadata.h"

// Reads the tags of an audio file with TagLib (M4B, M4A, MP3, ...): title,
// author, narrator, series, year, description, length and embedded cover.
// Audiobook conventions: the album is the book title, album artist / artist
// the author and composer (or a NARRATOR tag) the narrator.
void readAudio(const QString &path, FileMetadata &metadata);
