// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "filemetadata.h"

// Reads the metadata of an EPUB file (a ZIP archive with an OPF package
// document): title, authors, language, publisher, ISBN, year, description,
// subjects as tags, calibre/EPUB 3 series and the cover image.
void readEpub(const QString &path, FileMetadata &metadata);
