// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "readercontroller.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPainter>
#include <QUrl>
#include <QVariantMap>

#ifdef CALIBLY_HAS_QTPDF
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfSearchModel>
#endif

#include <algorithm>
#include <cmath>
#include <numeric>

#include "booklayout.h"
#include "booksource.h"
#include "epubbook.h"
#include "librarycontroller.h"
#include "readersettings.h"
#include "readingposition.h"
#include "textbook.h"
#include "textsearch.h"

namespace {

constexpr int kSaveDelayMs = 1500;
constexpr double kTextWidth = 680;          // longest line of text, in px
constexpr double kMinSpreadPageWidth = 440; // two pages only if each is this wide
constexpr double kZoomStep = 1.25;

enum class Kind { None, Epub, Pdf, Text };

Kind kindOf(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QLatin1String("epub"))
        return Kind::Epub;
#ifdef CALIBLY_HAS_QTPDF
    if (suffix == QLatin1String("pdf"))
        return Kind::Pdf;
#endif
    if (TextBook::isTextBook(filePath))
        return Kind::Text;
    return Kind::None;
}

} // namespace

ReaderController::ReaderController(LibraryController &library, ReaderSettings &settings, QQmlEngine *engine,
                                   QObject *parent)
    : QObject(parent)
    , m_library(library)
    , m_settings(settings)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &ReaderController::savePosition);
    m_countTimer.setInterval(0);
    connect(&m_countTimer, &QTimer::timeout, this, &ReaderController::countNextChapter);
    m_searchTimer.setInterval(0);
    connect(&m_searchTimer, &QTimer::timeout, this, &ReaderController::searchNextChapter);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ReaderController::savePosition);
    connect(&m_settings, &ReaderSettings::changed, this, &ReaderController::applyLayout);
    // The book is about to be merged or removed: store and let go of it.
    connect(&m_library, &LibraryController::worksAboutToChange, this, [this](const QStringList &workIds) {
        if (isOpen() && workIds.contains(m_workId))
            close();
    });
}

ReaderController::~ReaderController()
{
    close();
    s_instance = nullptr;
}

ReaderController *ReaderController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

// --- opening and closing -------------------------------------------------------

int ReaderController::readableFormat(const QString &workId) const
{
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(workId));
    if (!work)
        return -1;
    // Volumes of a combined work: the current one, if calibly can read it.
    if (work->isCombined() && work->formatCount(Format::Kind::Ebook) > 1) {
        const int current = work->currentFormat(Format::Kind::Ebook);
        return current >= 0 && kindOf(work->formats.at(current).filePath) != Kind::None ? current : -1;
    }
    // The same book in several files: EPUB before PDF before text.
    int best = -1;
    int bestRank = 99;
    for (int i = 0; i < work->formats.size(); ++i) {
        const Format &format = work->formats.at(i);
        if (format.kind != Format::Kind::Ebook)
            continue;
        const Kind kind = kindOf(format.filePath);
        const int rank = kind == Kind::Epub ? 0 : kind == Kind::Pdf ? 1 : kind == Kind::Text ? 2 : 99;
        if (rank < bestRank) {
            best = i;
            bestRank = rank;
        }
    }
    return best;
}

bool ReaderController::canReadFormat(const QString &workId, int formatIndex) const
{
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(workId));
    return work && formatIndex >= 0 && formatIndex < work->formats.size()
        && work->formats.at(formatIndex).kind == Format::Kind::Ebook
        && kindOf(work->formats.at(formatIndex).filePath) != Kind::None;
}

bool ReaderController::read(const QString &workId)
{
    if (canRead(workId))
        return open(workId);
    m_library.openFormat(workId, QStringLiteral("ebook"));
    return false;
}

bool ReaderController::open(const QString &workId)
{
    return openAt(workId, readableFormat(workId), Placement::Stored);
}

bool ReaderController::openFormat(const QString &workId, int formatIndex)
{
    return openAt(workId, formatIndex, Placement::Stored);
}

QList<int> ReaderController::volumes() const
{
    QList<int> list;
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(m_workId));
    if (!work || !work->isCombined())
        return m_formatIndex >= 0 ? QList<int>{ m_formatIndex } : list;
    for (int i = 0; i < work->formats.size(); ++i) {
        if (work->formats.at(i).kind == Format::Kind::Ebook)
            list.append(i);
    }
    return list;
}

int ReaderController::volumePosition() const
{
    return int(volumes().indexOf(m_formatIndex));
}

int ReaderController::volumeCount() const
{
    return isOpen() ? int(volumes().size()) : 0;
}

int ReaderController::volumeNumber() const
{
    return volumePosition() + 1;
}

bool ReaderController::openVolume(int position, Placement placement)
{
    const QList<int> list = volumes();
    if (position < 0 || position >= list.size())
        return false;
    return openAt(m_workId, list.at(position), placement);
}

