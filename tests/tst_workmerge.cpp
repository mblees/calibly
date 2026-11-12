#include <QTest>

#include "audiotimeline.h"
#include "workmerge.h"

namespace {

QDateTime at(const QString &text)
{
    return QDateTime::fromString(text, Qt::ISODate);
}

Format ebook(const QString &path)
{
    Format format;
    format.kind = Format::Kind::Ebook;
    format.filePath = path;
    format.fileSize = 500;
    return format;
}

Format audiobook(const QString &path, int durationSeconds)
{
    Format format;
    format.kind = Format::Kind::Audiobook;
    format.filePath = path;
    format.durationSeconds = durationSeconds;
    format.fileSize = 1000;
    return format;
}

} // namespace

class TestWorkMerge : public QObject
{
    Q_OBJECT

private slots:
    // A standalone ebook and a standalone audiobook become one work; the
    // work with the ebook gives the metadata.
    void mergesEbookAndAudiobook()
    {
        Work audio;
        audio.id = QStringLiteral("audio");
        audio.title = QStringLiteral("Dracula (ungekürzt)");
        audio.narrators = { QStringLiteral("Maren Brandt") };
        audio.series = QStringLiteral("Klassiker des Grauens");
        audio.seriesIndex = 2;
        audio.tags = { QStringLiteral("Horror") };
        audio.rating = 3;
        audio.language = QStringLiteral("de");
        audio.added = at(QStringLiteral("2026-02-03T10:00:00"));
        audio.lastOpened = at(QStringLiteral("2026-11-09T22:40:00"));
        audio.formats = { audiobook(QStringLiteral("c:/lib/dracula.m4b"), 3600) };

        Work book;
        book.id = QStringLiteral("book");
        book.title = QStringLiteral("Dracula");
        book.authors = { QStringLiteral("Bram Stoker") };
        book.tags = { QStringLiteral("Klassiker"), QStringLiteral("horror") };
        book.rating = 4;
        book.publishedYear = 1897;
        book.added = at(QStringLiteral("2026-03-01T10:00:00"));
        book.formats = { ebook(QStringLiteral("c:/lib/dracula.epub")) };

        QCOMPARE(mergeTarget({ audio, book }), 1);
        const Work merged = mergeWorks({ audio, book });
        QCOMPARE(merged.id, QStringLiteral("book"));
        QCOMPARE(merged.title, QStringLiteral("Dracula"));
        QCOMPARE(merged.authors, QStringList{ QStringLiteral("Bram Stoker") });
        QCOMPARE(merged.narrators, QStringList{ QStringLiteral("Maren Brandt") });
        QCOMPARE(merged.series, QStringLiteral("Klassiker des Grauens"));
        QCOMPARE(merged.seriesIndex, 2);
        QCOMPARE(merged.tags, (QStringList{ QStringLiteral("Klassiker"), QStringLiteral("horror") })); // no "Horror" twice
        QCOMPARE(merged.rating, 4);
        QCOMPARE(merged.language, QStringLiteral("de"));
        QCOMPARE(merged.publishedYear, 1897);
        QCOMPARE(merged.added, audio.added);           // the earliest
        QCOMPARE(merged.lastOpened, audio.lastOpened); // the latest
        QCOMPARE(merged.formats.size(), 2);
        QVERIFY(merged.hasEbook());
        QVERIFY(merged.hasAudiobook());
        QCOMPARE(merged.formats.at(1).filePath, QStringLiteral("c:/lib/dracula.m4b"));

        // Without an ebook the first work is the target.
        Work second = audio;
        second.id = QStringLiteral("second");
        QCOMPARE(mergeWorks({ audio, second }).id, QStringLiteral("audio"));
        QCOMPARE(mergeTarget({}), -1);
        QVERIFY(mergeWorks({}).id.isEmpty());
    }

