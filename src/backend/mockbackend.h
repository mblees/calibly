#pragma once

#include <QString>

#include "librarybackend.h"

// Reads sample works from a JSON file (by default the embedded
// :/mock/mock.json). Each format can have its own cover; the file names are
// relative to the "covers" folder next to the JSON file.
class MockBackend : public LibraryBackend
{
public:
    explicit MockBackend(QString jsonPath = QStringLiteral(":/mock/mock.json"));

    QList<Work> loadWorks() override;

private:
    QString m_jsonPath;
};
