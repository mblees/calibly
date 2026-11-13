#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPdfWriter>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <private/qzipwriter_p.h>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "readercontroller.h"
#include "readersettings.h"
#include "readingposition.h"
#include "workmerge.h"

namespace {

QString paragraphs(int count)
{
    QString html;
    for (int i = 0; i < count; ++i)
        html += QStringLiteral("<p>Absatz %1: ").arg(i) + QStringList(40, QStringLiteral("Wort")).join(u' ')
                + QStringLiteral("</p>");
    return html;
}

QByteArray xhtml(const QString &body)
{
    return (QStringLiteral("<?xml version='1.0' encoding='UTF-8'?><html xmlns='http://www.w3.org/1999/xhtml'>"
                           "<head><title>t</title></head><body>") + body + QStringLiteral("</body></html>")).toUtf8();
}

// Three chapters: a short one, a long one, a middle one; contents per chapter.
void writeEpub(const QString &path)
{
    QZipWriter zip(path);
    zip.addFile(QStringLiteral("mimetype"), "application/epub+zip");
    zip.addFile(QStringLiteral("META-INF/container.xml"),
                "<?xml version='1.0'?><container version='1.0' xmlns='urn:oasis:names:tc:opendocument:xmlns:container'>"
                "<rootfiles><rootfile full-path='content.opf' media-type='application/oebps-package+xml'/></rootfiles></container>");
    zip.addFile(QStringLiteral("content.opf"),
                "<?xml version='1.0'?><package xmlns='http://www.idpf.org/2007/opf' version='3.0'>"
                "<metadata xmlns:dc='http://purl.org/dc/elements/1.1/'><dc:title>Buch</dc:title></metadata><manifest>"
                "<item id='c0' href='c0.xhtml' media-type='application/xhtml+xml'/>"
                "<item id='c1' href='c1.xhtml' media-type='application/xhtml+xml'/>"
                "<item id='c2' href='c2.xhtml' media-type='application/xhtml+xml'/>"
                "<item id='nav' href='nav.xhtml' media-type='application/xhtml+xml' properties='nav'/>"
                "</manifest><spine><itemref idref='c0'/><itemref idref='c1'/><itemref idref='c2'/></spine></package>");
    zip.addFile(QStringLiteral("c0.xhtml"), xhtml(QStringLiteral("<h1>Titel</h1><p><a href='c2.xhtml#ziel'>Zum Ziel</a></p>")));
    zip.addFile(QStringLiteral("c1.xhtml"), xhtml(QStringLiteral("<h1>Eins</h1>") + paragraphs(60)));
    zip.addFile(QStringLiteral("c2.xhtml"), xhtml(QStringLiteral("<h1>Zwei</h1>") + paragraphs(10)
                                                  + QStringLiteral("<p id='ziel'>Das Ziel</p>") + paragraphs(10)));
    zip.addFile(QStringLiteral("nav.xhtml"),
                xhtml(QStringLiteral("<nav xmlns:epub='http://www.idpf.org/2007/ops' epub:type='toc'><ol>"
                                     "<li><a href='c0.xhtml'>Titel</a></li><li><a href='c1.xhtml'>Kapitel Eins</a></li>"
                                     "<li><a href='c2.xhtml'>Kapitel Zwei</a><ol><li><a href='c2.xhtml#ziel'>Ziel</a></li></ol></li>"
                                     "</ol></nav>")));
    zip.close();
}

void writePdf(const QString &path, int pages)
{
    QPdfWriter writer(path);
    QPainter painter(&writer);
    for (int i = 0; i < pages; ++i) {
        if (i > 0)
            writer.newPage();
        painter.fillRect(QRect(0, 0, 400, 400), i % 2 ? Qt::blue : Qt::red);
    }
}

class MemoryBackend : public LibraryBackend
{
public:
    QList<Work> works;
    QString root;
    QString libraryRoot() const override { return root; }
    QList<Work> loadWorks() override { return works; }
    bool saveWork(const Work &work) override
    {
        for (Work &existing : works) {
            if (existing.id == work.id) {
                existing = work;
                return true;
            }
        }
        works.append(work);
        return true;
    }
    bool removeWork(const QString &id) override
    {
        return works.removeIf([&id](const Work &work) { return work.id == id; }) > 0;
    }
    const Format &format(const QString &id) const
    {
        for (const Work &work : works) {
            if (work.id == id)
                return work.formats.first();
        }
        Q_ASSERT(false);
        return works.first().formats.first();
    }
};

Work ebookWork(const QString &id, const QString &path)
{
    Work work;
    work.id = id;
    work.title = id;
    work.authors = { QStringLiteral("Franz Kafka") };
    Format format;
    format.kind = Format::Kind::Ebook;
    format.filePath = path;
    work.formats = { format };
    return work;
}

} // namespace