bool ReaderController::openAt(QString workId, int formatIndex, Placement placement)
{
    // Another book or volume: store where this one was, without closing the
    // view in between.
    const bool wasOpen = isOpen();
    if (wasOpen) {
        savePosition();
        reset();
    }
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(workId));
    if (!work || formatIndex < 0 || formatIndex >= work->formats.size()
        || work->formats.at(formatIndex).kind != Format::Kind::Ebook
        || kindOf(work->formats.at(formatIndex).filePath) == Kind::None) {
        if (wasOpen) {
            emit openChanged();
            emit contentsChanged();
            emit positionChanged();
            emit layoutChanged();
        }
        return false;
    }

    const Format &format = work->formats.at(formatIndex);
    m_workId = workId;
    m_formatIndex = formatIndex;
    // A volume shows its own title.
    m_title = work->isCombined() && work->formatCount(Format::Kind::Ebook) > 1 ? work->formatTitle(formatIndex)
                                                                              : work->title;
    m_author = work->authors.join(QStringLiteral(", "));
    const QString path = format.filePath;
    const Kind kind = kindOf(path);
    m_type = kind == Kind::Pdf ? Pdf : Text;

    const bool exists = QFileInfo::exists(path);
    m_library.setFileMissing(workId, formatIndex, !exists);
    if (!exists) {
        m_error = FileMissing;
        emit openChanged();
        emit positionChanged();
        return true; // the view shows the message
    }

    // A finished book starts again from the beginning; turning into a
    // volume starts at its start (or end, see below).
    const ReadingPosition position = format.finished() || placement != Placement::Stored
                                         ? ReadingPosition()
                                         : ReadingPosition::fromString(format.readingPosition);
    bool readable = false;
    if (kind == Kind::Pdf) {
#ifdef CALIBLY_HAS_QTPDF
        m_pdf = new QPdfDocument(this);
        readable = m_pdf->load(path) == QPdfDocument::Error::None && m_pdf->pageCount() > 0;
        if (readable) {
            QPdfBookmarkModel bookmarks;
            bookmarks.setDocument(m_pdf);
            std::function<void(const QModelIndex &, int)> addLevel = [&](const QModelIndex &parent, int level) {
                for (int row = 0; row < bookmarks.rowCount(parent); ++row) {
                    const QModelIndex index = bookmarks.index(row, 0, parent);
                    Contents entry;
                    entry.title = index.data(int(QPdfBookmarkModel::Role::Title)).toString().simplified();
                    entry.level = level;
                    entry.page = index.data(int(QPdfBookmarkModel::Role::Page)).toInt();
                    if (!entry.title.isEmpty() && entry.page >= 0)
                        m_contents.append(entry);
                    addLevel(index, level + 1);
                }
            };
            addLevel(QModelIndex(), 0);
            m_pdfPage = std::clamp(position.page, 0, m_pdf->pageCount() - 1);
        }
#endif
    } else {
        std::unique_ptr<BookSource> book;
        QString error;
        if (kind == Kind::Epub) {
            auto epub = std::make_unique<EpubBook>();
            readable = epub->open(path, &error);
            book = std::move(epub);
        } else {
            auto text = std::make_unique<TextBook>();
            readable = text->open(path, &error);
            book = std::move(text);
        }
        if (!readable)
            qWarning("Reader: %s: %s", qPrintable(path), qPrintable(error));
        if (readable) {
            m_book = std::move(book);
            for (const QByteArray &font : m_book->fonts()) {
                const int id = QFontDatabase::addApplicationFontFromData(font);
                if (id >= 0)
                    m_fontIds.append(id);
            }
            for (const BookSource::ContentsEntry &entry : m_book->contents())
                m_contents.append({ entry.title, entry.level, entry.chapter, entry.fragment, -1 });
            m_layout = std::make_unique<BookLayout>(m_book.get());
            m_counter = std::make_unique<BookLayout>(m_book.get());
            m_chapter = position.chapter >= 0 && position.chapter < m_book->chapterCount() ? position.chapter : 0;
            m_anchor = position.chapter == m_chapter ? position.offset : 0;
            m_page = 0;
        }
    }
    if (!readable) {
        m_error = Unreadable;
        m_contents.clear();
        emit openChanged();
        emit contentsChanged();
        emit positionChanged();
        return true;
    }

    // Opened: for "Weitermachen" and the state "new".
    Work opened = *work;
    opened.markOpened(formatIndex, QDateTime::currentDateTime());
    m_library.saveWork(opened);

    emit openChanged();
    applyLayout();
    if (placement == Placement::End) {
        // Turned back from the next volume: its last page. Not stored until
        // the user turns a page here.
        goToEnd();
        m_moved = false;
        m_saveTimer.stop();
    }
    emit contentsChanged();
    // Another volume: the search goes on in it.
    if (!m_searchText.isEmpty())
        startSearch();
    return true;
}

void ReaderController::close()
{
    if (!isOpen())
        return;
    savePosition();
    reset();
    m_searchText.clear();
    emit openChanged();
    emit contentsChanged();
    emit positionChanged();
    emit layoutChanged();
    emit searchChanged();
}

