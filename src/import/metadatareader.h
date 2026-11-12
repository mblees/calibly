#pragma once

#include <QStringList>

#include "filemetadata.h"

// Reads the metadata of one ebook or audiobook file, depending on its type,
// and fills missing title, author and series number from the file name.
FileMetadata readMetadata(const QString &path);

// File suffixes calibly can import, lower case without dot.
QStringList ebookSuffixes();
QStringList audiobookSuffixes();
// Plain text and Markdown, read in calibly's reader. Imported only when the
// user picks or drops the file itself: folders of audiobooks often hold
// .txt files that are no books.
QStringList textSuffixes();
// picked: the user chose this file itself (not found in a folder).
bool isSupportedFile(const QString &path, bool picked = false);
