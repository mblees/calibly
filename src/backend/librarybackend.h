// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

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

    // Removes the work with all its formats from the library (not its
    // files). Returns false if the backend cannot remove it.
    virtual bool removeWork(const QString &) { return false; }

    // Folder that holds the library files; empty if files cannot be imported.
    virtual QString libraryRoot() const { return QString(); }
};