void ReaderController::reset()
{
    m_saveTimer.stop();
    m_countTimer.stop();
    stopSearch(); // before the book and the PDF it searches go
    m_layout.reset();
    m_counter.reset();
    m_book.reset();
    for (int id : std::as_const(m_fontIds))
        QFontDatabase::removeApplicationFont(id);
    m_fontIds.clear();
#ifdef CALIBLY_HAS_QTPDF
    delete m_pdf;
#endif
    m_pdf = nullptr;
    m_pdfCache[0] = CachedImage();
    m_pdfCache[1] = CachedImage();
    m_workId.clear();
    m_formatIndex = -1;
    m_title.clear();
    m_author.clear();
    m_type = NoDocument;
    m_error = NoError;
    m_contents.clear();
    m_pageCounts.clear();
    m_chapter = 0;
    m_page = 0;
    m_anchor = 0;
    m_pdfPage = 0;
    m_zoomMode = FitPage;
    m_zoomFactor = 1.0;
    m_moved = false;
}

bool ReaderController::openExternally()
{
    if (!isOpen())
        return false;
    const QString workId = m_workId;
    const int formatIndex = m_formatIndex;
    close();
    return m_library.openFormatAt(workId, formatIndex);
}

void ReaderController::savePosition()
{
    m_saveTimer.stop();
    if (!isOpen() || m_error != NoError || !m_moved)
        return;
    const WorkModel *works = m_library.works();
    const Work *current = works->workAt(works->rowOf(m_workId));
    if (!current || m_formatIndex >= current->formats.size())
        return;

    const ReadingPosition position = m_type == Pdf ? ReadingPosition::onPage(m_pdfPage)
                                                   : ReadingPosition::inChapter(m_chapter, m_anchor, chapterTitle());
    const QString text = position.toString();
    const double value = progress();
    const Format &stored = current->formats.at(m_formatIndex);
    if (stored.readingPosition == text && qFuzzyCompare(stored.progress + 1, value + 1))
        return;
    Work work = *current;
    work.formats[m_formatIndex].readingPosition = text;
    work.formats[m_formatIndex].progress = value;
    m_library.saveWork(work);
}

// --- layout ---------------------------------------------------------------------

void ReaderController::setViewSize(const QSizeF &size)
{
    if (size == m_viewSize)
        return;
    m_viewSize = size;
    applyLayout();
}

void ReaderController::setTwoPagesWanted(bool wanted)
{
    if (wanted == m_twoPagesWanted)
        return;
    m_twoPagesWanted = wanted;
    applyLayout();
}

void ReaderController::setPageColor(const QColor &color)
{
    if (color == m_pageColor)
        return;
    m_pageColor = color;
    emit layoutChanged();
}

void ReaderController::setTextColor(const QColor &color)
{
    if (color == m_textColor)
        return;
    m_textColor = color;
    applyLayout();
}

void ReaderController::setLinkColor(const QColor &color)
{
    if (color == m_linkColor)
        return;
    m_linkColor = color;
    applyLayout();
}

bool ReaderController::twoPages() const
{
    return m_twoPagesWanted && m_viewSize.width() >= 2 * kMinSpreadPageWidth;
}

double ReaderController::pageWidth() const
{
    const double available = twoPages() ? m_viewSize.width() / 2 : m_viewSize.width();
    return std::floor(std::min(available, kTextWidth + 2 * m_settings.margin()));
}

QSizeF ReaderController::pageItemSize() const
{
    if (!isOpen() || m_error != NoError || m_viewSize.isEmpty())
        return QSizeF();
    if (m_type == Pdf) {
        const double scale = pdfScale();
        double width = 0;
        double height = 0;
        for (int i = 0; i < pagesShown(); ++i) {
            const QSizeF size = pdfPageSize(m_pdfPage + i);
            width += size.width() * scale;
            height = std::max(height, size.height() * scale);
        }
        return QSizeF(std::floor(width), std::floor(height));
    }
    return QSizeF(pageWidth() * pagesShown(), std::floor(m_viewSize.height()));
}

void ReaderController::applyLayout()
{
    if (m_layout && !m_viewSize.isEmpty()) {
        TextStyle style = m_settings.textStyle();
        style.textColor = m_textColor;
        style.linkColor = m_linkColor;
        const QSizeF pageSize(pageWidth(), std::floor(m_viewSize.height()));
        const bool changed = style != m_layout->style() || pageSize != m_layout->pageSize() || m_pageCounts.isEmpty();
        m_layout->setStyle(style);
        m_layout->setPageSize(pageSize);
        m_counter->setStyle(style);
        m_counter->setPageSize(pageSize);
        // The same text stays at the top of the page.
        m_page = firstOfSpread(m_layout->pageOf(m_chapter, m_anchor));
        if (changed)
            startCounting();
    }
    if (m_pdf)
        m_pdfPage = firstOfSpread(m_pdfPage);
    m_pdfCache[0] = CachedImage();
    m_pdfCache[1] = CachedImage();
    emit layoutChanged();
    emit positionChanged();
}

void ReaderController::startCounting()
{
    if (!m_book)
        return;
    m_pageCounts = QList<int>(m_book->chapterCount(), -1);
    m_countNext = 0;
    m_countTimer.start();
}

