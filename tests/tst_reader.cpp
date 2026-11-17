// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QAbstractTextDocumentLayout>
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextCursor>
#include <QTextLayout>

#include <private/qzipwriter_p.h>

#include "booklayout.h"
#include "booksource.h"
#include "epubbook.h"
#include "htmlprep.h"
#include "readingposition.h"
#include "textbook.h"
#include "textsearch.h"

namespace {

QByteArray png(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::darkCyan);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

QString paragraphs(int count, const QString &word = QStringLiteral("Wort"))
{
    QString html;
    for (int i = 0; i < count; ++i)
        html += QStringLiteral("<p>Absatz %1: ").arg(i) + QStringList(40, word).join(u' ') + QStringLiteral("</p>");
    return html;
}

QByteArray xhtml(const QString &body, const QString &head = QString())
{
    return (QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE html>\n"
                           "<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">"
                           "<head><title>t</title>") + head + QStringLiteral("</head><body>") + body
            + QStringLiteral("</body></html>")).toUtf8();
}

// An EPUB 3 with a navigation document, or an EPUB 2 with an NCX.
void writeEpub(const QString &path, bool epub3)
{
    QZipWriter zip(path);
    zip.setCompressionPolicy(QZipWriter::NeverCompress);
    zip.addFile(QStringLiteral("mimetype"), "application/epub+zip");
    zip.addFile(QStringLiteral("META-INF/container.xml"),
                "<?xml version='1.0'?><container version='1.0' xmlns='urn:oasis:names:tc:opendocument:xmlns:container'>"
                "<rootfiles><rootfile full-path='OEBPS/content.opf' media-type='application/oebps-package+xml'/></rootfiles></container>");
    QByteArray opf = "<?xml version='1.0'?><package xmlns='http://www.idpf.org/2007/opf' version='3.0'>"
                     "<metadata xmlns:dc='http://purl.org/dc/elements/1.1/'><dc:title>Die Verwandlung</dc:title></metadata>"
                     "<manifest>"
                     "<item id='cover' href='Text/cover.xhtml' media-type='application/xhtml+xml'/>"
                     "<item id='c1' href='Text/kapitel%201.xhtml' media-type='application/xhtml+xml'/>"
                     "<item id='c2' href='Text/c2.xhtml' media-type='application/xhtml+xml'/>"
                     "<item id='notes' href='Text/notes.xhtml' media-type='application/xhtml+xml'/>"
                     "<item id='css' href='Styles/book.css' media-type='text/css'/>"
                     "<item id='img' href='Images/cover.png' media-type='image/png'/>"
                     "<item id='font' href='Fonts/book.ttf' media-type='font/ttf'/>";
    if (epub3)
        opf += "<item id='nav' href='nav.xhtml' media-type='application/xhtml+xml' properties='nav'/>";
    else
        opf += "<item id='ncx' href='toc.ncx' media-type='application/x-dtbncx+xml'/>";
    opf += "</manifest><spine";
    opf += epub3 ? ">" : " toc='ncx'>";
    opf += "<itemref idref='cover'/><itemref idref='c1'/><itemref idref='c2'/><itemref idref='notes' linear='no'/></spine></package>";
    zip.addFile(QStringLiteral("OEBPS/content.opf"), opf);

    zip.addFile(QStringLiteral("OEBPS/Text/cover.xhtml"),
                xhtml(QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>"
                                     "<image xlink:href='../Images/cover.png'/></svg>")));
    zip.addFile(QStringLiteral("OEBPS/Text/kapitel 1.xhtml"),
                xhtml(QStringLiteral("<h1 id='start'>I</h1>") + paragraphs(30)
                          + QStringLiteral("<p id='middle'>Die Mitte</p>") + paragraphs(30)
                          + QStringLiteral("<p><a href='c2.xhtml#zwei'>Weiter</a> und <a href='https://example.org'>Netz</a></p>"),
                      QStringLiteral("<link rel='stylesheet' type='text/css' href='../Styles/book.css'/>")));
    zip.addFile(QStringLiteral("OEBPS/Text/c2.xhtml"),
                xhtml(QStringLiteral("<h1>II</h1><p style='color: red; margin-left: 2em'>Anfang</p><p id='zwei'>Zwei</p>")
                      + paragraphs(5)));
    zip.addFile(QStringLiteral("OEBPS/Text/notes.xhtml"), xhtml(QStringLiteral("<p>Anmerkungen</p>")));
    zip.addFile(QStringLiteral("OEBPS/Styles/book.css"),
                "p { color: #333; font-size: 30px; text-indent: 1.5em; font-family: 'Book Font'; }");
    zip.addFile(QStringLiteral("OEBPS/Images/cover.png"), png(1200, 1800));
    zip.addFile(QStringLiteral("OEBPS/Fonts/book.ttf"), "not really a font");
    if (epub3) {
        zip.addFile(QStringLiteral("OEBPS/nav.xhtml"),
                    xhtml(QStringLiteral("<nav epub:type='landmarks'><ol><li><a href='Text/cover.xhtml'>Cover</a></li></ol></nav>"
                                         "<nav epub:type='toc'><ol>"
                                         "<li><a href='Text/kapitel%201.xhtml'>Kapitel&nbsp;I</a><ol>"
                                         "<li><a href='Text/kapitel%201.xhtml#middle'>Die <em>Mitte</em></a></li></ol></li>"
                                         "<li><a href='Text/c2.xhtml'>Kapitel II</a></li>"
                                         "<li><a href='Text/missing.xhtml'>Fehlt</a></li>"
                                         "</ol></nav>")));
    } else {
        zip.addFile(QStringLiteral("OEBPS/toc.ncx"),
                    "<?xml version='1.0'?><ncx xmlns='http://www.daisy.org/z3986/2005/ncx/' version='2005-1'><navMap>"
                    "<navPoint id='n1'><navLabel><text>Kapitel I</text></navLabel><content src='Text/kapitel%201.xhtml'/>"
                    "<navPoint id='n2'><navLabel><text>Die Mitte</text></navLabel><content src='Text/kapitel%201.xhtml#middle'/></navPoint>"
                    "</navPoint>"
                    "<navPoint id='n3'><navLabel><text>Kapitel II</text></navLabel><content src='Text/c2.xhtml'/></navPoint>"
                    "</navMap></ncx>");
    }
    zip.close();
}

} // namespace