class TestReaderController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_readercontroller"));
        QVERIFY(m_dir.isValid());
        m_epub = m_dir.filePath(QStringLiteral("buch.epub"));
        writeEpub(m_epub);
        m_pdf = m_dir.filePath(QStringLiteral("scan.pdf"));
        writePdf(m_pdf, 5);
        QFile broken(m_dir.filePath(QStringLiteral("kaputt.epub")));
        QVERIFY(broken.open(QIODevice::WriteOnly));
        broken.write("kein zip");
        broken.close();
        QFile mobi(m_dir.filePath(QStringLiteral("alt.mobi")));
        QVERIFY(mobi.open(QIODevice::WriteOnly));
        mobi.write("x");
    }

    void init()
    {
        QSettings().clear();
        m_backend = std::make_unique<MemoryBackend>();
        m_backend->root = m_dir.path();
        m_backend->works = { ebookWork(QStringLiteral("epub"), m_epub), ebookWork(QStringLiteral("pdf"), m_pdf),
                             ebookWork(QStringLiteral("kaputt"), m_dir.filePath(QStringLiteral("kaputt.epub"))),
                             ebookWork(QStringLiteral("fehlt"), m_dir.filePath(QStringLiteral("fehlt.epub"))),
                             ebookWork(QStringLiteral("mobi"), m_dir.filePath(QStringLiteral("alt.mobi"))) };
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        m_library->setDeleteFilesPermanently(true); // not into the recycle bin of whoever runs the tests
        m_settings = std::make_unique<ReaderSettings>(nullptr);
        m_reader = std::make_unique<ReaderController>(*m_library, *m_settings, nullptr);
    }

    void cleanup()
    {
        m_reader.reset();
        m_settings.reset();
        m_library.reset();
        m_backend.reset();
    }

    void settings()
    {
        QCOMPARE(m_settings->theme(), QStringLiteral("original"));
        QCOMPARE(m_settings->pageColor(), QStringLiteral("nacht"));
        QCOMPARE(m_settings->themes().size(), 6);
        const QVariantMap kraeftig = m_settings->themeInfo(QStringLiteral("kraeftig"));
        QCOMPARE(kraeftig.value(QStringLiteral("pageColor")).toString(), QStringLiteral("weiss"));
        QVERIFY(kraeftig.value(QStringLiteral("bold")).toBool());
        QVERIFY(m_settings->themeInfo(QStringLiteral("nope")).isEmpty());
        QSignalSpy changed(m_settings.get(), &ReaderSettings::changed);
        m_settings->applyTheme(QStringLiteral("papier"));
        QCOMPARE(m_settings->theme(), QStringLiteral("papier"));
        QCOMPARE(m_settings->pageColor(), QStringLiteral("sepia"));
        QVERIFY(changed.count() > 0);
        m_settings->setLineHeight(2.0);
        QCOMPARE(m_settings->theme(), QString()); // changed by hand
        m_settings->setLineHeight(9);
        QCOMPARE(m_settings->lineHeight(), 2.2);  // clamped
        m_settings->larger();
        QCOMPARE(m_settings->fontSizeStep(), 3);
        m_settings->setFontSizeStep(99);
        QCOMPARE(m_settings->fontSizeStep(), m_settings->fontSizeSteps() - 1);
        m_settings->setTwoPages(true);

        // Kept between starts.
        m_reader.reset();
        m_settings.reset();
        ReaderSettings again(nullptr);
        QCOMPARE(again.lineHeight(), 2.2);
        QCOMPARE(again.pageColor(), QStringLiteral("sepia"));
        QCOMPARE(again.theme(), QString());
        QVERIFY(again.twoPages());
        QCOMPARE(again.fontSizeStep(), again.fontSizeSteps() - 1);
        again.reset();
        QCOMPARE(again.theme(), QStringLiteral("original"));
        QCOMPARE(again.fontSizeStep(), 2);
    }

    void readableFormats()
    {
        QVERIFY(m_reader->canRead(QStringLiteral("epub")));
        QVERIFY(m_reader->canRead(QStringLiteral("pdf")));
        QVERIFY(!m_reader->canRead(QStringLiteral("mobi"))); // opens in the default application
        QVERIFY(!m_reader->canRead(QStringLiteral("unknown")));
        QVERIFY(!m_reader->open(QStringLiteral("mobi")));
        QVERIFY(!m_reader->isOpen());
    }

    void readEpub()
    {
        QSignalSpy opened(m_reader.get(), &ReaderController::openChanged);
        QVERIFY(m_reader->read(QStringLiteral("epub")));
        QVERIFY(m_reader->isOpen());
        QCOMPARE(opened.count(), 1);
        QCOMPARE(m_reader->type(), ReaderController::Text);
        QCOMPARE(m_reader->error(), ReaderController::NoError);
        QCOMPARE(m_reader->title(), QStringLiteral("epub"));
        QCOMPARE(m_reader->contents().size(), 4);
        QVERIFY(m_backend->format(QStringLiteral("epub")).lastOpened.isValid()); // counts as opened

        m_reader->setViewSize(QSizeF(600, 800));
        QCOMPARE(m_reader->pageItemSize(), QSizeF(600, 800));
        QVERIFY(m_reader->atStart());
        QTRY_VERIFY(m_reader->pageCount() > 3); // counted in the background
        QCOMPARE(m_reader->page(), 0);
        QCOMPARE(m_reader->contentsIndex(), 0);
        QCOMPARE(m_reader->chapterTitle(), QStringLiteral("Titel"));
        QCOMPARE(m_reader->contents().at(1).toMap().value(QStringLiteral("page")).toInt(), 1);

        m_reader->nextPage(); // the first chapter has one page
        QCOMPARE(m_reader->page(), 1);
        QCOMPARE(m_reader->chapterTitle(), QStringLiteral("Kapitel Eins"));
        m_reader->nextPage();
        QCOMPARE(m_reader->page(), 2);
        m_reader->previousPage();
        m_reader->previousPage();
        QVERIFY(m_reader->atStart());
        m_reader->previousPage(); // nothing before the start
        QCOMPARE(m_reader->page(), 0);

        // Contents and links.
        m_reader->goToContents(3);
        QCOMPARE(m_reader->chapterTitle(), QStringLiteral("Ziel"));
        m_reader->goToStart();
        m_reader->followLink(QStringLiteral("c2.xhtml#ziel"));
        QCOMPARE(m_reader->chapterTitle(), QStringLiteral("Ziel"));
        m_reader->goToStart();
        bool foundLink = false;
        for (int y = 0; y < 800 && !foundLink; y += 4) {
            for (int x = 0; x < 600 && !foundLink; x += 8)
                foundLink = m_reader->linkAt(x, y) == QLatin1String("c2.xhtml#ziel");
        }
        QVERIFY(foundLink);

        // Seeking and the progress.
        m_reader->seek(0.5);
        QVERIFY(m_reader->progress() > 0.2 && m_reader->progress() < 0.8);
        QVERIFY(!m_reader->atEnd());
        m_reader->goToEnd();
        QVERIFY(m_reader->atEnd());
        QCOMPARE(m_reader->progress(), 1.0);
        QCOMPARE(m_reader->page(), m_reader->pageCount() - 1);

        // Painting draws text in the text color on the page color.
        m_reader->setPageColor(Qt::white);
        m_reader->setTextColor(Qt::black);
        m_reader->goToStart();
        m_reader->nextPage();
        QImage image(600, 800, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QPainter painter(&image);
        m_reader->paint(&painter, QSizeF(600, 800), 1.0);
        painter.end();
        QCOMPARE(image.pixelColor(2, 2), QColor(Qt::white));
        bool text = false;
        for (int y = 0; y < 800 && !text; y += 2) {
            for (int x = 0; x < 600 && !text; x += 2)
                text = image.pixelColor(x, y).lightness() < 128;
        }
        QVERIFY(text);
    }

    // Closing stores the place; opening again goes back there, also with
    // another window size and other text settings.
    void storesPosition()
    {
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(600, 800));
        m_reader->goToContents(1);
        m_reader->nextPage();
        m_reader->nextPage();
        const double progress = m_reader->progress();
        const QString chapter = m_reader->chapterTitle();
        m_reader->close();
        QVERIFY(!m_reader->isOpen());

        const Format &stored = m_backend->format(QStringLiteral("epub"));
        const ReadingPosition position = ReadingPosition::fromString(stored.readingPosition);
        QCOMPARE(position.chapter, 1);
        QVERIFY(position.offset > 0);
        QCOMPARE(position.label, chapter);
        QCOMPARE(stored.progress, progress);
        QVERIFY(stored.progress > 0 && stored.progress < 1);

        // "Restart" with another window and a larger font.
        m_reader.reset();
        m_library.reset();
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        m_reader = std::make_unique<ReaderController>(*m_library, *m_settings, nullptr);
        m_settings->larger();
        m_settings->larger();
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(500, 700));
        QCOMPARE(m_reader->chapterTitle(), chapter);
        QVERIFY(qAbs(m_reader->progress() - progress) < 0.05);

        // Text settings while reading: the place stays.
        const double before = m_reader->progress();
        m_settings->applyTheme(QStringLiteral("fokus"));
        QVERIFY(qAbs(m_reader->progress() - before) < 0.03);
        m_reader->setViewSize(QSizeF(900, 1000));
        QVERIFY(qAbs(m_reader->progress() - before) < 0.03);
    }

    // The last page marks the book as finished; a finished book opens at its start.
    void finished()
    {
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(600, 800));
        m_reader->goToEnd();
        m_reader->close();
        QVERIFY(m_backend->format(QStringLiteral("epub")).finished());
        QCOMPARE(m_library->works()->workAt(0)->state(), Work::State::Finished);

        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(600, 800));
        QVERIFY(m_reader->atStart());
        // Only looking at it does not undo "finished".
        m_reader->close();
        QVERIFY(m_backend->format(QStringLiteral("epub")).finished());
        // Reading again does.
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(600, 800));
        m_reader->nextPage();
        m_reader->close();
        QVERIFY(!m_backend->format(QStringLiteral("epub")).finished());
    }

    void twoPages()
    {
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        m_reader->setViewSize(QSizeF(700, 800));
        m_reader->setTwoPagesWanted(true);
        QVERIFY(!m_reader->twoPages()); // too narrow for two pages
        m_reader->setViewSize(QSizeF(1400, 800));
        QVERIFY(m_reader->twoPages());
        // Each page: half the width, at most the longest line plus the margins.
        QCOMPARE(m_reader->pageItemSize().width(), 2 * std::min(700.0, 680.0 + 2 * m_settings->margin()));
        m_reader->goToContents(1);
        QTRY_VERIFY(m_reader->pageCount() > 0);
        const int first = m_reader->page();
        QCOMPARE(m_reader->lastPage(), first + 1);
        m_reader->nextPage();
        QCOMPARE(m_reader->page(), first + 2); // turns two pages
    }

    void pdf()
    {
        QVERIFY(m_reader->read(QStringLiteral("pdf")));
        QCOMPARE(m_reader->type(), ReaderController::Pdf);
        QCOMPARE(m_reader->pageCount(), 5);
        m_reader->setViewSize(QSizeF(800, 600));
        const QSizeF fit = m_reader->pageItemSize();
        QVERIFY(fit.height() <= 600.5 && fit.width() <= 800.5);
        m_reader->fitWidth();
        QCOMPARE(m_reader->zoomMode(), ReaderController::FitWidth);
        QVERIFY(qAbs(m_reader->pageItemSize().width() - 800) < 1);
        m_reader->zoomIn();
        QCOMPARE(m_reader->zoomMode(), ReaderController::Custom);
        QVERIFY(m_reader->pageItemSize().width() > 800);
        m_reader->fitPage();
        QCOMPARE(m_reader->pageItemSize(), fit);

        m_reader->nextPage();
        QCOMPARE(m_reader->page(), 1);
        QCOMPARE(m_reader->progress(), 0.4);
        m_reader->seek(1.0);
        QVERIFY(m_reader->atEnd());

        QImage image(fit.toSize(), QImage::Format_ARGB32);
        image.fill(Qt::black);
        QPainter painter(&image);
        m_reader->paint(&painter, fit, 1.0);
        painter.end();
        QVERIFY(image.pixelColor(image.width() / 2, image.height() / 2) != QColor(Qt::black));

        m_reader->goToStart();
        m_reader->nextPage();
        m_reader->nextPage();
        m_reader->close();
        QCOMPARE(ReadingPosition::fromString(m_backend->format(QStringLiteral("pdf")).readingPosition).page, 2);
        QVERIFY(m_reader->open(QStringLiteral("pdf")));
        QCOMPARE(m_reader->page(), 2);
    }

    void errors()
    {
        QVERIFY(m_reader->open(QStringLiteral("fehlt")));
        QCOMPARE(m_reader->error(), ReaderController::FileMissing);
        QCOMPARE(m_library->works()->workAt(m_library->works()->rowOf(QStringLiteral("fehlt")))->state(),
                 Work::State::FileMissing);
        m_reader->nextPage(); // nothing happens
        m_reader->close();
        QVERIFY(m_backend->format(QStringLiteral("fehlt")).readingPosition.isEmpty());

        QVERIFY(m_reader->open(QStringLiteral("kaputt")));
        QCOMPARE(m_reader->error(), ReaderController::Unreadable);
        QVERIFY(m_reader->contents().isEmpty());
    }

    void letsGoOfRemovedWork()
    {
        QVERIFY(m_reader->open(QStringLiteral("epub")));
        QCOMPARE(m_library->removeWorks({ QStringLiteral("epub") }), 1);
        QVERIFY(!m_reader->isOpen());
    }

    // Phase 12: a combined work of two volumes opens at the current volume
    // and its place; pages turn across the border in both directions; the
    // contents list the volumes.
    void volumes()
    {
        const QString second = m_dir.filePath(QStringLiteral("band2.epub"));
        // Own files: an earlier test removes buch.epub with its work.
        const QString first = m_dir.filePath(QStringLiteral("band1.epub"));
        writeEpub(first);
        writeEpub(second);
        Work one = ebookWork(QStringLiteral("v1"), first);
        one.title = QStringLiteral("Band Eins");
        one.seriesIndex = 1;
        one.formats[0].progress = 1.0;
        one.formats[0].lastOpened = QDateTime(QDate(2026, 11, 1), QTime(20, 0));
        Work two = ebookWork(QStringLiteral("v2"), second);
        two.title = QStringLiteral("Band Zwei");
        two.seriesIndex = 2;
        two.formats[0].readingPosition = ReadingPosition::inChapter(1, 0, QStringLiteral("II")).toString();
        two.formats[0].progress = 0.4;
        two.formats[0].lastOpened = QDateTime(QDate(2026, 11, 2), QTime(20, 0));
        m_backend->works = { mergeWorks({ two, one }) };
        m_reader.reset();
        m_library.reset();
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        m_reader = std::make_unique<ReaderController>(*m_library, *m_settings, nullptr);
        const QString id = QStringLiteral("v2");

        QCOMPARE(m_reader->readableFormat(id), 1); // the second volume
        QVERIFY(m_reader->canReadFormat(id, 0));
        QVERIFY(m_reader->open(id));
        m_reader->setViewSize(QSizeF(600, 800));
        QCOMPARE(m_reader->volumeCount(), 2);
        QCOMPARE(m_reader->volumeNumber(), 2);
        QCOMPARE(m_reader->title(), QStringLiteral("Band Zwei"));
        QCOMPARE(m_reader->chapterTitle(), QStringLiteral("Kapitel Eins")); // where the reader stopped
        QVERIFY(!m_reader->atStart());

        // Contents: both volumes, the chapters of the open one below it.
        const QVariantList contents = m_reader->contents();
        QCOMPARE(contents.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Band Eins"));
        QCOMPARE(contents.at(1).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Band Zwei"));
        QCOMPARE(contents.at(2).toMap().value(QStringLiteral("level")).toInt(), 1);
        QVERIFY(m_reader->contentsIndex() > 1);

        // Back from the first page of volume 2: the end of volume 1.
        m_reader->goToStart();
        QVERIFY(!m_reader->atStart()); // volume 1 is before it
        m_reader->previousPage();
        QCOMPARE(m_reader->volumeNumber(), 1);
        QCOMPARE(m_reader->progress(), 1.0);
        QVERIFY(!m_reader->atEnd()); // volume 2 follows
        // On again: volume 2 at its start.
        m_reader->nextPage();
        QCOMPARE(m_reader->volumeNumber(), 2);
        QCOMPARE(m_reader->progress() < 0.1, true);
        QVERIFY(m_reader->isOpen());

        // A volume from the contents.
        m_reader->goToContents(0);
        QCOMPARE(m_reader->volumeNumber(), 1);
        m_reader->close();
        QVERIFY(m_backend->works.first().formats.at(0).finished()); // looking did not undo it
    }

private:
    QTemporaryDir m_dir;
    QString m_epub;
    QString m_pdf;
    std::unique_ptr<MemoryBackend> m_backend;
    std::unique_ptr<LibraryController> m_library;
    std::unique_ptr<ReaderSettings> m_settings;
    std::unique_ptr<ReaderController> m_reader;
};

QTEST_MAIN(TestReaderController)
#include "tst_readercontroller.moc"
