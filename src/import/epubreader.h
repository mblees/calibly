#pragma once

#include "filemetadata.h"

// Reads the metadata of an EPUB file (a ZIP archive with an OPF package
// document): title, authors, language, publisher, year, description,
// subjects as tags, calibre/EPUB 3 series and the cover image.
void readEpub(const QString &path, FileMetadata &metadata);