void ReaderController::countNextChapter()
{
    if (!m_counter || m_countNext >= m_pageCounts.size()) {
        m_countTimer.stop();
        return;
    }
    // The shown chapter is laid out anyway.
    m_pageCounts[m_countNext] = m_countNext == m_chapter ? m_layout->pageCount(m_chapter)
                                                         : m_counter->pageCount(m_countNext);
    ++m_countNext;
    if (m_countNext >= m_pageCounts.size()) {
        m_countTimer.stop();
        emit positionChanged();
        emit contentsChanged(); // the page numbers of the contents are known now
    }
}

int ReaderController::chapterStartPage(int chapter) const
{
    if (chapter < 0 || chapter > m_pageCounts.size())
        return -1;
    int start = 0;
    for (int i = 0; i < chapter; ++i) {
        if (m_pageCounts.at(i) < 0)
            return -1;
        start += m_pageCounts.at(i);
    }
    return start;
}

// --- state -----------------------------------------------------------------------

QVariantList ReaderController::contents() const
{
    QVariantList chapters;
    // Volumes indent the chapters of the open one below its title.
    const QList<int> list = volumes();
    const int indent = list.size() > 1 ? 1 : 0;
    for (const Contents &entry : m_contents) {
        int page = entry.page;
        if (m_type == Text) {
            const int start = chapterStartPage(entry.chapter);
            page = pageCount() > 0 ? start : -1; // fragments: the page the chapter starts on
        }
        chapters.append(QVariantMap{ { QStringLiteral("title"), entry.title },
                                     { QStringLiteral("level"), entry.level + indent },
                                     { QStringLiteral("page"), page } });
    }
    if (list.size() <= 1)
        return chapters;

    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(m_workId));
    QVariantList result;
    for (int formatIndex : list) {
        result.append(QVariantMap{ { QStringLiteral("title"), work ? work->formatTitle(formatIndex) : QString() },
                                   { QStringLiteral("level"), 0 },
                                   { QStringLiteral("page"), -1 } });
        if (formatIndex == m_formatIndex)
            result += chapters;
    }
    return result;
}

int ReaderController::contentsIndex() const
{
    const int chapter = chapterIndex();
    const int volume = volumePosition();
    if (volumes().size() <= 1 || volume < 0)
        return chapter;
    // The title of the open volume, then its chapters.
    return chapter < 0 ? volume : volume + 1 + chapter;
}

int ReaderController::chapterIndex() const
{
    int found = -1;
    for (int i = 0; i < m_contents.size(); ++i) {
        const Contents &entry = m_contents.at(i);
        if (m_type == Pdf) {
            if (entry.page <= m_pdfPage)
                found = i;
            continue;
        }
        if (entry.chapter < m_chapter) {
            found = i;
        } else if (entry.chapter == m_chapter && m_layout) {
            const int offset = entry.fragment.isEmpty() ? 0 : m_layout->offsetOfFragment(m_chapter, entry.fragment);
            const int lastShown = m_layout->offsetOfPage(m_chapter, m_page + pagesShown()) - 1;
            if (offset <= std::max(m_anchor, lastShown))
                found = i;
        }
    }
    return found;
}

QString ReaderController::chapterTitle() const
{
    const int index = chapterIndex();
    return index >= 0 ? m_contents.at(index).title : QString();
}

int ReaderController::pageCount() const
{
    if (m_type == Pdf)
        return m_pdf ? pdfPageCount() : 0;
    if (m_pageCounts.isEmpty() || std::any_of(m_pageCounts.cbegin(), m_pageCounts.cend(), [](int c) { return c < 0; }))
        return 0;
    return std::accumulate(m_pageCounts.cbegin(), m_pageCounts.cend(), 0);
}

int ReaderController::page() const
{
    if (m_type == Pdf)
        return m_pdf ? m_pdfPage : -1;
    const int start = chapterStartPage(m_chapter);
    return start < 0 || pageCount() == 0 ? -1 : start + m_page;
}

int ReaderController::lastPage() const
{
    const int first = page();
    if (first < 0)
        return -1;
    return std::min(first + pagesShown() - 1, pageCount() - 1);
}

bool ReaderController::atStart() const
{
    return bookAtStart() && volumePosition() <= 0;
}

bool ReaderController::atEnd() const
{
    return bookAtEnd() && volumePosition() >= volumes().size() - 1;
}

bool ReaderController::bookAtStart() const
{
    if (m_type == Pdf)
        return m_pdfPage == 0;
    return m_chapter == 0 && m_page == 0;
}

bool ReaderController::bookAtEnd() const
{
    if (m_type == Pdf)
        return !m_pdf || m_pdfPage + pagesShown() >= pdfPageCount();
    if (!m_layout || m_viewSize.isEmpty())
        return false;
    return m_chapter == m_book->chapterCount() - 1 && m_page + pagesShown() >= m_layout->pageCount(m_chapter);
}