class TestReader : public QObject
{
    Q_OBJECT

private slots:
    void paths()
    {
        QCOMPARE(BookSource::resolvePath(QStringLiteral("OEBPS/Text/c1.xhtml"), QStringLiteral("../Images/a%20b.jpg")),
                 QStringLiteral("OEBPS/Images/a b.jpg"));
        QCOMPARE(BookSource::resolvePath(QStringLiteral("OEBPS/Text/c1.xhtml"), QStringLiteral("c2.xhtml#x")),
                 QStringLiteral("OEBPS/Text/c2.xhtml"));
        QCOMPARE(BookSource::resolvePath(QStringLiteral("OEBPS/Text/c1.xhtml"), QStringLiteral("#x")),
                 QStringLiteral("OEBPS/Text/c1.xhtml"));
        QCOMPARE(BookSource::resolvePath(QStringLiteral("c.xhtml"), QStringLiteral("./img/a.png")), QStringLiteral("img/a.png"));
        QCOMPARE(BookSource::fragmentOf(QStringLiteral("c2.xhtml#note%201")), QStringLiteral("note 1"));
        QCOMPARE(BookSource::fragmentOf(QStringLiteral("c2.xhtml")), QString());
        QCOMPARE(BookSource::textLength(QStringLiteral("<head><title>xx</title></head><p>ab &amp; c</p>")), 6);
    }

    void readingPosition()
    {
        const ReadingPosition epub = ReadingPosition::inChapter(3, 1234, QStringLiteral("Kapitel II"));
        QCOMPARE(ReadingPosition::fromString(epub.toString()), epub);
        const ReadingPosition pdf = ReadingPosition::onPage(11);
        QCOMPARE(ReadingPosition::fromString(pdf.toString()).page, 11);
        QVERIFY(!ReadingPosition::fromString(QString()).isValid());
        QVERIFY(!ReadingPosition::fromString(QStringLiteral("kaputt")).isValid());
        QVERIFY(ReadingPosition().toString().isEmpty());
    }

