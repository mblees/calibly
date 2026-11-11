#include <QBuffer>
#include <QPainter>
#include <QPdfWriter>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <private/qzipwriter_p.h>

#include "filenameparser.h"
#include "importplanner.h"
#include "libraryfolder.h"
#include "metadatareader.h"
#include "pdfreader.h"
#include "sqlitebackend.h"

namespace {

QByteArray pngCover(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::darkBlue);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

// Writes a minimal EPUB 2 file with calibre series metadata and a cover.
void writeEpub(const QString &path)
{
    QZipWriter zip(path);
    zip.addFile(QStringLiteral("mimetype"), "application/epub+zip");
    // Single-quoted XML attributes keep the C++ strings simple (moc does not
    // like raw string literals here).
    zip.addFile(QStringLiteral("META-INF/container.xml"),
                "<?xml version='1.0'?><container version='1.0' xmlns='urn:oasis:names:tc:opendocument:xmlns:container'>"
                "<rootfiles><rootfile full-path='OEBPS/content.opf' media-type='application/oebps-package+xml'/>"
                "</rootfiles></container>");
    zip.addFile(QStringLiteral("OEBPS/content.opf"),
                "<?xml version='1.0' encoding='utf-8'?>"
                "<package xmlns='http://www.idpf.org/2007/opf' version='2.0'>"
                "<metadata xmlns:dc='http://purl.org/dc/elements/1.1/' xmlns:opf='http://www.idpf.org/2007/opf'>"
                "<dc:title>MAR 02 - Der Marsianer</dc:title>"
                "<dc:creator opf:role='aut'>Andy Weir</dc:creator>"
                "<dc:creator opf:role='trl'>Jemand Anderes</dc:creator>"
                "<dc:language>de-DE</dc:language>"
                "<dc:publisher>Testverlag</dc:publisher>"
                "<dc:date>2014-10-01</dc:date>"
                "<dc:description>&lt;p&gt;Ein Astronaut &amp;amp; der Mars.&lt;/p&gt;</dc:description>"
                "<dc:subject>Science-Fiction</dc:subject>"
                "<meta name='calibre:series' content='Mars'/>"
                "<meta name='calibre:series_index' content='2.0'/>"
                "<meta name='calibre:rating' content='8.0'/>"
                "<meta name='cover' content='cover-image'/>"
                "</metadata>"
                "<manifest><item id='cover-image' href='images/cover%20art.png' media-type='image/png'/></manifest>"
                "</package>");
    zip.addFile(QStringLiteral("OEBPS/images/cover art.png"), pngCover(400, 600));
    zip.close();
}

FileMetadata file(const QString &path, Format::Kind kind, const QString &title, const QString &author,
                  qint64 size = 100)
{
    FileMetadata metadata;
    metadata.sourcePath = path;
    metadata.kind = kind;
    metadata.title = title;
    metadata.authors = { author };
    metadata.fileSize = size;
    return metadata;
}

} // namespace