double ReaderController::progress() const
{
    if (!isOpen() || m_error != NoError)
        return 0.0;
    if (bookAtEnd())
        return 1.0;
    if (m_type == Pdf)
        return m_pdf ? double(m_pdfPage + 1) / pdfPageCount() : 0.0;
    if (!m_book || !m_layout)
        return 0.0;
    qint64 total = 0;
    qint64 before = 0;
    for (int i = 0; i < m_book->chapterCount(); ++i) {
        total += m_book->chapterWeight(i);
        if (i < m_chapter)
            before += m_book->chapterWeight(i);
    }
    const int length = std::max(1, m_layout->chapterLength(m_chapter));
    const double inChapter = std::clamp(double(m_anchor) / length, 0.0, 1.0);
    const double value = (before + inChapter * m_book->chapterWeight(m_chapter)) / std::max<qint64>(1, total);
    return std::min(value, 0.999);
}

// --- moving ------------------------------------------------------------------------

void ReaderController::positionMoved()
{
    m_moved = true;
    m_pdfCache[0] = CachedImage();
    m_pdfCache[1] = CachedImage();
    emit positionChanged();
    if (m_type == Pdf)
        emit layoutChanged(); // the next page may have another size
    m_saveTimer.start();
}

void ReaderController::moveTo(int chapter, int page)
{
    if (!m_layout || !m_book)
        return;
    chapter = std::clamp(chapter, 0, m_book->chapterCount() - 1);
    page = firstOfSpread(std::clamp(page, 0, m_layout->pageCount(chapter) - 1));
    m_chapter = chapter;
    m_page = page;
    m_anchor = m_layout->offsetOfPage(chapter, page);
    positionMoved();
}

void ReaderController::movePdfTo(int page)
{
    if (!m_pdf)
        return;
    m_pdfPage = firstOfSpread(std::clamp(page, 0, pdfPageCount() - 1));
    positionMoved();
}

int ReaderController::lastSpreadStart(int count) const
{
    return firstOfSpread(std::max(0, count - 1));
}

void ReaderController::nextPage()
{
    if (!isOpen())
        return;
    // At the end of a volume (or one that cannot be shown): the next volume.
    if (m_error != NoError || bookAtEnd()) {
        if (const int volume = volumePosition(); volume >= 0 && volume + 1 < volumes().size())
            openVolume(volume + 1, Placement::Start);
        return;
    }
    if (m_type == Pdf) {
        movePdfTo(m_pdfPage + pagesShown());
        return;
    }
    if (m_page + pagesShown() < m_layout->pageCount(m_chapter))
        moveTo(m_chapter, m_page + pagesShown());
    else
        moveTo(m_chapter + 1, 0);
}

void ReaderController::previousPage()
{
    if (!isOpen())
        return;
    if (m_error != NoError || bookAtStart()) {
        if (const int volume = volumePosition(); volume > 0)
            openVolume(volume - 1, Placement::End);
        return;
    }
    if (m_type == Pdf) {
        movePdfTo(m_pdfPage - pagesShown());
        return;
    }
    if (m_page > 0)
        moveTo(m_chapter, m_page - pagesShown());
    else
        moveTo(m_chapter - 1, lastSpreadStart(m_layout->pageCount(m_chapter - 1)));
}

void ReaderController::goToStart()
{
    if (m_type == Pdf)
        movePdfTo(0);
    else if (m_layout)
        moveTo(0, 0);
}

void ReaderController::goToEnd()
{
    if (m_type == Pdf && m_pdf) {
        movePdfTo(lastSpreadStart(pdfPageCount()));
    } else if (m_layout) {
        const int last = m_book->chapterCount() - 1;
        moveTo(last, lastSpreadStart(m_layout->pageCount(last)));
    }
}

void ReaderController::seek(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (m_type == Pdf && m_pdf) {
        movePdfTo(int(std::round(value * (pdfPageCount() - 1))));
        return;
    }
    if (!m_layout)
        return;
    const int pages = pageCount();
    if (pages > 0) {
        // All pages are known: the page at this place of the book.
        int target = int(std::round(value * (pages - 1)));
        for (int chapter = 0; chapter < m_pageCounts.size(); ++chapter) {
            if (target < m_pageCounts.at(chapter) || chapter == m_pageCounts.size() - 1) {
                moveTo(chapter, target);
                return;
            }
            target -= m_pageCounts.at(chapter);
        }
        return;
    }
    // Still counting: by the length of the chapters.
    qint64 total = 0;
    for (int i = 0; i < m_book->chapterCount(); ++i)
        total += m_book->chapterWeight(i);
    double wanted = value * total;
    for (int chapter = 0; chapter < m_book->chapterCount(); ++chapter) {
        const qint64 weight = m_book->chapterWeight(chapter);
        if (wanted <= weight || chapter == m_book->chapterCount() - 1) {
            const int offset = int(std::clamp(wanted / weight, 0.0, 1.0) * m_layout->chapterLength(chapter));
            moveTo(chapter, m_layout->pageOf(chapter, offset));
            return;
        }
        wanted -= weight;
    }
}

