#pragma once

#include <QList>
#include <QString>

#include "work.h"

// Source of the library data. The UI only ever talks to models and
// controllers; which backend is behind them is decided in main().
//   MockBackend:   sample data (development, --mock)
//   SqliteBackend: the real library (phase 4)
class LibraryBackend
{
public:
    virtual ~LibraryBackend() = default;

    virtual QList<Work> loadWorks() = 0;

    // Inserts or updates the work with all its formats. Returns false if the
    // backend cannot store it.
    virtual bool saveWork(const Work &work) = 0;

    // Folder that holds the library files; empty if files cannot be imported.
    virtual QString libraryRoot() const { return QString(); }
};
