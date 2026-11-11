#pragma once

#include <QList>

#include "work.h"

// Source of the library data. The UI only ever talks to models and
// controllers; which backend is behind them is decided in main().
//   Phase 1: MockBackend (sample data from resources)
//   Phase 4: SqliteBackend (real library)
class LibraryBackend
{
public:
    virtual ~LibraryBackend() = default;

    virtual QList<Work> loadWorks() = 0;
};
