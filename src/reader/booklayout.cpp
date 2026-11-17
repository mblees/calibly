// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "booklayout.h"

#include <QAbstractTextDocumentLayout>
#include <QImage>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextLayout>
#include <QUrl>

#include <algorithm>
#include <cmath>

#include "booksource.h"
#include "epubbook.h"
#include "htmlprep.h"

namespace {

constexpr int kCachedChapters = 3;

// One chapter as a document. It loads images and style sheets from the
// book; images larger than the text area are scaled down to fit.
class ChapterDocument : public QTextDocument
{
public:
    ChapterDocument(const BookSource *book, int chapter, const QSizeF &textArea)
        : m_book(book)
        , m_chapterPath(book->chapterPath(chapter))
        , m_textArea(textArea)
    {
    }

protected:
    QVariant loadResource(int type, const QUrl &name) override
    {
        const QString path = BookSource::resolvePath(m_chapterPath, name.toString(QUrl::FullyEncoded));
        const QByteArray data = m_book->resource(path);
        if (data.isEmpty())
            return QTextDocument::loadResource(type, name);
        if (type == QTextDocument::ImageResource) {
            QImage image;
            if (!image.loadFromData(data))
                return QVariant();
            // A little smaller than the text area, so the line of the image fits too.
            const QSize limit = (m_textArea - QSizeF(0, 8)).toSize();
            if (limit.isValid() && (image.width() > limit.width() || image.height() > limit.height()))
                image = image.scaled(limit, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return image;
        }
        return data;
    }

private:
    const BookSource *m_book;
    QString m_chapterPath;
    QSizeF m_textArea;
};

// Applies the text settings to every paragraph and piece of text.
void applyStyle(QTextDocument *document, const TextStyle &style)
{
    const QColor link = style.linkColor.isValid() ? style.linkColor : style.textColor;
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        // The fragments first: changing them merges and splits fragments.
        struct Piece { int start; int length; QTextCharFormat format; };
        std::vector<Piece> pieces;
        bool hasImage = false;
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;
            pieces.push_back({ fragment.position(), fragment.length(), fragment.charFormat() });
            hasImage = hasImage || fragment.charFormat().isImageFormat();
        }

        QTextBlockFormat blockFormat = block.blockFormat();
        // Not for images: a page-high picture would no longer fit on the page.
        if (!hasImage)
            blockFormat.setLineHeight(style.lineHeight * 100, QTextBlockFormat::ProportionalHeight);
        const Qt::Alignment horizontal = blockFormat.alignment() & Qt::AlignHorizontal_Mask;
        const bool plain = horizontal == Qt::AlignLeft || horizontal == Qt::AlignLeading || horizontal == 0
                           || horizontal == (Qt::AlignLeft | Qt::AlignAbsolute);
        if (style.justify && plain && blockFormat.headingLevel() == 0)
            blockFormat.setAlignment(Qt::AlignJustify);
        else if (!style.justify && horizontal == Qt::AlignJustify)
            blockFormat.setAlignment(Qt::AlignLeft);
        blockFormat.clearBackground();
        cursor.setPosition(block.position());
        cursor.setBlockFormat(blockFormat);

        for (const Piece &piece : pieces) {
            if (piece.format.isImageFormat())
                continue;
            QTextCharFormat change;
            const bool isLink = piece.format.isAnchor() && !piece.format.anchorHref().isEmpty();
            change.setForeground(isLink ? link : style.textColor);
            change.setBackground(QBrush());
            if (!style.fontFamily.isEmpty())
                change.setFontFamilies(QStringList{ style.fontFamily });
            if (style.bold) {
                const int weight = piece.format.hasProperty(QTextFormat::FontWeight) ? piece.format.fontWeight()
                                                                                    : int(QFont::Normal);
                change.setFontWeight(std::min(int(QFont::Black), weight + 300));
            }
            if (style.letterSpacing > 0) {
                change.setFontLetterSpacingType(QFont::AbsoluteSpacing);
                change.setFontLetterSpacing(style.letterSpacing * style.fontPixelSize);
            }
            if (style.wordSpacing > 0)
                change.setFontWordSpacing(style.wordSpacing * style.fontPixelSize);
            cursor.setPosition(piece.start);
            cursor.setPosition(piece.start + piece.length, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(change);
        }
    }
    cursor.endEditBlock();
}

} // namespace

BookLayout::BookLayout(const BookSource *book)
    : m_book(book)
{
}

BookLayout::~BookLayout() = default;

void BookLayout::setStyle(const TextStyle &style)
{
    if (style == m_style)
        return;
    m_style = style;
    m_cache.clear();
}

void BookLayout::setPageSize(const QSizeF &size)
{
    const QSizeF rounded(std::floor(size.width()), std::floor(size.height()));
    if (rounded == m_pageSize || rounded.isEmpty())
        return;
    m_pageSize = rounded;
    m_cache.clear();
}

int BookLayout::chapterCount() const
{
    return m_book ? m_book->chapterCount() : 0;
}