class TestImport : public QObject
{
    Q_OBJECT

private slots:
    void parseFileName_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("code");
        QTest::addColumn<int>("index");
        QTest::addColumn<QString>("title");
        QTest::addColumn<QString>("author");
        QTest::newRow("series") << "DCC 01 - Dungeon Crawler Carl - Matt Dinniman" << "DCC" << 1
                                << "Dungeon Crawler Carl" << "Matt Dinniman";
        QTest::newRow("apostrophe") << "DCC 02 - Carl's Doomsday Scenario - Matt Dinniman" << "DCC" << 2
                                    << "Carl's Doomsday Scenario" << "Matt Dinniman";
        QTest::newRow("hyphen in title") << "EXP 05 - Nemesis-Spiele - James Corey" << "EXP" << 5
                                         << "Nemesis-Spiele" << "James Corey";
        QTest::newRow("title - author") << "Der Marsianer - Andy Weir" << "" << 0 << "Der Marsianer" << "Andy Weir";
        QTest::newRow("title only") << "Moby_Dick" << "" << 0 << "Moby Dick" << "";
    }

    void parseFileName()
    {
        QFETCH(QString, name);
        QFETCH(QString, code);
        QFETCH(int, index);
        QFETCH(QString, title);
        QFETCH(QString, author);
        const FileNameInfo info = ::parseFileName(name);
        QCOMPARE(info.seriesCode, code);
        QCOMPARE(info.seriesIndex, index);
        QCOMPARE(info.title, title);
        QCOMPARE(info.author, author);
    }

    void readEpub()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("ignored name - Someone.epub"));
        writeEpub(path);

        const FileMetadata metadata = readMetadata(path);
        QVERIFY2(metadata.error.isEmpty(), qPrintable(metadata.error));
        QCOMPARE(metadata.kind, Format::Kind::Ebook);
        QCOMPARE(metadata.title, QStringLiteral("Der Marsianer")); // "MAR 02 - " removed
        QCOMPARE(metadata.authors, QStringList{ QStringLiteral("Andy Weir") }); // translator left out
        QCOMPARE(metadata.language, QStringLiteral("de"));
        QCOMPARE(metadata.publisher, QStringLiteral("Testverlag"));
        QCOMPARE(metadata.publishedYear, 2014);
        QCOMPARE(metadata.description, QStringLiteral("Ein Astronaut & der Mars."));
        QCOMPARE(metadata.tags, QStringList{ QStringLiteral("Science-Fiction") });
        QCOMPARE(metadata.series, QStringLiteral("Mars"));
        QCOMPARE(metadata.seriesIndex, 2);
        QCOMPARE(metadata.rating, 4);
        QCOMPARE(metadata.cover.size(), QSize(400, 600));
    }

    void readPdf()
    {
        if (!pdfSupportAvailable())
            QSKIP("built without Qt PDF");
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("DAN 07 - Something - Someone.pdf"));
        {
            QPdfWriter writer(path);
            writer.setTitle(QStringLiteral("Dandadan 7"));
            QPainter painter(&writer);
            painter.fillRect(QRect(0, 0, 500, 700), Qt::red);
            writer.newPage();
            painter.fillRect(QRect(0, 0, 100, 100), Qt::blue); // no text: the test runs without fonts
        }
        const FileMetadata metadata = readMetadata(path);
        QVERIFY2(metadata.error.isEmpty(), qPrintable(metadata.error));
        QCOMPARE(metadata.title, QStringLiteral("Dandadan 7"));
        QCOMPARE(metadata.pages, 2);
        QVERIFY(!metadata.cover.isNull());
        QVERIFY(metadata.cover.height() > metadata.cover.width()); // portrait page
        QCOMPARE(metadata.seriesIndex, 7);                        // from the file name
    }

    void fileNameFallback()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("ERE 03 - Erebos 3 - Ursula Poznanski.mobi"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();

        const FileMetadata metadata = readMetadata(path);
        QCOMPARE(metadata.title, QStringLiteral("Erebos 3"));
        QCOMPARE(metadata.authors, QStringList{ QStringLiteral("Ursula Poznanski") });
        QCOMPARE(metadata.series, QStringLiteral("ERE"));
        QCOMPARE(metadata.seriesIndex, 3);
        QCOMPARE(metadata.fileSize, 1);
    }

    void planGroupsFormats()
    {
        const QList<FileMetadata> files = {
            file(QStringLiteral("c:/in/DCC 01 - Carl - Matt Dinniman.m4b"), Format::Kind::Audiobook,
                 QStringLiteral("Dungeon Crawler Carl: A LitRPG"), QStringLiteral("Matt Dinniman")),
            file(QStringLiteral("c:/in/DCC 01 - Carl - Matt Dinniman.epub"), Format::Kind::Ebook,
                 QStringLiteral("Dungeon Crawler Carl"), QStringLiteral("Matt Dinniman")),
            file(QStringLiteral("c:/in/Der Marsianer.epub"), Format::Kind::Ebook,
                 QStringLiteral("Der Marsianer"), QStringLiteral("Andy Weir")),
        };
        const ImportPlan plan = planImport(files, {});
        QCOMPARE(plan.works.size(), 2);
        QCOMPARE(plan.formats.size(), 3);
        // The ebook comes first and gives the title.
        QCOMPARE(plan.works.at(0).title, QStringLiteral("Dungeon Crawler Carl"));
        // Ebooks are planned first; the m4b (last) joins the ebook's work.
        QCOMPARE(plan.formats.at(2).metadata.kind, Format::Kind::Audiobook);
        QCOMPARE(plan.formats.at(2).workIndex, 0);
        QVERIFY(!plan.works.at(0).id.isEmpty());
        QVERIFY(plan.works.at(0).added.isValid());
    }

    void planAddsToExistingAndSkipsDuplicates()
    {
        Work existing;
        existing.id = QStringLiteral("marsianer");
        existing.title = QStringLiteral("Der Marsianer");
        existing.authors = { QStringLiteral("Andy Weir") };
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.fileSize = 500;
        existing.formats = { ebook };

        const QList<FileMetadata> files = {
            file(QStringLiteral("c:/in/marsianer.m4b"), Format::Kind::Audiobook, QStringLiteral("Der Marsianer"),
                 QStringLiteral("Andy Weir")),
            file(QStringLiteral("c:/in/marsianer.epub"), Format::Kind::Ebook, QStringLiteral("Der Marsianer"),
                 QStringLiteral("Andy Weir"), 500),
        };
        const ImportPlan plan = planImport(files, { existing });
        QCOMPARE(plan.works.size(), 1);
        QCOMPARE(plan.works.at(0).id, QStringLiteral("marsianer"));
        QCOMPARE(plan.formats.size(), 1);
        QCOMPARE(plan.formats.at(0).metadata.kind, Format::Kind::Audiobook);
        QCOMPARE(plan.duplicates, QStringList{ QStringLiteral("c:/in/marsianer.epub") });
    }

    void safeNames()
    {
        QCOMPARE(LibraryFolder::safeName(QStringLiteral("Carl's: Doom? <Scenario>.")),
                 QStringLiteral("Carl's_ Doom_ _Scenario_"));
        QCOMPARE(LibraryFolder::safeName(QString()), QStringLiteral("_"));
    }

    void copyIntoLibrary()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("book.epub"));
        QFile f(source);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("abc");
        f.close();

        const QString folder = dir.filePath(QStringLiteral("library/Andy Weir/Der Marsianer"));
        const QString first = LibraryFolder::copyInto(source, folder);
        QCOMPARE(QFileInfo(first).fileName(), QStringLiteral("book.epub"));
        // Same file again: reused, not copied twice.
        QCOMPARE(LibraryFolder::copyInto(source, folder), first);
        QCOMPARE(QDir(folder).entryList(QDir::Files).size(), 1);
    }

    void sqliteRoundTrip()
    {
        QTemporaryDir dir;
        const QString root = dir.filePath(QStringLiteral("library"));
        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Grimms Märchen");
        work.authors = { QStringLiteral("Jacob Grimm"), QStringLiteral("Wilhelm Grimm") };
        work.tags = { QStringLiteral("Märchen") };
        work.seriesIndex = 2;
        work.rating = 5;
        work.added = QDateTime(QDate(2026, 11, 1), QTime(10, 0));
        Format format;
        format.kind = Format::Kind::Audiobook;
        format.filePath = root + QStringLiteral("/Jacob Grimm/Grimms Märchen/grimm.m4b");
        format.coverPath = format.filePath + QStringLiteral(".cover.jpg");
        format.coverIsSquare = true;
        format.durationSeconds = 3600;
        format.progress = 0.25;
        work.formats = { format };

        {
            SqliteBackend backend(root);
            QString error;
            QVERIFY2(backend.open(&error), qPrintable(error));
            QVERIFY(backend.saveWork(work));
            work.title = QStringLiteral("Kinder- und Hausmärchen");
            QVERIFY(backend.saveWork(work)); // update, not a second row
        }
        QVERIFY(QFileInfo::exists(root + QStringLiteral("/calibly.db")));

        // A new backend on the same folder sees the same data ("after a restart").
        SqliteBackend reopened(root);
        QVERIFY(reopened.open());
        const QList<Work> works = reopened.loadWorks();
        QCOMPARE(works.size(), 1);
        const Work &loaded = works.first();
        QCOMPARE(loaded.title, QStringLiteral("Kinder- und Hausmärchen"));
        QCOMPARE(loaded.authors, work.authors);
        QCOMPARE(loaded.tags, work.tags);
        QCOMPARE(loaded.seriesIndex, 2);
        QCOMPARE(loaded.rating, 5);
        QCOMPARE(loaded.added, work.added);
        QCOMPARE(loaded.formats.size(), 1);
        QCOMPARE(QDir::cleanPath(loaded.formats.first().filePath), QDir::cleanPath(format.filePath));
        QCOMPARE(loaded.formats.first().coverIsSquare, true);
        QCOMPARE(loaded.formats.first().durationSeconds, 3600);
        QCOMPARE(loaded.formats.first().progress, 0.25);
    }
};

QTEST_GUILESS_MAIN(TestImport)
#include "tst_import.moc"