void ReaderController::goToContents(int index)
{
    // With volumes: their titles, and the chapters of the open one after its title.
    if (const int volume = volumePosition(); volumes().size() > 1 && volume >= 0) {
        if (index < volume) {
            openVolume(index, Placement::Stored);
            return;
        }
        if (index == volume) {
            goToStart();
            return;
        }
        if (index > volume + m_contents.size()) {
            openVolume(int(index - m_contents.size()), Placement::Stored);
            return;
        }
        index -= volume + 1;
    }
    if (index < 0 || index >= m_contents.size())
        return;
    const Contents &entry = m_contents.at(index);
    if (m_type == Pdf) {
        movePdfTo(entry.page);
        return;
    }
    if (!m_layout)
        return;
    const int offset = entry.fragment.isEmpty() ? 0 : std::max(0, m_layout->offsetOfFragment(entry.chapter, entry.fragment));
    moveTo(entry.chapter, m_layout->pageOf(entry.chapter, offset));
}

QString ReaderController::linkAt(double x, double y)
{
    if (m_type != Text || !m_layout || m_viewSize.isEmpty())
        return QString();
    const double width = pageWidth();
    const int index = int(x / width);
    if (index < 0 || index >= pagesShown() || m_page + index >= m_layout->pageCount(m_chapter))
        return QString();
    return m_layout->linkAt(m_chapter, m_page + index, QPointF(x - index * width, y));
}

void ReaderController::followLink(const QString &href)
{
    if (href.isEmpty() || !m_book)
        return;
    const QUrl url(href);
    if (!url.scheme().isEmpty() && url.scheme() != QLatin1String("file")) {
        QDesktopServices::openUrl(url); // e.g. https: in the browser
        return;
    }
    const int chapter = m_book->chapterOf(BookSource::resolvePath(m_book->chapterPath(m_chapter), href));
    if (chapter < 0)
        return; // e.g. a page that is not part of the reading order
    const QString fragment = BookSource::fragmentOf(href);
    const int offset = fragment.isEmpty() ? 0 : std::max(0, m_layout->offsetOfFragment(chapter, fragment));
    moveTo(chapter, m_layout->pageOf(chapter, offset));
}

// --- PDF zoom ------------------------------------------------------------------------

QSizeF ReaderController::pdfPageSize(int page) const
{
#ifdef CALIBLY_HAS_QTPDF
    if (m_pdf && page >= 0 && page < m_pdf->pageCount())
        return m_pdf->pagePointSize(page);
#else
    Q_UNUSED(page)
#endif
    return QSizeF();
}

int ReaderController::pdfPageCount() const
{
#ifdef CALIBLY_HAS_QTPDF
    return m_pdf ? m_pdf->pageCount() : 0;
#else
    return 0;
#endif
}

double ReaderController::pdfScale() const
{
    const QSizeF page = pdfPageSize(m_pdfPage);
    if (page.isEmpty() || m_viewSize.isEmpty())
        return 1.0;
    const double width = page.width() * pagesShown();
    const double fit = std::min(m_viewSize.width() / width, m_viewSize.height() / page.height());
    switch (m_zoomMode) {
    case FitWidth:
        return m_viewSize.width() / width;
    case Custom:
        return fit * m_zoomFactor;
    case FitPage:
        break;
    }
    return fit;
}

double ReaderController::zoom() const
{
    return m_type == Pdf ? pdfScale() * 72.0 / 96.0 : 1.0; // 100 % = the page at 96 dpi
}

void ReaderController::fitPage()
{
    m_zoomMode = FitPage;
    m_zoomFactor = 1.0;
    applyLayout();
}

void ReaderController::fitWidth()
{
    m_zoomMode = FitWidth;
    applyLayout();
}

void ReaderController::zoomIn()
{
    const QSizeF page = pdfPageSize(m_pdfPage);
    if (m_zoomMode != Custom && !page.isEmpty() && !m_viewSize.isEmpty()) {
        // Start from what is shown now.
        const double fit = std::min(m_viewSize.width() / (page.width() * pagesShown()), m_viewSize.height() / page.height());
        m_zoomFactor = pdfScale() / fit;
    }
    m_zoomMode = Custom;
    m_zoomFactor = std::min(m_zoomFactor * kZoomStep, 8.0);
    applyLayout();
}

void ReaderController::zoomOut()
{
    const QSizeF page = pdfPageSize(m_pdfPage);
    if (m_zoomMode != Custom && !page.isEmpty() && !m_viewSize.isEmpty()) {
        const double fit = std::min(m_viewSize.width() / (page.width() * pagesShown()), m_viewSize.height() / page.height());
        m_zoomFactor = pdfScale() / fit;
    }
    m_zoomMode = Custom;
    m_zoomFactor = std::max(m_zoomFactor / kZoomStep, 0.25);
    applyLayout();
}

QImage ReaderController::pdfImage(int page, const QSize &size)
{
#ifdef CALIBLY_HAS_QTPDF
    for (CachedImage &cached : m_pdfCache) {
        if (cached.page == page && cached.image.size() == size)
            return cached.image;
    }
    if (!m_pdf || size.isEmpty())
        return QImage();
    QImage image = m_pdf->render(page, size);
    // One slot per shown page.
    m_pdfCache[std::clamp(page - m_pdfPage, 0, 1)] = { page, image };
    return image;
#else
    Q_UNUSED(page)
    Q_UNUSED(size)
    return QImage();
#endif
}

