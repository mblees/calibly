// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QString>

#include <memory>
#include <vector>

class BookSource;
class QPainter;
class QTextDocument;

// How the reader sets the text of EPUB and text books (the text settings,
// "Aa"). Sizes in pixels.
struct TextStyle
{
    int fontPixelSize = 18;
    QString fontFamily;                         // "" = the book's own fonts ("Original")
    QString defaultFamily = QStringLiteral("Georgia"); // where the book names no font
    bool bold = false;
    double lineHeight = 1.6;                    // factor of the font height
    double letterSpacing = 0.0;                 // em
    double wordSpacing = 0.0;                   // em
    int margin = 56;                            // left and right of the text
    int verticalMargin = 40;                    // above and below the text
    bool justify = true;
    QColor textColor = Qt::black;
    QColor linkColor;                           // invalid = textColor

    bool operator==(const TextStyle &other) const
    {
        return fontPixelSize == other.fontPixelSize && fontFamily == other.fontFamily
            && defaultFamily == other.defaultFamily && bold == other.bold
            && qFuzzyCompare(lineHeight, other.lineHeight)
            && qFuzzyCompare(letterSpacing + 1, other.letterSpacing + 1)
            && qFuzzyCompare(wordSpacing + 1, other.wordSpacing + 1) && margin == other.margin
            && verticalMargin == other.verticalMargin && justify == other.justify
            && textColor == other.textColor && linkColor == other.linkColor;
    }
    bool operator!=(const TextStyle &other) const { return !(*this == other); }
};

// Lays the chapters of a book out as pages of one size (option A of the
// reader: QTextDocument with a page size) and draws them. Positions are
// characters in a chapter, so they survive another page size or style.
// Chapters are laid out when first needed; the last few stay in memory.
class BookLayout
{
public:
    explicit BookLayout(const BookSource *book);
    ~BookLayout();

    // Both throw away the laid out chapters.
    void setStyle(const TextStyle &style);
    void setPageSize(const QSizeF &size);
    const TextStyle &style() const { return m_style; }
    QSizeF pageSize() const { return m_pageSize; }

    int chapterCount() const;
    int pageCount(int chapter);
    // Characters in the chapter.
    int chapterLength(int chapter);
    // Page of the chapter that shows this character.
    int pageOf(int chapter, int offset);
    // First character on the page.
    int offsetOfPage(int chapter, int page);
    // Character where the fragment (an id, or "heading:<text>" for a
    // Markdown heading) is, -1 if it is not found.
    int offsetOfFragment(int chapter, const QString &fragment);
    // Link under the point (relative to the page), "" if none.
    QString linkAt(int chapter, int page, const QPointF &point);
    // Draws the page with its top left corner at topLeft. The background is
    // not drawn.
    void paintPage(QPainter *painter, int chapter, int page, const QPointF &topLeft);

    // The laid out chapter (owned by the layout), nullptr for an invalid one.
    QTextDocument *document(int chapter);

private:
    struct Cached
    {
        int chapter = -1;
        std::unique_ptr<QTextDocument> document;
    };

    const BookSource *m_book = nullptr;
    TextStyle m_style;
    QSizeF m_pageSize = QSizeF(600, 800);
    std::vector<Cached> m_cache; // most recently used last
};
