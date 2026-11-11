#pragma once

#include <QStringList>

#include "filemetadata.h"

// Reads the metadata of one ebook or audiobook file, depending on its type,
// and fills missing title, author and series number from the file name.
FileMetadata readMetadata(const QString &path);

// File suffixes calibly can import, lower case without dot.
QStringList ebookSuffixes();
QStringList audiobookSuffixes();
bool isSupportedFile(const QString &path);
