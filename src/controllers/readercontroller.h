// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>
#include <QImage>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QSizeF>
#include <QTimer>
#include <QVariantList>

#include <memory>

class BookLayout;
class BookSource;
class LibraryController;
class QPainter;
class QPdfDocument;
class ReaderSettings;

// The ebook reader (phase 10): opens the EPUB, PDF or text file of a work,
// turns pages and remembers where the user stopped. EPUBs and text books are
// laid out by BookLayout with the text settings (ReaderSettings); PDFs are
// drawn page by page with Qt PDF. BookPage draws what this controller shows.
//
// In QML (import Calibly.Controllers):
//   ReaderController.read(workId)   opens the ebook in calibly if it can
//                                   (EPUB, PDF, TXT, Markdown), otherwise
//                                   with the default application; true = in calibly
//   ReaderController.open(workId)   opens it in calibly
//   ReaderController.openFormat(workId, formatIndex)   that ebook format,
//                                   e.g. one volume of a combined work
//   ReaderController.close()        stores the position and closes the book
//
// A combined work (phase 12) has its volumes as ebook formats: read() and
// open() take the current one (Work::currentFormat()) at its position;
// turning past the last page of a volume opens the next one at its start,
// back from the first page the previous one at its end. The contents list
// the volumes, with the chapters of the open one below it.
//   volumeCount     ebook formats (volumes) of the work, 1 for a single book
//   volumeNumber    the open one, 1-based
//   nextPage(), previousPage(), goToStart(), goToEnd(), seek(progress),
//   goToContents(index), linkAt(x, y) / followLink(href), openExternally()
//   PDF: fitPage(), fitWidth(), zoomIn(), zoomOut()
//
// Properties:
//   isOpen, workId, title, author, type (NoDocument, Text, Pdf),
//   error (NoError, FileMissing, Unreadable)
//   contents        [{ title, level, page (-1 = not known yet) }]
//   contentsIndex   the entry the shown page belongs to, -1 = none
//   chapterTitle    its title
//   page            first shown page of the book (0-based), -1 = not known yet
//   lastPage        last shown page (two pages: page + 1)
//   pageCount       pages of the book, 0 = still being counted (EPUB pages
//                   depend on window and text settings and are counted in
//                   the background)
//   progress        0 – 1 through the book (the volume); 1 = the last page is shown
//   atStart, atEnd  the first / last page of the first / last volume
//   viewSize        the area the pages may use (set by the view)
//   twoPagesWanted  show two pages side by side if the area is wide enough
//   twoPages        two pages are shown
//   pageItemSize    size of the BookPage item (a zoomed PDF may be larger
//                   than viewSize)
//   pageColor, textColor, linkColor   colors of EPUB / text pages
//   zoomMode (FitPage, FitWidth, Custom), zoom (PDF, 1 = 100 %)
//
// The position is stored (Format::readingPosition and the progress) a moment
// after turning a page, when the book is closed and when the app quits – only
// once the user moved, so opening a finished book does not undo "finished".
// Opening a book counts as opened ("Weitermachen"); the last page marks it
// as finished. A finished book opens at its start again.
class ReaderController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY openChanged)
    Q_PROPERTY(QString workId READ workId NOTIFY openChanged)
    Q_PROPERTY(QString title READ title NOTIFY openChanged)
    Q_PROPERTY(QString author READ author NOTIFY openChanged)
    Q_PROPERTY(DocumentType type READ type NOTIFY openChanged)
    Q_PROPERTY(Error error READ error NOTIFY openChanged)
    Q_PROPERTY(QVariantList contents READ contents NOTIFY contentsChanged)
    Q_PROPERTY(int contentsIndex READ contentsIndex NOTIFY positionChanged)
    Q_PROPERTY(QString chapterTitle READ chapterTitle NOTIFY positionChanged)
    Q_PROPERTY(int page READ page NOTIFY positionChanged)
    Q_PROPERTY(int lastPage READ lastPage NOTIFY positionChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY positionChanged)
    Q_PROPERTY(double progress READ progress NOTIFY positionChanged)
    Q_PROPERTY(bool atStart READ atStart NOTIFY positionChanged)
    Q_PROPERTY(bool atEnd READ atEnd NOTIFY positionChanged)
    Q_PROPERTY(int volumeCount READ volumeCount NOTIFY openChanged)
    Q_PROPERTY(int volumeNumber READ volumeNumber NOTIFY openChanged)
    Q_PROPERTY(QSizeF viewSize READ viewSize WRITE setViewSize NOTIFY layoutChanged)
    Q_PROPERTY(bool twoPagesWanted READ twoPagesWanted WRITE setTwoPagesWanted NOTIFY layoutChanged)
    Q_PROPERTY(bool twoPages READ twoPages NOTIFY layoutChanged)
    Q_PROPERTY(QSizeF pageItemSize READ pageItemSize NOTIFY layoutChanged)
    Q_PROPERTY(QColor pageColor READ pageColor WRITE setPageColor NOTIFY layoutChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY layoutChanged)
    Q_PROPERTY(QColor linkColor READ linkColor WRITE setLinkColor NOTIFY layoutChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY layoutChanged)
    Q_PROPERTY(double zoom READ zoom NOTIFY layoutChanged)

