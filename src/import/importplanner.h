#pragma once

#include <QList>
#include <QStringList>

#include "filemetadata.h"
#include "work.h"

// Decides which work each imported file belongs to (pure logic, no files are
// touched):
//   * files with the same base name ("DCC 01 - … .epub" / "… .m4b") or the
//     same title and author belong to one work,
//   * a file whose work already exists in the library is added to it,
//   * a file whose format already exists there with the same size is a
//     duplicate and skipped.
// Ebooks are processed first, so their metadata wins over audiobook tags.
struct ImportPlan
{
    struct PlannedFormat
    {
        int workIndex = -1;      // index into works
        FileMetadata metadata;
    };

    QList<Work> works;           // new works and copies of existing ones (without the new formats)
    QList<PlannedFormat> formats;
    QStringList duplicates;      // source paths that are already in the library
};

ImportPlan planImport(QList<FileMetadata> files, const QList<Work> &existing);

// Key that is equal for the same book in different editions:
// title (without subtitle, case, accents, punctuation) + last name of the
// first author.
QString workMatchKey(const QString &title, const QStringList &authors);
