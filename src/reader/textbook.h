#pragma once

#include <QString>

#include "booksource.h"

// A plain text (.txt) or Markdown (.md) file as a book of one chapter.
// Markdown may refer to images next to the file. Markdown headings
// (# and ##) become the table of contents.
class TextBook : public BookSource
{
public:
    // False (with a reason) if the file cannot be read.
    bool open(const QString &filePath, QString *error = nullptr);

    // The suffixes it reads, lower case without dot.
    static bool isTextBook(const QString &filePath);

    QString title() const override { return m_title; }
    int chapterCount() const override { return 1; }
    TextFormat chapterFormat(int) const override { return m_format; }
    QString chapterText(int chapter) const override { return chapter == 0 ? m_text : QString(); }
    QString chapterPath(int chapter) const override { return chapter == 0 ? m_fileName : QString(); }
    QByteArray resource(const QString &path) const override;
    QList<ContentsEntry> contents() const override { return m_contents; }
    qint64 chapterWeight(int) const override { return std::max<qint64>(1, m_text.size()); }

private:
    QString m_folder;
    QString m_fileName;
    QString m_title;
    QString m_text;
    TextFormat m_format = TextFormat::PlainText;
    QList<ContentsEntry> m_contents;
};
