// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>

#include "work.h"

// Export of works ("Exportieren …" for the selection): their files are
// copied into a new folder with the two subfolders "Ebooks" and
// "Audiobooks", without further subfolders. Files keep their names; a name
// that is already taken in a subfolder gets " (2)", " (3)" … before the
// extension. Covers and the database are not exported.
namespace LibraryExport {

struct File
{
    QString source;
    QString target;             // full path inside the export folder
    qint64 size = 0;
};

struct Plan
{
    QString folder;             // the new export folder
    QList<File> files;          // in the order of the works and their formats
    QStringList missing;        // files of the works that were not found
    qint64 totalBytes = 0;
};

enum class Result {
    Done,
    Cancelled,                  // the export folder was removed again
    CannotCreateFolder,         // the folder or a subfolder cannot be created
    CannotCopy,                 // a file could not be read or written
};

struct Outcome
{
    Result result = Result::Done;
    int copied = 0;             // files copied completely
    QString failedFile;         // CannotCopy: the source that failed
};

// Names of the subfolders.
QString subfolderName(Format::Kind kind);

// Name of the export folder of that day, e.g. "calibly-Export 2026-11-13".
QString folderName(const QDate &date);

// name in parent, or "name (2)", "name (3)" … if that exists already.
QString uniqueFolder(const QString &parent, const QString &name);

// The files of a format: its own file, then the files of its chapters that
// are not that file (audiobooks of several files), each once, in playing
// order.
QStringList formatFiles(const Format &format);

// What would be copied for these works into folder (which need not exist).
// The same file is planned only once, even if two works name it.
Plan plan(const QList<Work> &works, const QString &folder);

// Copies the files of the plan. The folder must not exist yet; it is
// created with both subfolders. progress is called after every piece
// (bytes copied in total, files finished, the file being copied). When
// cancel becomes true the export stops and the folder is removed again. On
// CannotCopy the files copied before stay, the broken one is removed.
using Progress = std::function<void(qint64 bytes, int files, const QString &current)>;
Outcome run(const Plan &plan, const Progress &progress, const std::atomic_bool &cancel);

} // namespace LibraryExport