// --- painting --------------------------------------------------------------------------

void ReaderController::paint(QPainter *painter, const QSizeF &itemSize, qreal devicePixelRatio)
{
    if (!isOpen() || m_error != NoError || itemSize.isEmpty())
        return;
    if (m_type == Pdf) {
        const double scale = pdfScale();
        double x = 0;
        for (int i = 0; i < pagesShown() && m_pdf && m_pdfPage + i < pdfPageCount(); ++i) {
            const QSizeF size = pdfPageSize(m_pdfPage + i) * scale;
            const QRectF target(x, 0, size.width(), size.height());
            const QImage image = pdfImage(m_pdfPage + i, (size * devicePixelRatio).toSize());
            painter->fillRect(target, Qt::white);
            if (!image.isNull())
                painter->drawImage(target, image);
            paintPdfHighlights(painter, m_pdfPage + i, target.topLeft(), scale);
            x += size.width();
        }
        return;
    }
    if (!m_layout)
        return;
    painter->fillRect(QRectF(QPointF(0, 0), itemSize), m_pageColor);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    const double width = pageWidth();
    const int count = m_layout->pageCount(m_chapter);
    // The places found in this chapter; the current one more strongly.
    QList<BookLayout::Highlight> highlights;
    const QColor other = QColor(m_highlightColor.red(), m_highlightColor.green(), m_highlightColor.blue(),
                                m_highlightColor.alpha() * 2 / 5);
    for (int i = 0; i < m_searchResults.size(); ++i) {
        const SearchResult &result = m_searchResults.at(i);
        if (result.chapter == m_chapter)
            highlights.append({ result.offset, result.length, i == m_searchIndex ? m_highlightColor : other });
    }
    for (int i = 0; i < pagesShown() && m_page + i < count; ++i)
        m_layout->paintPage(painter, m_chapter, m_page + i, QPointF(i * width, 0), highlights);
}

// --- search in the book ----------------------------------------------------------------

void ReaderController::setSearchText(const QString &text)
{
    if (text == m_searchText)
        return;
    m_searchText = text;
    startSearch();
}

void ReaderController::setHighlightColor(const QColor &color)
{
    if (color == m_highlightColor)
        return;
    m_highlightColor = color;
    emit layoutChanged();
}

void ReaderController::stopSearch()
{
    m_searchTimer.stop();
    m_searchNext = -1;
    m_searchLayout.reset();
#ifdef CALIBLY_HAS_QTPDF
    delete m_pdfSearch;
#endif
    m_pdfSearch = nullptr;
    m_searchResults.clear();
    m_searchIndex = -1;
}

void ReaderController::startSearch()
{
    const bool hadResults = !m_searchResults.isEmpty();
    stopSearch();
    if (TextSearch::normalizedQuery(m_searchText).isEmpty() || !isOpen() || m_error != NoError) {
        emit searchChanged();
        if (hadResults)
            emit layoutChanged(); // no highlights any more
        return;
    }
    if (m_book && m_layout) {
        // A layout of its own, so the shown chapters stay in the cache of
        // m_layout. The text does not depend on the page size or the style.
        m_searchLayout = std::make_unique<BookLayout>(m_book.get());
        m_searchLayout->setStyle(m_layout->style());
        m_searchNext = 0;
        m_searchTimer.start();
    }
#ifdef CALIBLY_HAS_QTPDF
    if (m_pdf) {
        m_pdfSearch = new QPdfSearchModel(this);
        connect(m_pdfSearch, &QPdfSearchModel::countChanged, this, &ReaderController::takePdfResults);
        connect(m_pdfSearch, &QPdfSearchModel::statusChanged, this, &ReaderController::takePdfResults);
        m_pdfSearch->setDocument(m_pdf);
        m_pdfSearch->setSearchString(m_searchText.simplified());
    }
#endif
    emit searchChanged();
    emit layoutChanged();
}

void ReaderController::searchNextChapter()
{
    if (!m_book || !m_searchLayout || m_searchNext < 0 || m_searchNext >= m_book->chapterCount()
        || searchLimited()) {
        m_searchTimer.stop();
        m_searchNext = -1;
        emit searchChanged();
        return;
    }
    const int chapter = m_searchNext++;
    const QString text = m_searchLayout->chapterText(chapter);
    const QList<TextSearch::Match> matches
        = TextSearch::findAll(text, m_searchText, kMaxSearchResults - int(m_searchResults.size()));
    for (const TextSearch::Match &match : matches) {
        const TextSearch::Excerpt excerpt = TextSearch::excerpt(text, match);
        SearchResult result;
        result.chapter = chapter;
        result.offset = match.offset;
        result.length = match.length;
        result.title = contentsTitleAt(chapter, match.offset);
        result.before = excerpt.before;
        result.match = excerpt.match;
        result.after = excerpt.after;
        m_searchResults.append(result);
    }
    if (m_searchNext >= m_book->chapterCount() || searchLimited()) {
        m_searchTimer.stop();
        m_searchNext = -1;
    }
    if (!matches.isEmpty() || m_searchNext < 0)
        emit searchChanged();
    if (chapter == m_chapter && !matches.isEmpty())
        emit layoutChanged(); // highlights on the shown page
}

