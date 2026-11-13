// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "filemetadata.h"

// Reads title, author and page count of a PDF and renders the first page as
// cover. Needs the Qt PDF module; without it (CALIBLY_HAS_QTPDF not defined)
// nothing is read and the file name is used instead.
void readPdf(const QString &path, FileMetadata &metadata);

// Whether PDF metadata and covers are available in this build.
bool pdfSupportAvailable();
