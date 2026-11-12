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
    // EPUB before PDF before text, if a work has several.
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

bool ReaderController::read(const QString &workId)
{
    if (canRead(workId))
        return open(workId);
    m_library.openFormat(workId, QStringLiteral("ebook"));
    return false;
}

bool ReaderController::open(const QString &workId)
{
    close();
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(workId));
    const int formatIndex = readableFormat(workId);
    if (!work || formatIndex < 0)
        return false;

    const Format &format = work->formats.at(formatIndex);
    m_workId = workId;
    m_formatIndex = formatIndex;
    m_title = work->title;
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

    // A finished book starts again from the beginning.
    const ReadingPosition position = format.finished() ? ReadingPosition()
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
    emit contentsChanged();
    return true;
}

void ReaderController::close()
{
    if (!isOpen())
        return;
    savePosition();
    reset();
    emit openChanged();
    emit contentsChanged();
    emit positionChanged();
    emit layoutChanged();
}

void ReaderController::reset()
{
    m_saveTimer.stop();
    m_countTimer.stop();
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
    close();
    return m_library.openFormat(workId, QStringLiteral("ebook"));
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
    QVariantList list;
    for (const Contents &entry : m_contents) {
        int page = entry.page;
        if (m_type == Text) {
            const int start = chapterStartPage(entry.chapter);
            page = pageCount() > 0 ? start : -1; // fragments: the page the chapter starts on
        }
        list.append(QVariantMap{ { QStringLiteral("title"), entry.title },
                                 { QStringLiteral("level"), entry.level },
                                 { QStringLiteral("page"), page } });
    }
    return list;
}

int ReaderController::contentsIndex() const
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
    const int index = contentsIndex();
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
    if (m_type == Pdf)
        return m_pdfPage == 0;
    return m_chapter == 0 && m_page == 0;
}

bool ReaderController::atEnd() const
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
    if (atEnd())
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
    if (!isOpen() || m_error != NoError || atEnd())
        return;
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
    if (!isOpen() || m_error != NoError || atStart())
        return;
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
    for (int i = 0; i < pagesShown() && m_page + i < count; ++i)
        m_layout->paintPage(painter, m_chapter, m_page + i, QPointF(i * width, 0));
}
