#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

// A book the reader can lay out (no UI): chapters of HTML, Markdown or plain
// text in reading order, the files they refer to (images, style sheets) and
// the table of contents. EpubBook and TextBook are the two kinds.
class BookSource
{
public:
    enum class TextFormat { Html, Markdown, PlainText };

    // One line of the table of contents. chapter is an index into the
    // chapters, fragment the place in it ("" = its start), level 0 = top.
    struct ContentsEntry
    {
        QString title;
        int level = 0;
        int chapter = -1;
        QString fragment;
    };

    virtual ~BookSource() = default;

    virtual QString title() const { return QString(); }
    virtual int chapterCount() const = 0;
    virtual TextFormat chapterFormat(int chapter) const;
    virtual QString chapterText(int chapter) const = 0;
    // Path of the chapter inside the book; links and resources are relative to it.
    virtual QString chapterPath(int chapter) const = 0;
    // A file of the book by its path inside the book, empty if there is none.
    virtual QByteArray resource(const QString &path) const;
    virtual QList<ContentsEntry> contents() const { return {}; }
    // The font files that come with the book.
    virtual QList<QByteArray> fonts() const { return {}; }
    // Rough length of the chapter in characters of text (> 0), for the
    // progress through the whole book.
    virtual qint64 chapterWeight(int chapter) const = 0;

    // Chapter with this path, -1 if there is none.
    int chapterOf(const QString &path) const;

    // Path of href relative to the file basePath, both inside the book:
    // "../Images/a.jpg" from "OEBPS/Text/c1.xhtml" is "OEBPS/Images/a.jpg".
    // Percent-encoding, the fragment and the query are removed.
    static QString resolvePath(const QString &basePath, const QString &href);
    // The fragment of a link ("c2.xhtml#note3" -> "note3"), "" if none.
    static QString fragmentOf(const QString &href);
    // Length of the text of an HTML page, for chapterWeight().
    static qint64 textLength(const QString &html);
};