void ReaderController::takePdfResults()
{
#ifdef CALIBLY_HAS_QTPDF
    if (!m_pdfSearch)
        return;
    const int count = std::min(m_pdfSearch->count(), kMaxSearchResults);
    bool shownPage = false;
    for (int i = int(m_searchResults.size()); i < count; ++i) {
        const QPdfLink link = m_pdfSearch->resultAtIndex(i);
        if (!link.isValid())
            break;
        SearchResult result;
        result.page = link.page();
        result.rects = link.rectangles();
        result.title = tr("Seite %1").arg(result.page + 1);
        result.before = link.contextBefore().simplified();
        result.match = m_searchText.simplified();
        result.after = link.contextAfter().simplified();
        if (!result.before.isEmpty())
            result.before.prepend(QStringLiteral("…"));
        if (!result.after.isEmpty())
            result.after.append(QStringLiteral("…"));
        shownPage = shownPage || (result.page >= m_pdfPage && result.page < m_pdfPage + pagesShown());
        m_searchResults.append(result);
    }
    emit searchChanged();
    if (shownPage)
        emit layoutChanged();
#endif
}

bool ReaderController::searching() const
{
#ifdef CALIBLY_HAS_QTPDF
    if (m_pdfSearch)
        return m_pdfSearch->status() != QPdfSearchModel::Status::Finished && !searchLimited();
#endif
    return m_searchNext >= 0;
}

QString ReaderController::contentsTitleAt(int chapter, int offset) const
{
    // The last entry of the contents that starts before the place.
    QString title;
    for (const Contents &entry : m_contents) {
        if (entry.chapter > chapter)
            break;
        if (entry.chapter == chapter && !entry.fragment.isEmpty() && m_searchLayout) {
            const int start = m_searchLayout->offsetOfFragment(chapter, entry.fragment);
            if (start > offset)
                break;
        }
        title = entry.title;
    }
    return title;
}

QVariantList ReaderController::searchResults() const
{
    QVariantList list;
    list.reserve(m_searchResults.size());
    for (const SearchResult &result : m_searchResults) {
        list.append(QVariantMap{ { QStringLiteral("title"), result.title },
                                 { QStringLiteral("before"), result.before },
                                 { QStringLiteral("match"), result.match },
                                 { QStringLiteral("after"), result.after } });
    }
    return list;
}

void ReaderController::goToSearchResult(int index)
{
    if (index < 0 || index >= m_searchResults.size())
        return;
    m_searchIndex = index;
    const SearchResult &result = m_searchResults.at(index);
    if (m_type == Pdf) {
        movePdfTo(result.page);
    } else if (m_layout && m_book) {
        m_chapter = std::clamp(result.chapter, 0, m_book->chapterCount() - 1);
        m_page = firstOfSpread(m_layout->pageOf(m_chapter, result.offset));
        // The place stays on the page when the layout changes.
        m_anchor = result.offset;
        positionMoved();
    }
    emit searchChanged();
    emit layoutChanged();
}

int ReaderController::searchResultFromHere(bool forward) const
{
    // Before the shown page.
    const auto before = [this](const SearchResult &result) {
        if (m_type == Pdf)
            return result.page < m_pdfPage;
        return result.chapter < m_chapter
            || (result.chapter == m_chapter && m_layout
                && result.offset < m_layout->offsetOfPage(m_chapter, m_page));
    };
    if (forward) {
        for (int i = 0; i < m_searchResults.size(); ++i) {
            if (!before(m_searchResults.at(i)))
                return i;
        }
        return m_searchResults.isEmpty() ? -1 : 0;
    }
    for (int i = int(m_searchResults.size()) - 1; i >= 0; --i) {
        if (before(m_searchResults.at(i)))
            return i;
    }
    return int(m_searchResults.size()) - 1;
}

void ReaderController::nextSearchResult()
{
    if (m_searchResults.isEmpty())
        return;
    goToSearchResult(m_searchIndex < 0 ? searchResultFromHere(true)
                                       : (m_searchIndex + 1) % int(m_searchResults.size()));
}

void ReaderController::previousSearchResult()
{
    if (m_searchResults.isEmpty())
        return;
    const int count = int(m_searchResults.size());
    goToSearchResult(m_searchIndex < 0 ? searchResultFromHere(false) : (m_searchIndex - 1 + count) % count);
}

void ReaderController::paintPdfHighlights(QPainter *painter, int page, const QPointF &topLeft, double scale)
{
    // PDF pages are pictures: the highlight lies over the text and must let
    // it show through.
    for (int i = 0; i < m_searchResults.size(); ++i) {
        const SearchResult &result = m_searchResults.at(i);
        if (result.page != page || result.rects.isEmpty())
            continue;
        QColor color = m_highlightColor;
        color.setAlphaF(color.alphaF() * (i == m_searchIndex ? 0.5 : 0.2));
        for (const QRectF &rect : result.rects)
            painter->fillRect(QRectF(topLeft + rect.topLeft() * scale, rect.size() * scale), color);
    }
}
