#pragma once

#include <QString>

#include "librarybackend.h"

// Reads sample works from a JSON file (by default the embedded
// :/mock/mock.json, or any file given with --mock). Each format can have its
// own cover; cover file names are relative to the "covers" folder next to the
// JSON file. Relative format file paths are relative to the JSON file; for
// existing files without a "size" the real file size is used. Audiobook
// formats can list "chapters" (title, start and duration in seconds,
// optional file).
class MockBackend : public LibraryBackend
{
public:
    explicit MockBackend(QString jsonPath = QStringLiteral(":/mock/mock.json"));

    QList<Work> loadWorks() override;
    // Changes are kept in memory only (e.g. "last opened"), the JSON file stays unchanged.
    bool saveWork(const Work &work) override;

private:
    QList<Work> readJson() const;

    QString m_jsonPath;
    QList<Work> m_works;
    bool m_loaded = false;
};
