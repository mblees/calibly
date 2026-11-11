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
// Audio files come in playing order (folder, track number, file name with
// numbers compared as numbers), so the files of an MP3 folder can be joined
// into one audiobook (see appendAudioPart()).
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

// The format of an imported file; target is its copy in the library. The
// cover is not set. An audiobook without chapters gets one chapter for the
// whole file, so further files can be appended as chapters.
Format formatFromFile(const FileMetadata &file, const QString &target);

// Appends one more file of an audiobook made of several files (e.g. the next
// MP3 of a folder): its chapters, or one chapter for the whole file.
void appendAudioPart(Format &format, const FileMetadata &file, const QString &target);
