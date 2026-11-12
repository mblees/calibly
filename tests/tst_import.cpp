#include <QBuffer>
#include <QPainter>
#include <QPdfWriter>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <QSignalSpy>
#include <private/qzipwriter_p.h>

#include <tuple>

#include <chapterframe.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <textidentificationframe.h>

#include "filenameparser.h"
#include "importplanner.h"
#include "importworker.h"
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
                "<dc:identifier opf:scheme='uuid'>urn:uuid:0a5e7c2e-1d4b-4bb9-9c29-7c3c0f5c1a11</dc:identifier>"
                "<dc:identifier>urn:isbn:978-3-16-148410-0</dc:identifier>"
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

// Writes an MP3 of silence (MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono)
// with ID3v2 tags. chapters: (title, start ms, end ms) as ID3v2 CHAP frames.
void writeMp3(const QString &path, int durationMs, const QString &album, const QString &title, int track,
              const QList<std::tuple<QString, int, int>> &chapters = {})
{
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray frame(417, '\0');
        frame[0] = char(0xFF);
        frame[1] = char(0xFB);
        frame[2] = char(0x90);
        frame[3] = char(0xC4);
        const int frames = int(durationMs / (1152.0 / 44.1));
        for (int i = 0; i < frames; ++i)
            file.write(frame);
    }
    TagLib::MPEG::File mp3(reinterpret_cast<const wchar_t *>(path.utf16()));
    QVERIFY(mp3.isValid());
    TagLib::ID3v2::Tag *tag = mp3.ID3v2Tag(true);
    tag->setAlbum(TagLib::String(album.toStdWString()));
    tag->setArtist(TagLib::String("Andy Weir"));
    tag->setTitle(TagLib::String(title.toStdWString()));
    tag->setTrack(unsigned(track));
    unsigned index = 0;
    for (const auto &[chapterTitle, start, end] : chapters) {
        auto *titleFrame = new TagLib::ID3v2::TextIdentificationFrame("TIT2", TagLib::String::UTF8);
        titleFrame->setText(TagLib::String(chapterTitle.toStdWString()));
        const TagLib::ByteVector id(QByteArray("ch" + QByteArray::number(index++)).constData()); // no null bytes
        TagLib::ID3v2::FrameList embedded;
        embedded.append(titleFrame);
        tag->addFrame(new TagLib::ID3v2::ChapterFrame(id, unsigned(start), unsigned(end), 0xFFFFFFFF, 0xFFFFFFFF,
                                                      embedded));
    }
    QVERIFY(mp3.save(TagLib::MPEG::File::ID3v2));
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
        QCOMPARE(metadata.isbn, QStringLiteral("978-3-16-148410-0")); // not the UUID
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
        // Outside the red area the page has no background of its own: white.
        QCOMPARE(metadata.cover.pixelColor(metadata.cover.width() - 2, metadata.cover.height() - 2),
                 QColor(Qt::white));
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

    void readMp3Chapters()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("marsianer.mp3"));
        writeMp3(path, 10000, QStringLiteral("Der Marsianer"), QStringLiteral("Der Marsianer"), 1,
                 { { QStringLiteral("Sol 6"), 4000, 10000 }, { QStringLiteral("Prolog"), 0, 4000 } });
        const FileMetadata metadata = readMetadata(path);
        QVERIFY2(metadata.error.isEmpty(), qPrintable(metadata.error));
        QCOMPARE(metadata.kind, Format::Kind::Audiobook);
        QVERIFY2(qAbs(metadata.durationMs - 10000) < 300, qPrintable(QString::number(metadata.durationMs)));
        QCOMPARE(metadata.trackNumber, 1);
        QCOMPARE(metadata.chapters.size(), 2);
        QCOMPARE(metadata.chapters.at(0).title, QStringLiteral("Prolog")); // sorted by start
        QCOMPARE(metadata.chapters.at(0).durationMs, 4000);
        QCOMPARE(metadata.chapters.at(1).startMs, 4000);
        QCOMPARE(metadata.chapters.at(1).durationMs, metadata.durationMs - 4000);
    }

    // The files of an MP3 folder become one audiobook with one chapter per
    // file, in track order; importing them again finds them as duplicates.
    void importMp3Folder()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("in"));
        QVERIFY(QDir().mkpath(source));
        writeMp3(source + QStringLiteral("/Teil 10.mp3"), 3000, QStringLiteral("Der Marsianer"),
                 QStringLiteral("Das Ende"), 3);
        writeMp3(source + QStringLiteral("/Teil 2.mp3"), 2000, QStringLiteral("Der Marsianer"),
                 QStringLiteral("Die Mitte"), 2);
        writeMp3(source + QStringLiteral("/Teil 1.mp3"), 4000, QStringLiteral("Der Marsianer"), QString(), 1,
                 { { QStringLiteral("Prolog"), 0, 1000 }, { QStringLiteral("Sol 6"), 1000, 4000 } });
        const QString root = dir.filePath(QStringLiteral("library"));

        ImportWorker worker;
        QSignalSpy finished(&worker, &ImportWorker::finished);
        worker.run({ source }, {}, root);
        QCOMPARE(finished.count(), 1);
        const QList<Work> works = finished.first().at(0).value<QList<Work>>();
        QCOMPARE(works.size(), 1);
        QCOMPARE(finished.first().at(1).toInt(), 3);
        const Work &work = works.first();
        QCOMPARE(work.title, QStringLiteral("Der Marsianer"));
        QCOMPARE(work.formats.size(), 1);
        const Format &format = work.formats.first();
        QCOMPARE(QFileInfo(format.filePath).fileName(), QStringLiteral("Teil 1.mp3"));
        QCOMPARE(format.chapters.size(), 4);
        QCOMPARE(format.chapters.at(0).title, QStringLiteral("Prolog"));
        QVERIFY(format.chapters.at(0).filePath.isEmpty());   // the format's own file
        QCOMPARE(format.chapters.at(2).title, QStringLiteral("Die Mitte"));
        QCOMPARE(QFileInfo(format.chapters.at(2).filePath).fileName(), QStringLiteral("Teil 2.mp3"));
        QVERIFY(QFileInfo::exists(format.chapters.at(2).filePath)); // copied into the library
        QCOMPARE(format.chapters.at(3).title, QStringLiteral("Das Ende"));
        QVERIFY(qAbs(format.durationSeconds - 9) <= 1);

        ImportWorker again;
        QSignalSpy finishedAgain(&again, &ImportWorker::finished);
        again.run({ source }, works, root);
        QCOMPARE(finishedAgain.first().at(1).toInt(), 0);
        QCOMPARE(finishedAgain.first().at(2).toStringList().size(), 3);
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

    // Phase 7: the parts of an audiobook folder become one work, also
    // without album tags and across "CD 1" / "CD 2" subfolders.
    void groupAudioFoldersJoinsParts()
    {
        auto part = [](const QString &path, const QString &title, bool fromName, bool fromTrack) {
            FileMetadata metadata = file(path, Format::Kind::Audiobook, title, QString());
            metadata.authors.clear();
            metadata.titleFromFileName = fromName;
            metadata.titleFromTrack = fromTrack;
            return metadata;
        };
        QList<FileMetadata> files = {
            // No album: track titles and file names only.
            part(QStringLiteral("c:/in/Dracula - Bram Stoker/01.mp3"), QStringLiteral("Kapitel 1"), false, true),
            part(QStringLiteral("c:/in/Dracula - Bram Stoker/02.mp3"), QStringLiteral("02"), true, false),
            // Disc folders: the album tag of one part names the book.
            part(QStringLiteral("c:/in/Odyssee/CD 1/01.mp3"), QStringLiteral("Die Odyssee"), false, false),
            part(QStringLiteral("c:/in/Odyssee/CD 2/01.mp3"), QStringLiteral("01"), true, false),
            // Two different albums in one folder: left alone.
            part(QStringLiteral("c:/in/Mix/a.mp3"), QStringLiteral("Faust"), false, false),
            part(QStringLiteral("c:/in/Mix/b.mp3"), QStringLiteral("Woyzeck"), false, false),
            // M4B files are complete audiobooks: left alone.
            part(QStringLiteral("c:/in/Reihe/Band 1.m4b"), QStringLiteral("Band 1"), true, false),
            part(QStringLiteral("c:/in/Reihe/Band 2.m4b"), QStringLiteral("Band 2"), true, false),
        };
        files[0].authors = { QStringLiteral("Bram Stoker") };
        files[0].authorsFromFileName = true; // guessed, the folder knows better

        groupAudioFolders(files);
        QCOMPARE(files.at(0).title, QStringLiteral("Dracula"));
        QCOMPARE(files.at(1).title, QStringLiteral("Dracula"));
        QCOMPARE(files.at(1).authors, QStringList{ QStringLiteral("Bram Stoker") });
        QVERIFY(files.at(1).titleFromFileName);
        QVERIFY(!files.at(0).titleFromTrack);
        QCOMPARE(files.at(2).title, QStringLiteral("Die Odyssee"));
        QCOMPARE(files.at(3).title, QStringLiteral("Die Odyssee"));
        QVERIFY(!files.at(3).titleFromFileName);
        QCOMPARE(files.at(4).title, QStringLiteral("Faust"));
        QCOMPARE(files.at(5).title, QStringLiteral("Woyzeck"));
        QCOMPARE(files.at(6).title, QStringLiteral("Band 1"));
        QCOMPARE(files.at(7).title, QStringLiteral("Band 2"));

        const ImportPlan plan = planImport(files, {});
        QCOMPARE(plan.works.size(), 6); // Dracula, Odyssee, Faust, Woyzeck, Band 1, Band 2
        QVERIFY(!needsReview(files.at(1)));  // the folder named the author
    }

    void needsReviewOnlyForUnknownFiles()
    {
        FileMetadata scan = file(QStringLiteral("c:/in/scan_042.pdf"), Format::Kind::Ebook,
                                 QStringLiteral("scan 042"), QString());
        scan.authors.clear();
        scan.titleFromFileName = true;
        QVERIFY(needsReview(scan));
        scan.authors = { QStringLiteral("Someone") };
        QVERIFY(!needsReview(scan));
        scan.authors.clear();
        scan.titleFromFileName = false; // the file has its own title
        QVERIFY(!needsReview(scan));
    }

    // Files dropped onto a cover become formats of that work, whatever
    // their own title says; duplicates are still skipped.
    void planAddsToDropTarget()
    {
        Work dracula;
        dracula.id = QStringLiteral("dracula");
        dracula.title = QStringLiteral("Dracula");
        dracula.authors = { QStringLiteral("Bram Stoker") };
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.fileSize = 500;
        dracula.formats = { ebook };
        Work other;
        other.id = QStringLiteral("other");
        other.title = QStringLiteral("Hoerbuch");
        other.authors = { QStringLiteral("Niemand") };

        const QList<FileMetadata> files = {
            file(QStringLiteral("c:/in/Dracula_Hoerbuch/01.mp3"), Format::Kind::Audiobook,
                 QStringLiteral("Hoerbuch"), QStringLiteral("Niemand")),
            file(QStringLiteral("c:/in/dracula.epub"), Format::Kind::Ebook, QStringLiteral("Dracula"),
                 QStringLiteral("Bram Stoker"), 500),
        };
        const ImportPlan plan = planImport(files, { other, dracula }, QStringLiteral("dracula"));
        QCOMPARE(plan.works.size(), 1);
        QCOMPARE(plan.works.at(0).id, QStringLiteral("dracula"));
        QCOMPARE(plan.works.at(0).title, QStringLiteral("Dracula")); // the target keeps its metadata
        QCOMPARE(plan.formats.size(), 1);
        QCOMPARE(plan.formats.at(0).metadata.kind, Format::Kind::Audiobook);
        QCOMPARE(plan.duplicates, QStringList{ QStringLiteral("c:/in/dracula.epub") });
    }

    // The worker reports one queue item per work before copying, then each work.
    void workerReportsItems()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("in"));
        QVERIFY(QDir().mkpath(source));
        writeMp3(source + QStringLiteral("/1.mp3"), 1000, QStringLiteral("Der Marsianer"), QString(), 1);
        writeMp3(source + QStringLiteral("/2.mp3"), 1000, QStringLiteral("Der Marsianer"), QString(), 2);
        QFile scan(source + QStringLiteral("/scan_042.mobi"));
        QVERIFY(scan.open(QIODevice::WriteOnly));
        scan.write("x");
        scan.close();

        Work existing;
        existing.id = QStringLiteral("marsianer");
        existing.title = QStringLiteral("Der Marsianer");
        existing.authors = { QStringLiteral("Andy Weir") };

        ImportWorker worker;
        QSignalSpy planned(&worker, &ImportWorker::planned);
        QSignalSpy ready(&worker, &ImportWorker::workReady);
        worker.run({ source }, { existing }, dir.filePath(QStringLiteral("library")));

        QCOMPARE(planned.count(), 1);
        const QList<ImportItem> items = planned.first().at(0).value<QList<ImportItem>>();
        QCOMPARE(items.size(), 2);
        // Ebooks are planned first.
        QCOMPARE(items.at(0).title, QStringLiteral("scan 042"));
        QVERIFY(items.at(0).newWork);
        QVERIFY(items.at(0).needsReview);
        QCOMPARE(items.at(0).fileTypes, QStringList{ QStringLiteral("MOBI") });
        QCOMPARE(items.at(1).workId, QStringLiteral("marsianer"));
        QVERIFY(!items.at(1).newWork);
        QVERIFY(items.at(1).merged);
        QVERIFY(items.at(1).hasAudiobook);
        QCOMPARE(items.at(1).fileCount, 2);

        QCOMPARE(ready.count(), 2);
        const Work marsianer = ready.at(1).at(1).value<Work>();
        QCOMPARE(marsianer.id, QStringLiteral("marsianer"));
        QCOMPARE(marsianer.formats.size(), 1);
        QCOMPARE(marsianer.formats.first().chapters.size(), 2);
        QVERIFY(ready.at(1).at(2).toStringList().isEmpty());

        const PathSummary summary = ImportWorker::inspectPaths({ source });
        QCOMPARE(summary.name, QStringLiteral("in"));
        QCOMPARE(summary.audiobookCount, 2);
        QCOMPARE(summary.ebookCount, 1);
        QCOMPARE(summary.countBySuffix.value(QStringLiteral("MP3")), 2);
    }

    // Text and Markdown files are imported when picked or dropped
    // themselves, not when found in a folder (audiobook folders often hold
    // .txt files).
    void textFilesOnlyWhenPicked()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("in"));
        QVERIFY(QDir().mkpath(source));
        const auto write = [&source](const QString &name) {
            QFile f(source + u'/' + name);
            return f.open(QIODevice::WriteOnly) && f.write("# Titel\n\nText.\n") > 0;
        };
        QVERIFY(write(QStringLiteral("Notizen - Testautor.md")));
        QVERIFY(write(QStringLiteral("info.txt")));
        writeMp3(source + QStringLiteral("/1.mp3"), 1000, QStringLiteral("Der Marsianer"), QString(), 1);

        QCOMPARE(ImportWorker::collectFiles({ source }), QStringList{ source + QStringLiteral("/1.mp3") });
        QCOMPARE(ImportWorker::inspectPaths({ source }).ebookCount, 0);

        const QString picked = source + QStringLiteral("/Notizen - Testautor.md");
        QCOMPARE(ImportWorker::collectFiles({ picked }), QStringList{ picked });
        QCOMPARE(ImportWorker::inspectPaths({ picked }).ebookCount, 1);
        const FileMetadata metadata = readMetadata(picked);
        QCOMPARE(metadata.kind, Format::Kind::Ebook);
        QCOMPARE(metadata.title, QStringLiteral("Notizen"));
        QCOMPARE(metadata.authors, QStringList{ QStringLiteral("Testautor") });
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
        format.publisher = QStringLiteral("Hörverlag");
        format.isbn = QStringLiteral("978-3-16-148410-0");
        format.positionMs = 900000;
        format.lastOpened = QDateTime(QDate(2026, 11, 10), QTime(8, 14));
        format.chapters = { { QStringLiteral("Eins"), QString(), 0, 1000 },
                            { QStringLiteral("Zwei"), root + QStringLiteral("/Jacob Grimm/Grimms Märchen/02.mp3"), 0, 2000 } };
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
        QCOMPARE(loaded.formats.first().publisher, QStringLiteral("Hörverlag"));
        QCOMPARE(loaded.formats.first().isbn, QStringLiteral("978-3-16-148410-0"));
        QCOMPARE(loaded.formats.first().positionMs, 900000);
        QCOMPARE(loaded.formats.first().lastOpened, format.lastOpened);
        QVERIFY(loaded.formats.first().fileMissing); // the test never created the file
        QCOMPARE(loaded.formats.first().chapters.size(), 2);
        QCOMPARE(loaded.formats.first().chapters.at(0).filePath, QString());
        QCOMPARE(QDir::cleanPath(loaded.formats.first().chapters.at(1).filePath),
                 QDir::cleanPath(format.chapters.at(1).filePath));
        QCOMPARE(loaded.formats.first().chapters.at(1).durationMs, 2000);
    }

    // Phase 9: a removed work is gone after a restart, with its formats.
    void sqliteRemovesWork()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Faust");
        Format format;
        format.kind = Format::Kind::Ebook;
        format.filePath = root + QStringLiteral("/faust.epub");
        work.formats = { format };
        Work other = work;
        other.id = QStringLiteral("w2");
        {
            SqliteBackend backend(root);
            QVERIFY(backend.open());
            QVERIFY(backend.saveWork(work));
            QVERIFY(backend.saveWork(other));
            QVERIFY(backend.removeWork(QStringLiteral("w1")));
            QVERIFY(!backend.removeWork(QStringLiteral("w1"))); // already gone
            QVERIFY(!backend.removeWork(QString()));
        }
        SqliteBackend reopened(root);
        QVERIFY(reopened.open());
        const QList<Work> works = reopened.loadWorks();
        QCOMPARE(works.size(), 1);
        QCOMPARE(works.first().id, QStringLiteral("w2"));
        QCOMPARE(works.first().formats.size(), 1);

        // The formats of the removed work did not stay behind.
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("check"));
            db.setDatabaseName(root + QStringLiteral("/calibly.db"));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM formats")));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toInt(), 1);
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("check"));
    }

    // Only files inside the library folder belong to a work.
    void workFilesInLibraryOnly()
    {
        QTemporaryDir dir;
        const QString root = dir.filePath(QStringLiteral("library"));
        const QString inside = root + QStringLiteral("/Autor/Titel/book.epub");
        const QString outside = dir.filePath(QStringLiteral("elsewhere/book.m4b"));
        for (const QString &path : { inside, inside + QStringLiteral(".cover.jpg"), outside }) {
            QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly));
        }
        Work work;
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.filePath = inside;
        ebook.coverPath = inside + QStringLiteral(".cover.jpg");
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = outside;
        work.formats = { ebook, audiobook };

        QCOMPARE(LibraryFolder::workFiles(root, work).size(), 2);
        QVERIFY(LibraryFolder::workFiles(QString(), work).isEmpty());
        QVERIFY(LibraryFolder::removeWorkFiles(root, work, false).isEmpty());
        QVERIFY(!QFileInfo::exists(inside));
        QVERIFY(!QFileInfo::exists(root + QStringLiteral("/Autor")));
        QVERIFY(QFileInfo::exists(root));
        QVERIFY(QFileInfo::exists(outside));
    }

    // Loading checks whether the files are still there, also the parts of
    // an audiobook of several files.
    void sqliteFindsMissingFiles()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        const auto touch = [&root](const QString &name) {
            QFile f(root + u'/' + name);
            return f.open(QIODevice::WriteOnly) && f.write("x") == 1;
        };
        QVERIFY(touch(QStringLiteral("book.epub")));
        QVERIFY(touch(QStringLiteral("01.mp3")));

        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Der Marsianer");
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.filePath = root + QStringLiteral("/book.epub");
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = root + QStringLiteral("/01.mp3");
        audiobook.chapters = { { QStringLiteral("Eins"), QString(), 0, 1000 },
                               { QStringLiteral("Zwei"), root + QStringLiteral("/02.mp3"), 0, 1000 } };
        work.formats = { ebook, audiobook };

        SqliteBackend backend(root);
        QVERIFY(backend.open());
        QVERIFY(backend.saveWork(work));
        QList<Work> works = backend.loadWorks();
        QVERIFY(!works.first().formats.at(0).fileMissing);
        QVERIFY(works.first().formats.at(1).fileMissing); // 02.mp3 is not there
        QCOMPARE(works.first().state(), Work::State::FileMissing);

        QVERIFY(touch(QStringLiteral("02.mp3")));
        works = backend.loadWorks();
        QVERIFY(!works.first().formats.at(1).fileMissing);
        QCOMPARE(works.first().state(), Work::State::New);
    }

    // A library of phase 7 (schema 3) gets "last opened" per format: the
    // format with progress takes it over from the work.
    void sqliteMigratesVersion3()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Moby Dick");
        work.lastOpened = QDateTime(QDate(2026, 11, 11), QTime(8, 14));
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.filePath = root + QStringLiteral("/moby.epub");
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = root + QStringLiteral("/moby.m4b");
        audiobook.progress = 0.5;
        work.formats = { ebook, audiobook };
        {
            SqliteBackend backend(root);
            QVERIFY(backend.open());
            QVERIFY(backend.saveWork(work));
        }
        {
            // Back to how phase 7 left the database.
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("v3"));
            db.setDatabaseName(root + QStringLiteral("/calibly.db"));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY2(query.exec(QStringLiteral("ALTER TABLE formats DROP COLUMN last_opened")),
                     qPrintable(query.lastError().text()));
            QVERIFY2(query.exec(QStringLiteral("ALTER TABLE formats DROP COLUMN reading_position")),
                     qPrintable(query.lastError().text()));
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 3")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("v3"));

        SqliteBackend backend(root);
        QString error;
        QVERIFY2(backend.open(&error), qPrintable(error));
        const QList<Work> works = backend.loadWorks();
        QCOMPARE(works.size(), 1);
        QVERIFY(!works.first().formats.at(0).lastOpened.isValid());
        QCOMPARE(works.first().formats.at(1).lastOpened, work.lastOpened);
        QVERIFY(backend.saveWork(works.first()));
        QCOMPARE(backend.loadWorks().first().formats.at(1).lastOpened, work.lastOpened);
    }

    // A library of phase 8 / 9 (schema 4) gets the reading position of the
    // reader; everything else stays.
    void sqliteMigratesVersion4()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Die Verwandlung");
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.filePath = root + QStringLiteral("/verwandlung.epub");
        ebook.progress = 0.25;
        ebook.lastOpened = QDateTime(QDate(2026, 11, 10), QTime(21, 5));
        work.formats = { ebook };
        {
            SqliteBackend backend(root);
            QVERIFY(backend.open());
            QVERIFY(backend.saveWork(work));
        }
        {
            // Back to how phase 9 left the database.
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("v4"));
            db.setDatabaseName(root + QStringLiteral("/calibly.db"));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY2(query.exec(QStringLiteral("ALTER TABLE formats DROP COLUMN reading_position")),
                     qPrintable(query.lastError().text()));
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 4")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("v4"));

        SqliteBackend backend(root);
        QString error;
        QVERIFY2(backend.open(&error), qPrintable(error));
        QList<Work> works = backend.loadWorks();
        QCOMPARE(works.size(), 1);
        QCOMPARE(works.first().formats.at(0).progress, 0.25);
        QCOMPARE(works.first().formats.at(0).lastOpened, ebook.lastOpened);
        QVERIFY(works.first().formats.at(0).readingPosition.isEmpty());

        // The new column is stored.
        works.first().formats[0].readingPosition = QStringLiteral("{\"chapter\":2,\"offset\":140}");
        QVERIFY(backend.saveWork(works.first()));
        QCOMPARE(backend.loadWorks().first().formats.at(0).readingPosition,
                 QStringLiteral("{\"chapter\":2,\"offset\":140}"));

        // The library was copied before the update, as it was.
        const QStringList backups = QDir(root).entryList({ QStringLiteral("calibly.db.v4-*.bak") }, QDir::Files);
        QCOMPARE(backups.size(), 1);
        QSqlDatabase old = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("backup"));
        old.setDatabaseName(QDir(root).filePath(backups.first()));
        QVERIFY(old.open());
        {
            QSqlQuery query(old);
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version")) && query.next());
            QCOMPARE(query.value(0).toInt(), 4);
        }
        old.close();
        old = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("backup"));
    }

    // Only the newest three backups are kept.
    void sqliteKeepsThreeBackups()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        {
            SqliteBackend backend(root);
            QVERIFY(backend.open());
        }
        for (int i = 0; i < 4; ++i) {
            const QString old = QStringLiteral("calibly.db.v3-2026010%1-120000.bak").arg(i + 1);
            QVERIFY(QFile::copy(SqliteBackend::databasePath(root), QDir(root).filePath(old)));
            QFile file(QDir(root).filePath(old));
            QVERIFY(file.open(QIODevice::ReadWrite));
            file.setFileTime(QDateTime(QDate(2026, 1, i + 1), QTime(12, 0)), QFileDevice::FileModificationTime);
        }
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("v4b"));
            db.setDatabaseName(SqliteBackend::databasePath(root));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(QStringLiteral("ALTER TABLE formats DROP COLUMN reading_position")));
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 4")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("v4b"));
        SqliteBackend backend(root);
        QVERIFY(backend.open());
        const QStringList backups = QDir(root).entryList({ QStringLiteral("calibly.db.v*.bak") }, QDir::Files, QDir::Time);
        QCOMPARE(backups.size(), 3);
        QVERIFY(backups.first().startsWith(QLatin1String("calibly.db.v4-"))); // the new one
        QVERIFY(backups.contains(QStringLiteral("calibly.db.v3-20260104-120000.bak")));
        QVERIFY(!backups.contains(QStringLiteral("calibly.db.v3-20260101-120000.bak")));
    }

    // A missing, damaged or newer library is reported and left as it is.
    void sqliteOpenErrors()
    {
        QTemporaryDir dir;

        // Missing: only created when asked for.
        const QString missing = dir.filePath(QStringLiteral("gone"));
        {
            SqliteBackend backend(missing);
            QVERIFY(!backend.open(nullptr, false));
            QCOMPARE(backend.openError(), SqliteBackend::OpenError::NotFound);
            QVERIFY(!QFileInfo::exists(missing));
        }

        // Damaged: not a database; the file stays unchanged.
        const QString damaged = dir.filePath(QStringLiteral("damaged"));
        QVERIFY(QDir().mkpath(damaged));
        const QByteArray garbage(4096, 'x');
        {
            QFile file(SqliteBackend::databasePath(damaged));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(garbage);
        }
        {
            SqliteBackend backend(damaged);
            QString error;
            QVERIFY(!backend.open(&error));
            QCOMPARE(backend.openError(), SqliteBackend::OpenError::Damaged);
            QVERIFY(!error.isEmpty());
        }
        QFile file(SqliteBackend::databasePath(damaged));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), garbage);
        file.close();

        // Newer: written by a later calibly.
        const QString newer = dir.filePath(QStringLiteral("newer"));
        {
            SqliteBackend backend(newer);
            QVERIFY(backend.open());
        }
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("newer"));
            db.setDatabaseName(SqliteBackend::databasePath(newer));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 99")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("newer"));
        SqliteBackend backend(newer);
        QVERIFY(!backend.open());
        QCOMPARE(backend.openError(), SqliteBackend::OpenError::NewerVersion);
    }

    // A library of phase 4 (schema 1, publisher per work) is updated on open.
    void sqliteMigratesVersion1()
    {
        QTemporaryDir dir;
        const QString root = dir.path();
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("v1"));
            db.setDatabaseName(root + QStringLiteral("/calibly.db"));
            QVERIFY(db.open());
            QSqlQuery query(db);
            const QStringList statements = {
                QStringLiteral("CREATE TABLE works (id TEXT PRIMARY KEY, title TEXT NOT NULL, authors TEXT,"
                               " narrators TEXT, series TEXT, series_index INTEGER, description TEXT, tags TEXT,"
                               " rating INTEGER, language TEXT, published_year INTEGER, publisher TEXT,"
                               " added TEXT, last_opened TEXT)"),
                QStringLiteral("CREATE TABLE formats (id INTEGER PRIMARY KEY AUTOINCREMENT,"
                               " work_id TEXT NOT NULL REFERENCES works(id) ON DELETE CASCADE, position INTEGER,"
                               " kind TEXT NOT NULL, file_path TEXT, progress REAL, cover_path TEXT,"
                               " cover_square INTEGER, file_size INTEGER, pages INTEGER, duration INTEGER)"),
                QStringLiteral("INSERT INTO works (id, title, publisher) VALUES ('w1', 'Faust', 'Reclam')"),
                QStringLiteral("INSERT INTO formats (work_id, position, kind, file_path) VALUES ('w1', 0, 'ebook', 'faust.epub')"),
                QStringLiteral("INSERT INTO formats (work_id, position, kind, file_path) VALUES ('w1', 1, 'audiobook', 'faust.m4b')"),
                QStringLiteral("PRAGMA user_version = 1"),
            };
            for (const QString &statement : statements)
                QVERIFY2(query.exec(statement), qPrintable(query.lastError().text()));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("v1"));

        SqliteBackend backend(root);
        QString error;
        QVERIFY2(backend.open(&error), qPrintable(error));
        const QList<Work> works = backend.loadWorks();
        QCOMPARE(works.size(), 1);
        QCOMPARE(works.first().title, QStringLiteral("Faust"));
        QCOMPARE(works.first().formats.size(), 2);
        QCOMPARE(works.first().formats.at(0).publisher, QStringLiteral("Reclam"));
        QCOMPARE(works.first().formats.at(1).publisher, QStringLiteral("Reclam"));
        QVERIFY(backend.saveWork(works.first()));

        // Opening again does not run the update a second time.
        SqliteBackend reopened(root);
        QVERIFY(reopened.open());
        QCOMPARE(reopened.loadWorks().first().formats.at(0).publisher, QStringLiteral("Reclam"));
    }
};

QTEST_GUILESS_MAIN(TestImport)
#include "tst_import.moc"