QTextDocument *BookLayout::document(int chapter)
{
    if (!m_book || chapter < 0 || chapter >= m_book->chapterCount())
        return nullptr;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->chapter == chapter) {
            // Most recently used goes last.
            std::rotate(it, it + 1, m_cache.end());
            return m_cache.back().document.get();
        }
    }

    const QSizeF textArea(m_pageSize.width() - 2 * m_style.margin, m_pageSize.height() - 2 * m_style.verticalMargin);
    auto document = std::make_unique<ChapterDocument>(m_book, chapter, textArea);
    document->setUndoRedoEnabled(false);
    QFont font(m_style.fontFamily.isEmpty() ? m_style.defaultFamily : m_style.fontFamily);
    font.setPixelSize(m_style.fontPixelSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    document->setDefaultFont(font);
    document->setPageSize(m_pageSize);

    const QString text = m_book->chapterText(chapter);
    switch (m_book->chapterFormat(chapter)) {
    case BookSource::TextFormat::Markdown:
        document->setMarkdown(text);
        break;
    case BookSource::TextFormat::PlainText:
        document->setPlainText(text);
        break;
    case BookSource::TextFormat::Html: {
        const QString chapterPath = m_book->chapterPath(chapter);
        const BookSource *book = m_book;
        document->setHtml(HtmlPrep::prepare(text, m_style.fontFamily.isEmpty(), m_style.fontPixelSize,
                                            [book, chapterPath](const QString &href) {
            return decodeText(book->resource(BookSource::resolvePath(chapterPath, href)));
        }));
        break;
    }
    }
    // After the text: setHtml() and friends reset the root frame.
    document->setDocumentMargin(0);
    QTextFrameFormat frame = document->rootFrame()->frameFormat();
    frame.setLeftMargin(m_style.margin);
    frame.setRightMargin(m_style.margin);
    frame.setTopMargin(m_style.verticalMargin);
    frame.setBottomMargin(m_style.verticalMargin);
    document->rootFrame()->setFrameFormat(frame);
    applyStyle(document.get(), m_style);

    if (m_cache.size() >= kCachedChapters)
        m_cache.erase(m_cache.begin());
    m_cache.push_back({ chapter, std::move(document) });
    return m_cache.back().document.get();
}

int BookLayout::pageCount(int chapter)
{
    QTextDocument *doc = document(chapter);
    return doc ? std::max(1, doc->pageCount()) : 0;
}

int BookLayout::chapterLength(int chapter)
{
    QTextDocument *doc = document(chapter);
    return doc ? std::max(0, doc->characterCount() - 1) : 0;
}

int BookLayout::pageOf(int chapter, int offset)
{
    QTextDocument *doc = document(chapter);
    if (!doc)
        return 0;
    offset = std::clamp(offset, 0, std::max(0, doc->characterCount() - 1));
    const QTextBlock block = doc->findBlock(offset);
    if (!block.isValid())
        return 0;
    double y = doc->documentLayout()->blockBoundingRect(block).top();
    if (const QTextLayout *layout = block.layout()) {
        const QTextLine line = layout->lineForTextPosition(offset - block.position());
        if (line.isValid())
            y += line.y() + line.height() / 2;
    }
    const int page = int(y / m_pageSize.height());
    return std::clamp(page, 0, pageCount(chapter) - 1);
}

int BookLayout::offsetOfPage(int chapter, int page)
{
    QTextDocument *doc = document(chapter);
    if (!doc || page <= 0)
        return 0;
    const double top = page * m_pageSize.height();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        const QRectF rect = layout->blockBoundingRect(block);
        if (rect.bottom() < top)
            continue;
        const QTextLayout *lines = block.layout();
        for (int i = 0; lines && i < lines->lineCount(); ++i) {
            const QTextLine line = lines->lineAt(i);
            if (rect.top() + line.y() + line.height() / 2 >= top)
                return block.position() + line.textStart();
        }
    }
    return std::max(0, doc->characterCount() - 1);
}

int BookLayout::offsetOfFragment(int chapter, const QString &fragment)
{
    QTextDocument *doc = document(chapter);
    if (!doc || fragment.isEmpty())
        return -1;
    // Markdown heading, see TextBook.
    if (fragment.startsWith(QLatin1String("heading:"))) {
        const QString title = fragment.mid(8);
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            if (block.blockFormat().headingLevel() > 0 && block.text().trimmed() == title)
                return block.position();
        }
        return -1;
    }
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment piece = it.fragment();
            if (piece.isValid() && piece.charFormat().anchorNames().contains(fragment))
                return piece.position();
        }
    }
    return -1;
}

QString BookLayout::linkAt(int chapter, int page, const QPointF &point)
{
    QTextDocument *doc = document(chapter);
    if (!doc)
        return QString();
    return doc->documentLayout()->anchorAt(QPointF(point.x(), page * m_pageSize.height() + point.y()));
}

QString BookLayout::chapterText(int chapter)
{
    QTextDocument *doc = document(chapter);
    return doc ? doc->toRawText() : QString();
}

void BookLayout::paintPage(QPainter *painter, int chapter, int page, const QPointF &topLeft,
                           const QList<Highlight> &highlights)
{
    QTextDocument *doc = document(chapter);
    if (!doc)
        return;
    const double height = m_pageSize.height();
    painter->save();
    painter->translate(topLeft.x(), topLeft.y() - page * height);
    QAbstractTextDocumentLayout::PaintContext context;
    context.clip = QRectF(0, page * height, m_pageSize.width(), height);
    context.palette.setColor(QPalette::Text, m_style.textColor);
    for (const Highlight &highlight : highlights) {
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor = QTextCursor(doc);
        selection.cursor.setPosition(highlight.offset);
        selection.cursor.setPosition(highlight.offset + highlight.length, QTextCursor::KeepAnchor);
        selection.format.setBackground(highlight.color);
        context.selections.append(selection);
    }
    painter->setClipRect(context.clip);
    doc->documentLayout()->draw(painter, context);
    painter->restore();
}