public:
    enum DocumentType { NoDocument, Text, Pdf };
    Q_ENUM(DocumentType)
    enum Error { NoError, FileMissing, Unreadable };
    Q_ENUM(Error)
    enum ZoomMode { FitPage, FitWidth, Custom };
    Q_ENUM(ZoomMode)

    ReaderController(LibraryController &library, ReaderSettings &settings, QQmlEngine *engine,
                     QObject *parent = nullptr);
    ~ReaderController() override;

    static ReaderController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static ReaderController *instance() { return s_instance; }

    bool isOpen() const { return !m_workId.isEmpty(); }
    QString workId() const { return m_workId; }
    QString title() const { return m_title; }
    QString author() const { return m_author; }
    DocumentType type() const { return m_type; }
    Error error() const { return m_error; }
    QVariantList contents() const;
    int contentsIndex() const;
    QString chapterTitle() const;
    int page() const;
    int lastPage() const;
    int pageCount() const;
    double progress() const;
    bool atStart() const;
    bool atEnd() const;
    int volumeCount() const;
    int volumeNumber() const;
    QSizeF viewSize() const { return m_viewSize; }
    void setViewSize(const QSizeF &size);
    bool twoPagesWanted() const { return m_twoPagesWanted; }
    void setTwoPagesWanted(bool wanted);
    bool twoPages() const;
    QSizeF pageItemSize() const;
    QColor pageColor() const { return m_pageColor; }
    void setPageColor(const QColor &color);
    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor &color);
    QColor linkColor() const { return m_linkColor; }
    void setLinkColor(const QColor &color);
    ZoomMode zoomMode() const { return m_zoomMode; }
    double zoom() const;

    // The format of the work calibly can read itself, -1 if there is none.
    int readableFormat(const QString &workId) const;
    Q_INVOKABLE bool canRead(const QString &workId) const { return readableFormat(workId) >= 0; }
    // That ebook format (e.g. one volume) can be read in calibly.
    Q_INVOKABLE bool canReadFormat(const QString &workId, int formatIndex) const;
    Q_INVOKABLE bool read(const QString &workId);
    Q_INVOKABLE bool open(const QString &workId);
    Q_INVOKABLE bool openFormat(const QString &workId, int formatIndex);
    Q_INVOKABLE void close();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();
    Q_INVOKABLE void goToStart();
    Q_INVOKABLE void goToEnd();
    Q_INVOKABLE void seek(double progress);
    Q_INVOKABLE void goToContents(int index);
    Q_INVOKABLE QString linkAt(double x, double y);
    Q_INVOKABLE void followLink(const QString &href);
    Q_INVOKABLE bool openExternally();
    Q_INVOKABLE void fitPage();
    Q_INVOKABLE void fitWidth();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

    // Draws the shown page(s) into an item of this size (see BookPage).
    void paint(QPainter *painter, const QSizeF &itemSize, qreal devicePixelRatio);
    // Stores the position in the library now.
    void savePosition();

signals:
    void openChanged();
    void contentsChanged();
    void positionChanged();
    void layoutChanged();

private:
    struct Contents
    {
        QString title;
        int level = 0;
        int chapter = -1;   // text books
        QString fragment;
        int page = -1;      // PDF
    };

    // Where a volume opens: its stored position, its start or its end.
    enum class Placement { Stored, Start, End };
    bool openAt(QString workId, int formatIndex, Placement placement); // a copy: may be m_workId
    QList<int> volumes() const;  // the ebook formats of the open work
    int volumePosition() const;  // the open one in volumes(), -1 = none
    bool openVolume(int position, Placement placement);
    int chapterIndex() const;    // the entry of m_contents the shown page belongs to
    bool bookAtStart() const;    // first / last page of the open volume
    bool bookAtEnd() const;
    void reset();
    void applyLayout();         // page size and style changed: lay out again, keep the place
    void startCounting();
    void countNextChapter();
    void moveTo(int chapter, int page);
    void movePdfTo(int page);
    void positionMoved();        // emit, store a moment later
    double pageWidth() const;    // width of one text page
    int pagesShown() const { return twoPages() ? 2 : 1; }
    int firstOfSpread(int page) const { return twoPages() ? page - page % 2 : page; }
    int lastSpreadStart(int count) const;
    int chapterStartPage(int chapter) const; // -1 if not counted yet
    int pdfPageCount() const;
    double pdfScale() const;     // pixels per point
    QSizeF pdfPageSize(int page) const;
    QImage pdfImage(int page, const QSize &size);

    LibraryController &m_library;
    ReaderSettings &m_settings;
    QQmlEngine *m_engine = nullptr;

    QString m_workId;
    int m_formatIndex = -1;
    QString m_title;
    QString m_author;
    DocumentType m_type = NoDocument;
    Error m_error = NoError;
    QList<Contents> m_contents;

    // Text books
    std::unique_ptr<BookSource> m_book;
    std::unique_ptr<BookLayout> m_layout;
    std::unique_ptr<BookLayout> m_counter; // counts the pages in the background
    QList<int> m_pageCounts;               // per chapter, -1 = not counted yet
    int m_countNext = 0;
    QTimer m_countTimer;
    QList<int> m_fontIds;
    int m_chapter = 0;
    int m_page = 0;                        // first shown page of the chapter
    int m_anchor = 0;                      // first character on that page

    // PDF
    QPdfDocument *m_pdf = nullptr;
    int m_pdfPage = 0;
    ZoomMode m_zoomMode = FitPage;
    double m_zoomFactor = 1.0;             // Custom: relative to "fit page"
    struct CachedImage { int page = -1; QImage image; };
    CachedImage m_pdfCache[2];

    QSizeF m_viewSize;
    bool m_twoPagesWanted = false;
    QColor m_pageColor = QColor(0x12, 0x13, 0x16);
    QColor m_textColor = QColor(0xd9, 0xd9, 0xde);
    QColor m_linkColor;
    QTimer m_saveTimer;
    bool m_moved = false;                  // the user turned a page since the book was opened

    static inline ReaderController *s_instance = nullptr;
};
