// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QStringList>

#include "filemetadata.h"

// Reads the metadata of one ebook or audiobook file, depending on its type,
// and fills missing title, author and series number from the file name.
FileMetadata readMetadata(const QString &path);

// File suffixes calibly can import, lower case without dot: the whitelist
// of the import (phase 13: books and audiobooks only; text and Markdown
// files are no longer imported, the reader still opens those a library
// already has).
QStringList ebookSuffixes();
QStringList audiobookSuffixes();
bool isSupportedFile(const QString &path);