    void cleanCss()
    {
        QCOMPARE(HtmlPrep::cleanDeclarations(QStringLiteral("color: red; margin-left: 2em; FONT-SIZE: 3em; text-align:center"), false),
                 QStringLiteral("margin-left: 2em; text-align:center"));
        QCOMPARE(HtmlPrep::cleanDeclarations(QStringLiteral("font-family: Book; line-height: 2"), true),
                 QStringLiteral("font-family: Book"));
        const QString css = HtmlPrep::cleanCss(QStringLiteral("/* x */ @font-face { src: url(a.ttf) } "
                                                              "p { color: #333; text-indent: 1em } a:hover { background: red }"),
                                               false);
        QVERIFY(!css.contains(QLatin1String("font-face")));
        QVERIFY(css.contains(QLatin1String("p { text-indent: 1em }")) || css.contains(QLatin1String("p {text-indent: 1em}")));
        QVERIFY(!css.contains(QLatin1String("color")));
        QVERIFY(!css.contains(QLatin1String("background")));

        QCOMPARE(HtmlPrep::cleanDeclarations(QStringLiteral("text-indent: 1.5em; margin: 0.5rem 1em -2em 12pt"), false, 18),
                 QStringLiteral("text-indent: 27px; margin: 9px 18px -36px 16px"));

        const QString html = HtmlPrep::prepare(
            QStringLiteral("<?xml version='1.0'?><html><head><link rel='stylesheet' href='a.css'/></head><body>"
                           "<svg><image xlink:href='../i.png'/></svg><h1 id='h'>T</h1><a id='x' href='#h'>L</a>"
                           "<table id='t'><tr><td>c</td></tr></table></body></html>"),
            false, 18, [](const QString &href) { return href == QLatin1String("a.css") ? QStringLiteral("p{color:red;margin:0}") : QString(); });
        QVERIFY(!html.contains(QLatin1String("<?xml")));
        QVERIFY(html.contains(QLatin1String("<style>p{margin:0}</style>")));
        QVERIFY(html.contains(QLatin1String("<img src=\"../i.png\" />")));
        QVERIFY(html.contains(QLatin1String("<h1 id='h'><a name=\"h\"></a>")));
        QVERIFY(html.contains(QLatin1String("name=\"x\"")));
        QVERIFY(html.contains(QLatin1String("<a name=\"t\"></a><table id='t'>"))); // before, not inside the table
    }

    void openEpub_data()
    {
        QTest::addColumn<bool>("epub3");
        QTest::newRow("EPUB 3 nav") << true;
        QTest::newRow("EPUB 2 NCX") << false;
    }

    void openEpub()
    {
        QFETCH(bool, epub3);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("book.epub"));
        writeEpub(path, epub3);

        EpubBook book;
        QString error;
        QVERIFY2(book.open(path, &error), qPrintable(error));
        QCOMPARE(book.title(), QStringLiteral("Die Verwandlung"));
        QCOMPARE(book.chapterCount(), 3); // the non-linear notes are left out
        QCOMPARE(book.chapterPath(1), QStringLiteral("OEBPS/Text/kapitel 1.xhtml"));
        QVERIFY(book.chapterText(2).contains(QLatin1String("Anfang")));
        QVERIFY(book.chapterWeight(1) > book.chapterWeight(2));
        QCOMPARE(book.chapterOf(QStringLiteral("oebps/text/C2.xhtml")), 2); // letter case does not matter
        QVERIFY(!book.resource(QStringLiteral("OEBPS/Images/cover.png")).isEmpty());
        QVERIFY(!book.resource(QStringLiteral("oebps/images/COVER.png")).isEmpty());
        QCOMPARE(book.fonts().size(), 1);

