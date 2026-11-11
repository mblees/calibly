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
//   formats  one row per file, linked to its work
class SqliteBackend : public LibraryBackend
{
public:
    explicit SqliteBackend(QString libraryRoot);
    ~SqliteBackend() override;

    // Creates the folder and the database if needed. Must be called first.
    bool open(QString *error = nullptr);

    QList<Work> loadWorks() override;
    bool saveWork(const Work &work) override;
    QString libraryRoot() const override { return m_root; }

    // Default library folder: <Documents>/calibly-Bibliothek
    static QString defaultLibraryRoot();

private:
    QString absolute(const QString &relativePath) const;
    QString relative(const QString &absolutePath) const;

    QString m_root;
    QString m_connectionName;
};
