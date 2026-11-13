// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

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

    // "Kombinieren" (phase 12): three volumes become one entry that keeps
    // them as parts in volume order; the ebooks follow that order.
    void combinesVolumes()
    {
        const auto volume = [](const QString &id, const QString &title, int number, const QString &file) {
            Work work;
            work.id = id;
            work.title = title;
            work.authors = { QStringLiteral("J. R. R. Tolkien") };
            work.series = QStringLiteral("Der Herr der Ringe");
            work.seriesIndex = number;
            work.description = title + QStringLiteral(" …");
            work.added = at(QStringLiteral("2026-01-0%1T10:00:00").arg(number));
            work.formats = { ebook(file) };
            work.formats.first().pages = 100 * number;
            return work;
        };
        const Work two = volume(QStringLiteral("two"), QStringLiteral("Die zwei Türme"), 2, QStringLiteral("c:/lib/2.epub"));
        const Work one = volume(QStringLiteral("one"), QStringLiteral("Die Gefährten"), 1, QStringLiteral("c:/lib/1.epub"));
        const Work three = volume(QStringLiteral("three"), QStringLiteral("Die Rückkehr des Königs"), 3,
                                  QStringLiteral("c:/lib/3.epub"));

        Work combined = mergeWorks({ two, one, three });
        QCOMPARE(combined.id, QStringLiteral("two")); // the first with an ebook, as before
        QVERIFY(combined.isCombined());
        QCOMPARE(combined.parts.size(), 3);
        QCOMPARE(combined.parts.at(0).id, QStringLiteral("one"));
        QCOMPARE(combined.parts.at(1).title, QStringLiteral("Die zwei Türme"));
        QCOMPARE(combined.parts.at(2).files, QStringList{ QStringLiteral("c:/lib/3.epub") });
        QCOMPARE(combined.formats.at(0).filePath, QStringLiteral("c:/lib/1.epub"));
        QCOMPARE(combined.formats.at(2).filePath, QStringLiteral("c:/lib/3.epub"));
        QCOMPARE(combined.formatTitle(1), QStringLiteral("Die zwei Türme"));
        QVERIFY(combined.canSplit());

        // Progress by pages (100 + 200 + 300): volume 1 read, volume 2 half.
        combined.formats[0].progress = 1.0;
        combined.formats[1].progress = 0.5;
        QCOMPARE(combined.readProgress(), (100.0 + 100.0) / 600.0);
        QCOMPARE(combined.state(), Work::State::Normal); // not all volumes finished
        // Without pages: the average.
        combined.formats[2].pages = 0;
        QCOMPARE(combined.readProgress(), 1.5 / 3.0);

        // Combining it with one more passes on its parts (one level).
        Work four = volume(QStringLiteral("four"), QStringLiteral("Anhänge"), 4, QStringLiteral("c:/lib/4.epub"));
        const Work bigger = mergeWorks({ combined, four });
        QCOMPARE(bigger.parts.size(), 4);
        QCOMPARE(bigger.parts.at(3).id, QStringLiteral("four"));

        // All volumes finished: the work is finished.
        for (Format &format : combined.formats)
            format.progress = 1.0;
        QCOMPARE(combined.state(), Work::State::Finished);
    }

    // Several ebooks of a work that was not combined are the same book: the
    // furthest one counts, and one finished is enough (as before).
    void sameBookInSeveralFiles()
    {
        Work work;
        work.id = QStringLiteral("w");
        work.formats = { ebook(QStringLiteral("c:/lib/a.epub")), ebook(QStringLiteral("c:/lib/a.pdf")) };
        work.formats[0].progress = 1.0;
        QCOMPARE(work.readProgress(), 1.0);
        QCOMPARE(work.state(), Work::State::Finished);
    }

    // The volume to continue with.
    void currentFormat()
    {
        Work work;
        work.formats = { ebook(QStringLiteral("1.epub")), audiobook(QStringLiteral("a.m4b"), 60),
                         ebook(QStringLiteral("2.epub")), ebook(QStringLiteral("3.epub")) };
        // Nothing opened: the first.
        QCOMPARE(work.currentFormat(Format::Kind::Ebook), 0);
        QCOMPARE(work.currentFormat(Format::Kind::Audiobook), 1);
        // Volume 1 finished, nothing else opened: the first unfinished one.
        work.formats[0].progress = 1.0;
        QCOMPARE(work.currentFormat(Format::Kind::Ebook), 2);
        // Volume 3 opened last and not finished: volume 3.
        work.formats[3].lastOpened = at(QStringLiteral("2026-11-10T10:00:00"));
        work.formats[3].progress = 0.2;
        QCOMPARE(work.currentFormat(Format::Kind::Ebook), 3);
        // Volume 1 was read to the end last: the next unfinished after it.
        work.formats[0].lastOpened = at(QStringLiteral("2026-11-12T10:00:00"));
        QCOMPARE(work.currentFormat(Format::Kind::Ebook), 2);
        // All finished: the first.
        for (Format &format : work.formats)
            format.progress = 1.0;
        QCOMPARE(work.currentFormat(Format::Kind::Ebook), 0);
        QCOMPARE(Work().currentFormat(Format::Kind::Ebook), -1);
    }

    // "Kombination auflösen": the works come back with their ids, metadata
    // and formats; the joined audiobook is cut into the files of each part
    // and the listening position goes to the part it is in.
    void splitsCombinedWork()
    {
        Work a;
        a.id = QStringLiteral("a");
        a.title = QStringLiteral("Band A");
        a.seriesIndex = 1;
        a.description = QStringLiteral("Erster Band");
        a.formats = { ebook(QStringLiteral("c:/lib/a.epub")), audiobook(QStringLiteral("c:/lib/a.mp3"), 100) };
        a.formats[0].readingPosition = QStringLiteral("{\"chapter\":3}");
        a.formats[0].progress = 0.4;
        Work b;
        b.id = QStringLiteral("b");
        b.title = QStringLiteral("Band B");
        b.seriesIndex = 2;
        b.formats = { audiobook(QStringLiteral("c:/lib/b.mp3"), 200) };
        // b's audiobook has two chapters of its own.
        b.formats[0].chapters = { { QStringLiteral("B1"), QString(), 0, 120000 },
                                  { QStringLiteral("B2"), QString(), 120000, 80000 } };

        Work combined = mergeWorks({ a, b });
        QCOMPARE(combined.formats.size(), 2); // a's ebook, one joined audiobook
        Format &joined = combined.formats[1];
        QCOMPARE(joined.chapters.size(), 3);
        QCOMPARE(joined.chapters.at(0).title, QStringLiteral("Band A")); // a file without chapters: its part's title
        // Listened into the first chapter of b: 100 s of a + 30 s.
        joined.positionMs = 130000;
        joined.lastOpened = at(QStringLiteral("2026-11-11T20:00:00"));
        combined.title = QStringLiteral("Band A (bearbeitet)"); // changed after combining

        int created = 0;
        const QList<Work> works = splitWork(combined, [&created] { return QStringLiteral("new%1").arg(++created); });
        QCOMPARE(created, 0); // stored parts keep their ids
        QCOMPARE(works.size(), 2);
        const Work &backA = works.at(0);
        const Work &backB = works.at(1);
        QCOMPARE(backA.id, QStringLiteral("a"));
        QCOMPARE(backA.title, QStringLiteral("Band A (bearbeitet)")); // the work's id: keeps the changes
        QCOMPARE(backB.id, QStringLiteral("b"));
        QCOMPARE(backB.title, QStringLiteral("Band B"));
        QCOMPARE(backB.seriesIndex, 2);
        QVERIFY(backA.parts.isEmpty() || !backA.isCombined());

        QCOMPARE(backA.formats.size(), 2);
        QCOMPARE(backA.formats.at(0).readingPosition, QStringLiteral("{\"chapter\":3}"));
        const Format &audioA = backA.formats.at(1);
        QCOMPARE(audioA.filePath, QStringLiteral("c:/lib/a.mp3"));
        QVERIFY(audioA.chapters.isEmpty()); // as it was: a file without chapters
        QVERIFY(audioA.finished());         // listened past it
        QCOMPARE(audioA.durationSeconds, 100);

        QCOMPARE(backB.formats.size(), 1);
        const Format &audioB = backB.formats.first();
        QCOMPARE(audioB.filePath, QStringLiteral("c:/lib/b.mp3"));
        QCOMPARE(audioB.chapters.size(), 2);
        QCOMPARE(audioB.chapters.first().filePath, QString()); // its own file
        QCOMPARE(audioB.positionMs, qint64(30000));
        QCOMPARE(audioB.progress, 30.0 / 200.0);
        QCOMPARE(audioB.lastOpened, joined.lastOpened);
        QCOMPARE(backB.lastOpened, joined.lastOpened);

        // A work that is no combination stays as it is.
        QCOMPARE(splitWork(a, [] { return QString(); }).size(), 1 + 1); // two formats: split (merged earlier)
        Work single = b;
        QCOMPARE(splitWork(single, [] { return QString(); }).first().id, QStringLiteral("b"));
    }

    // Merged before phase 12 (no parts): one work per format, the first
    // keeps the id, the others get new ids and the name of their file.
    void splitsWorkWithoutParts()
    {
        Work work;
        work.id = QStringLiteral("old");
        work.title = QStringLiteral("Alt");
        work.authors = { QStringLiteral("Autorin") };
        work.formats = { ebook(QStringLiteral("c:/lib/Band 1.epub")), ebook(QStringLiteral("c:/lib/Band 2.epub")),
                         audiobook(QStringLiteral("c:/lib/hoerbuch.m4b"), 60) };
        int created = 0;
        const QList<Work> works = splitWork(work, [&created] { return QStringLiteral("new%1").arg(++created); });
        QCOMPARE(works.size(), 3);
        QCOMPARE(works.at(0).id, QStringLiteral("old"));
        QCOMPARE(works.at(0).title, QStringLiteral("Alt"));
        QCOMPARE(works.at(1).id, QStringLiteral("new1"));
        QCOMPARE(works.at(1).title, QStringLiteral("Band 2"));
        QCOMPARE(works.at(1).authors, work.authors);
        QCOMPARE(works.at(2).formats.first().kind, Format::Kind::Audiobook);
    }
};

QTEST_GUILESS_MAIN(TestWorkMerge)
#include "tst_workmerge.moc"
