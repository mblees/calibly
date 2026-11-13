// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QImage>
#include <QString>

#include "work.h"

// Layout of the library folder:
//   <library>/<Author>/<Title>/<file>
//   <library>/<Author>/<Title>/<file name>.cover.jpg   (cover of that format)
//   <library>/calibly.db                               (see SqliteBackend)
namespace LibraryFolder {

// Folder of a work: the folder of its first format if it has one, otherwise
// a new one from author and title.
QString workFolder(const QString &libraryRoot, const Work &work);

// Name usable as a file or folder name on all systems.
QString safeName(const QString &name);

// Copies the file into the folder and returns the new path. An identical file
// (same name and size) that is already there is reused; another file with the
// same name gets a number. Empty string on failure.
QString copyInto(const QString &sourcePath, const QString &folder, QString *error = nullptr);

// Saves the cover next to the format file; returns its path, empty if the
// image is null or cannot be saved.
QString saveCover(const QImage &cover, const QString &formatFilePath);

// The files of the work inside the library folder: the format files, the
// files of audiobooks of several files and the covers. Files outside the
// library folder (e.g. of sample data) are never part of it.
QStringList workFiles(const QString &libraryRoot, const Work &work);

// Removes the files of a removed work from the library folder, and the
// folders that are empty afterwards (title, author). toTrash moves the files
// to the recycle bin of the system and deletes only where there is none;
// otherwise they are deleted. Files that are already gone do not count as
// failed. Returns the files that could not be removed.
QStringList removeWorkFiles(const QString &libraryRoot, const Work &work, bool toTrash);

} // namespace LibraryFolder