    // The audio files of one book, imported one by one, become one
    // audiobook: in the order of their names, one chapter per file (or the
    // chapters the file has), continuing where the last one was stopped.
    void joinsAudioParts()
    {
        Format last = audiobook(QStringLiteral("c:/lib/Teil 10.mp3"), 30);
        Format middle = audiobook(QStringLiteral("c:/lib/Teil 2.mp3"), 20);
        middle.chapters = { { QStringLiteral("A"), QString(), 0, 10000 }, { QStringLiteral("B"), QString(), 10000, 10000 } };
        middle.positionMs = 5000;
        middle.progress = 0.25;
        middle.lastOpened = at(QStringLiteral("2026-11-10T08:00:00"));
        Format first = audiobook(QStringLiteral("c:/lib/Teil 1.mp3"), 10);
        first.progress = 1.0;
        first.positionMs = 10000;
        first.coverPath = QStringLiteral("c:/lib/Teil 1.mp3.cover.jpg");
        first.coverIsSquare = true;
        first.publisher = QStringLiteral("Hörverlag");

        Work work;
        work.id = QStringLiteral("w");
        work.formats = { last, middle, ebook(QStringLiteral("c:/lib/book.epub")), first };

        QVERIFY(joinAudiobooks(work));
        QCOMPARE(work.formats.size(), 2);
        QCOMPARE(work.formats.at(1).kind, Format::Kind::Ebook);
        const Format &joined = work.formats.at(0);
        QCOMPARE(joined.kind, Format::Kind::Audiobook);
        QCOMPARE(joined.filePath, QStringLiteral("c:/lib/Teil 1.mp3")); // "Teil 2" before "Teil 10"
        QCOMPARE(joined.chapters.size(), 4);
        QVERIFY(joined.chapters.at(0).filePath.isEmpty());              // the format's own file
        QCOMPARE(joined.chapters.at(0).durationMs, 10000);
        QCOMPARE(joined.chapters.at(1).title, QStringLiteral("A"));
        QCOMPARE(joined.chapters.at(1).filePath, QStringLiteral("c:/lib/Teil 2.mp3"));
        QCOMPARE(joined.chapters.at(2).startMs, 10000);                 // still relative to its file
        QCOMPARE(joined.chapters.at(3).filePath, QStringLiteral("c:/lib/Teil 10.mp3"));
        QCOMPARE(joined.chapters.at(3).durationMs, 30000);
        QCOMPARE(joined.durationSeconds, 60);
        QCOMPARE(joined.fileSize, 3000);
        QCOMPARE(joined.coverPath, first.coverPath);
        QVERIFY(joined.coverIsSquare);
        QCOMPARE(joined.publisher, QStringLiteral("Hörverlag"));
        QCOMPARE(joined.lastOpened, middle.lastOpened);
        // Part 1 was finished, part 2 stopped after 5 s.
        QCOMPARE(joined.positionMs, 15000);
        QCOMPARE(joined.progress, 0.25);

        const AudioTimeline timeline(joined);
        QCOMPARE(timeline.parts().size(), 3);
        QCOMPARE(timeline.durationMs(), 60000);
        QCOMPARE(timeline.chapters().at(3).startMs, 30000);

        // Nothing left to join.
        QVERIFY(!joinAudiobooks(work));
        QCOMPARE(work.formats.size(), 2);
    }

    void joinedPartsKeepFinishedAndMissing()
    {
        Format one = audiobook(QStringLiteral("c:/lib/1.mp3"), 10);
        one.progress = 1.0;
        Format two = audiobook(QStringLiteral("c:/lib/2.mp3"), 10);
        two.progress = 1.0;
        two.fileMissing = true;
        Work work;
        work.formats = { one, two };
        QVERIFY(joinAudiobooks(work));
        QCOMPARE(work.formats.size(), 1);
        QVERIFY(work.formats.first().finished());
        QVERIFY(work.formats.first().fileMissing);

        // Never played: no position, not finished.
        Work fresh;
        fresh.formats = { audiobook(QStringLiteral("c:/lib/1.mp3"), 10), audiobook(QStringLiteral("c:/lib/2.mp3"), 10) };
        QVERIFY(joinAudiobooks(fresh));
        QCOMPARE(fresh.formats.first().positionMs, 0);
        QCOMPARE(fresh.formats.first().progress, 0.0);

        // mergeWorks joins the audiobooks of the works it merges.
        Work a;
        a.id = QStringLiteral("a");
        a.formats = { audiobook(QStringLiteral("c:/lib/CD 2/01.mp3"), 10) };
        Work b;
        b.id = QStringLiteral("b");
        b.formats = { audiobook(QStringLiteral("c:/lib/CD 1/01.mp3"), 10) };
        const Work merged = mergeWorks({ a, b });
        QCOMPARE(merged.id, QStringLiteral("a"));
        QCOMPARE(merged.formats.size(), 1);
        QCOMPARE(merged.formats.first().filePath, QStringLiteral("c:/lib/CD 1/01.mp3"));
        QCOMPARE(merged.formats.first().chapters.size(), 2);
    }
};

QTEST_GUILESS_MAIN(TestWorkMerge)
#include "tst_workmerge.moc"
