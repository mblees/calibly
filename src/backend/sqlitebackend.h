// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

#include "librarybackend.h"

// The real library: a SQLite database calibly.db inside the library folder,
// next to the imported files (like calibre's metadata.db). File and cover
// paths are stored relative to the library folder, so the whole folder can be
// moved or backed up as one.
//
// Tables:
//   works    one row per work (lists such as authors are stored as JSON)
//   formats  one row per file, linked to its work, with the metadata that
//            differs per format (publisher, ISBN), the chapters (JSON), the
//            listening position, the reading position (ebooks) and when
//            the format was last opened
// Whether the files still exist is checked when the library is loaded
// (Format::fileMissing).
// The schema version is kept in PRAGMA user_version; open() updates older
// libraries step by step, after copying calibly.db to
// calibly.db.v<old version>-<date>.bak (the newest three are kept).
class SqliteBackend : public LibraryBackend
{
public:
    explicit SqliteBackend(QString libraryRoot);
    ~SqliteBackend() override;

    // Why open() failed.
    enum class OpenError {
        None,
        NotFound,      // no calibly.db there and create was false
        CannotCreate,  // the folder or the database cannot be created
        Damaged,       // calibly.db is no readable SQLite database
        NewerVersion,  // written by a newer calibly
        CannotUpdate,  // the schema update or its backup failed (nothing changed)
    };

    // Opens the library. create: make the folder and the database if there
    // is none (a new library); otherwise a missing calibly.db is NotFound.
    // Must be called first; a damaged database is never changed.
    bool open(QString *error = nullptr, bool create = true);
    OpenError openError() const { return m_openError; }

    // <libraryRoot>/calibly.db
    static QString databasePath(const QString &libraryRoot);

    QList<Work> loadWorks() override;
    bool saveWork(const Work &work) override;
    bool removeWork(const QString &workId) override;
    QString libraryRoot() const override { return m_root; }

    // Default library folder: <Documents>/calibly-Bibliothek
    static QString defaultLibraryRoot();

private:
    static constexpr int kKeptBackups = 3;

    bool fail(OpenError kind, const QString &text, QString *error);
    bool backUp(int version);
    QString absolute(const QString &relativePath) const;
    QString relative(const QString &absolutePath) const;
    QString chaptersToJson(const QList<Chapter> &chapters) const;
    QList<Chapter> chaptersFromJson(const QString &json) const;
    QString partsToJson(const QList<WorkPart> &parts) const;
    QList<WorkPart> partsFromJson(const QString &json) const;

    QString m_root;
    QString m_connectionName;
    OpenError m_openError = OpenError::None;
};