        const QList<BookSource::ContentsEntry> contents = book.contents();
        QCOMPARE(contents.size(), 3); // the missing file is left out, the landmarks too
        QCOMPARE(contents.at(0).title, QStringLiteral("Kapitel I").replace(u' ', epub3 ? u' ' : u' ').simplified());
        QCOMPARE(contents.at(0).chapter, 1);
        QCOMPARE(contents.at(0).level, 0);
        QCOMPARE(contents.at(1).title, QStringLiteral("Die Mitte"));
        QCOMPARE(contents.at(1).level, 1);
        QCOMPARE(contents.at(1).fragment, QStringLiteral("middle"));
        QCOMPARE(contents.at(2).chapter, 2);
    }

    void brokenEpub()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("broken.epub"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("no zip");
        file.close();
        EpubBook book;
        QString error;
        QVERIFY(!book.open(path, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!book.open(dir.filePath(QStringLiteral("missing.epub"))));
    }

    // Pages, positions and links of the laid out chapters.
    void layout()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("book.epub"));
        writeEpub(path, true);
        EpubBook book;
        QVERIFY(book.open(path));

        BookLayout layout(&book);
        TextStyle style;
        style.textColor = QColor(QStringLiteral("#3b2f22"));
        layout.setStyle(style);
        layout.setPageSize(QSizeF(600, 800));

        // The cover: the SVG became an image, scaled to the text area.
        QTextDocument *cover = layout.document(0);
        QVERIFY(cover);
        QCOMPARE(layout.pageCount(0), 1);
        QVERIFY(cover->toHtml().contains(QLatin1String("<img")));

        const int pages = layout.pageCount(1);
        QVERIFY2(pages >= 3, qPrintable(QString::number(pages)));
        QCOMPARE(layout.offsetOfPage(1, 0), 0);
        for (int page = 1; page < pages; ++page) {
            const int offset = layout.offsetOfPage(1, page);
            QVERIFY(offset > layout.offsetOfPage(1, page - 1));
            QCOMPARE(layout.pageOf(1, offset), page); // round trip
        }
        QCOMPARE(layout.pageOf(1, layout.chapterLength(1)), pages - 1);

        // The margins of the settings, on every page: the text starts below
        // the top margin and keeps off the left edge.
        QTextDocument *text = layout.document(1);
        QCOMPARE(text->rootFrame()->frameFormat().leftMargin(), qreal(style.margin));
        const QTextBlock first = text->findBlock(0);
        const QRectF firstRect = text->documentLayout()->blockBoundingRect(first);
        QVERIFY2(firstRect.left() >= style.margin, qPrintable(QString::number(firstRect.left())));
        QVERIFY2(firstRect.top() + first.layout()->lineAt(0).y() >= style.verticalMargin - 1,
                 qPrintable(QString::number(firstRect.top())));
        const QTextBlock onSecond = text->findBlock(layout.offsetOfPage(1, 1));
        const QTextLine line = onSecond.layout()->lineForTextPosition(layout.offsetOfPage(1, 1) - onSecond.position());
        const double yOnPage = text->documentLayout()->blockBoundingRect(onSecond).top() + line.y() - 800;
        QVERIFY2(yOnPage >= style.verticalMargin - 1, qPrintable(QString::number(yOnPage)));

        // The book's CSS: indent kept, color and size replaced by the settings.
        QTextDocument *chapter = layout.document(1);
        const QTextBlock paragraph = chapter->findBlock(layout.offsetOfPage(1, 1));
        QVERIFY(paragraph.blockFormat().textIndent() > 0);
        QCOMPARE(paragraph.blockFormat().alignment() & Qt::AlignHorizontal_Mask, Qt::AlignJustify);
        QCOMPARE(int(paragraph.blockFormat().lineHeight()), 160);
        QCOMPARE(paragraph.begin().fragment().charFormat().foreground().color(), style.textColor);
        QVERIFY(chapter->findBlock(0).blockFormat().headingLevel() > 0
                || (chapter->findBlock(0).blockFormat().alignment() & Qt::AlignJustify) == 0); // headings not justified

        // Fragments and links.
        const int middle = layout.offsetOfFragment(1, QStringLiteral("middle"));
        QVERIFY(middle > 0);
        QVERIFY(chapter->toPlainText().mid(middle, 9) == QStringLiteral("Die Mitte"));
        QCOMPARE(layout.offsetOfFragment(1, QStringLiteral("start")), 0);
        QCOMPARE(layout.offsetOfFragment(1, QStringLiteral("unknown")), -1);
        QVERIFY(layout.offsetOfFragment(2, QStringLiteral("zwei")) > 0);
        QString link;
        const int last = pages - 1;
        for (int y = 0; y < 800 && link.isEmpty(); y += 4) {
            for (int x = 56; x < 300 && link.isEmpty(); x += 4)
                link = layout.linkAt(1, last, QPointF(x, y));
        }
        QCOMPARE(link, QStringLiteral("c2.xhtml#zwei"));

        // Painting does not crash and draws something.
        QImage image(600, 800, QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        layout.paintPage(&painter, 1, 1, QPointF(0, 0));
        painter.end();
        bool drawn = false;
        for (int y = 40; y < 760 && !drawn; y += 3)
            drawn = image.pixel(300, y) != qRgb(255, 255, 255);
        QVERIFY(drawn);
    }

    // The place in the chapter stays when page size or text settings change.
    void positionSurvivesRelayout()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("book.epub"));
        writeEpub(path, true);
        EpubBook book;
        QVERIFY(book.open(path));
        BookLayout layout(&book);
        layout.setPageSize(QSizeF(600, 800));
        const int offset = layout.offsetOfPage(1, 2);
        const int pages = layout.pageCount(1);

        layout.setPageSize(QSizeF(400, 500));
        QVERIFY(layout.pageCount(1) > pages);
        const int page = layout.pageOf(1, offset);
        QVERIFY(layout.offsetOfPage(1, page) <= offset);
        QVERIFY(page + 1 >= layout.pageCount(1) || layout.offsetOfPage(1, page + 1) > offset);

        TextStyle large;
        large.fontPixelSize = 30;
        large.lineHeight = 2.0;
        large.fontFamily = QStringLiteral("Verdana");
        large.bold = true;
        large.justify = false;
        layout.setStyle(large);
        QVERIFY(layout.pageCount(1) > pages);
        const QTextBlock block = layout.document(1)->findBlock(layout.offsetOfPage(1, 1));
        QVERIFY(block.begin().fragment().charFormat().fontWeight() > QFont::Normal);
        QVERIFY((block.blockFormat().alignment() & Qt::AlignJustify) == 0);
    }

    void textBook()
    {
        QTemporaryDir dir;
        const QString md = dir.filePath(QStringLiteral("Notizen.md"));
        QFile file(md);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("# Titel\n\nText mit **fett**.\n\n```\n# kein Kapitel\n```\n\n## Zweites Kapitel\n\nMehr Text.\n");
        file.close();
        QVERIFY(TextBook::isTextBook(md));
        QVERIFY(!TextBook::isTextBook(dir.filePath(QStringLiteral("a.epub"))));

        TextBook book;
        QVERIFY(book.open(md));
        QCOMPARE(book.title(), QStringLiteral("Notizen"));
        QCOMPARE(book.chapterCount(), 1);
        QCOMPARE(book.contents().size(), 2);
        QCOMPARE(book.contents().at(1).title, QStringLiteral("Zweites Kapitel"));
        QCOMPARE(book.contents().at(1).level, 1);

        BookLayout layout(&book);
        layout.setPageSize(QSizeF(600, 800));
        QCOMPARE(layout.pageCount(0), 1);
        QVERIFY(layout.offsetOfFragment(0, book.contents().at(1).fragment) > 0);
        QVERIFY(!layout.document(0)->toPlainText().contains(QLatin1String("**")));

        TextBook plain;
        const QString txt = dir.filePath(QStringLiteral("a.txt"));
        QFile textFile(txt);
        QVERIFY(textFile.open(QIODevice::WriteOnly));
        textFile.write("# kein Markdown");
        textFile.close();
        QVERIFY(plain.open(txt));
        QVERIFY(plain.contents().isEmpty());
        BookLayout plainLayout(&plain);
        QVERIFY(plainLayout.document(0)->toPlainText().startsWith(QLatin1String("# kein")));
    }

    // --- search in the book (phase 14) ---------------------------------------

    void textSearch()
    {
        using TextSearch::Match;
        QCOMPARE(TextSearch::normalizedQuery(QStringLiteral("  Das   ZIEL ")), QStringLiteral("das ziel"));
        QVERIFY(TextSearch::normalizedQuery(QStringLiteral(" \t ")).isEmpty());
        QVERIFY(TextSearch::findAll(QStringLiteral("abc"), QStringLiteral("  ")).isEmpty());

        // Case does not count; matches do not overlap.
        QCOMPARE(TextSearch::findAll(QStringLiteral("Wort wort WORT"), QStringLiteral("wort")),
                 (QList<Match>{ { 0, 4 }, { 5, 4 }, { 10, 4 } }));
        QCOMPARE(TextSearch::findAll(QStringLiteral("aaaa"), QStringLiteral("aa")), (QList<Match>{ { 0, 2 }, { 2, 2 } }));
        QCOMPARE(TextSearch::findAll(QStringLiteral("Wort wort WORT"), QStringLiteral("wort"), 2).size(), 2);

        // Every kind of space and line break is one space.
        const QString breaks = QStringLiteral("das Ziel und das  Ziel, dasZiel");
        QCOMPARE(TextSearch::findAll(breaks, QStringLiteral("das ziel")), (QList<Match>{ { 0, 8 }, { 13, 9 } }));

        // Soft hyphens in the text are skipped; accents count.
        const QString hyphen = QStringLiteral("Der Kapi­tän und der Kapitan");
        QCOMPARE(TextSearch::findAll(hyphen, QStringLiteral("kapitän")), (QList<Match>{ { 4, 8 } }));
        QVERIFY(TextSearch::findAll(QStringLiteral("Bär"), QStringLiteral("bar")).isEmpty());
    }

    void textSearchExcerpt()
    {
        const QString text = QStringLiteral("Am Anfang war das Wort und das Wort war bei Gott, und Gott war das Wort. "
                                            "Dasselbe war im Anfang bei Gott.");
        const QList<TextSearch::Match> matches = TextSearch::findAll(text, QStringLiteral("gott,"));
        QCOMPARE(matches.size(), 1);
        const TextSearch::Excerpt excerpt = TextSearch::excerpt(text, matches.first(), 20);
        QCOMPARE(excerpt.match, QStringLiteral("Gott,"));
        // Cut at word borders, with "…" where it was cut.
        QCOMPARE(excerpt.before, QStringLiteral("…das Wort war bei "));
        QCOMPARE(excerpt.after, QStringLiteral(" und Gott war das…"));

        // The start of the text is not cut; a paragraph break is a space.
        const QString paragraphs = QStringLiteral("Erster Absatz Zweiter");
        const TextSearch::Excerpt short_ = TextSearch::excerpt(paragraphs, { 0, 6 });
        QCOMPARE(short_.before, QString());
        QCOMPARE(short_.match, QStringLiteral("Erster"));
        QCOMPARE(short_.after, QStringLiteral(" Absatz Zweiter"));
    }

    // Offsets in chapterText() are positions in the laid out chapter, so a
    // place found there is the place that is highlighted and shown.
    void chapterTextMatchesLayout()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("book.epub"));
        writeEpub(path, true);
        EpubBook book;
        QVERIFY(book.open(path));
        BookLayout layout(&book);
        layout.setPageSize(QSizeF(600, 800));

        const QString text = layout.chapterText(1);
        const QList<TextSearch::Match> matches = TextSearch::findAll(text, QStringLiteral("die mitte"));
        QCOMPARE(matches.size(), 1);
        QTextCursor cursor(layout.document(1));
        cursor.setPosition(matches.first().offset);
        cursor.setPosition(matches.first().offset + matches.first().length, QTextCursor::KeepAnchor);
        QCOMPARE(cursor.selectedText(), QStringLiteral("Die Mitte"));
        QCOMPARE(matches.first().offset, layout.offsetOfFragment(1, QStringLiteral("middle")));
        QVERIFY(layout.pageOf(1, matches.first().offset) > 0);
        QVERIFY(layout.chapterText(99).isEmpty());

        // A highlight is drawn behind the text.
        const int page = layout.pageOf(1, matches.first().offset);
        QImage plain(600, 800, QImage::Format_RGB32);
        plain.fill(Qt::white);
        QImage marked = plain;
        {
            QPainter painter(&plain);
            layout.paintPage(&painter, 1, page, QPointF(0, 0));
        }
        {
            QPainter painter(&marked);
            layout.paintPage(&painter, 1, page, QPointF(0, 0),
                             { { matches.first().offset, matches.first().length, QColor(0, 255, 0) } });
        }
        QVERIFY(plain != marked);
        int green = 0;
        for (int y = 0; y < marked.height(); ++y) {
            for (int x = 0; x < marked.width(); ++x)
                green += marked.pixel(x, y) == qRgb(0, 255, 0) ? 1 : 0;
        }
        QVERIFY(green > 100);
    }
};

QTEST_MAIN(TestReader)
#include "tst_reader.moc"
