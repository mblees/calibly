#include <QTest>

#include "audiotimeline.h"

class TestAudioTimeline : public QObject
{
    Q_OBJECT

private slots:
    void singleFileWithoutChapters()
    {
        Format format;
        format.kind = Format::Kind::Audiobook;
        format.filePath = QStringLiteral("book.m4b");
        format.durationSeconds = 3600;
        const AudioTimeline timeline(format);
        QCOMPARE(timeline.durationMs(), 3600000);
        QCOMPARE(timeline.parts().size(), 1);
        QCOMPARE(timeline.chapters().size(), 1);
        QCOMPARE(timeline.chapterAt(1234567), 0);
        QVERIFY(timeline.chapters().first().title.isEmpty());

        QVERIFY(AudioTimeline(Format()).isEmpty());
        QCOMPARE(AudioTimeline(Format()).chapterAt(0), -1);
    }

    void chaptersOfOneFile()
    {
        Format format;
        format.filePath = QStringLiteral("book.m4b");
        format.chapters = { { QStringLiteral("Eins"), QString(), 0, 60000 },
                            { QStringLiteral("Zwei"), QString(), 60000, 30000 },
                            { QStringLiteral("Drei"), QString(), 90000, 10000 } };
        const AudioTimeline timeline(format);
        QCOMPARE(timeline.durationMs(), 100000);
        QCOMPARE(timeline.parts().size(), 1);
        QCOMPARE(timeline.parts().first().filePath, QStringLiteral("book.m4b"));
        QCOMPARE(timeline.chapterAt(0), 0);
        QCOMPARE(timeline.chapterAt(59999), 0);
        QCOMPARE(timeline.chapterAt(60000), 1);
        QCOMPARE(timeline.chapterAt(95000), 2);
        QCOMPARE(timeline.chapterAt(500000), 2); // clamped
        QCOMPARE(timeline.chapterAt(-5), 0);
    }

    void severalFiles()
    {
        // An MP3 folder: the first file is the format's file, the second one
        // holds two chapters itself.
        Format format;
        format.filePath = QStringLiteral("01.mp3");
        format.chapters = { { QStringLiteral("Teil 1"), QString(), 0, 20000 },
                            { QStringLiteral("Teil 2a"), QStringLiteral("02.mp3"), 0, 5000 },
                            { QStringLiteral("Teil 2b"), QStringLiteral("02.mp3"), 5000, 7000 },
                            { QStringLiteral("Teil 3"), QStringLiteral("03.mp3"), 0, 8000 } };
        const AudioTimeline timeline(format);
        QCOMPARE(timeline.parts().size(), 3);
        QCOMPARE(timeline.parts().at(0).filePath, QStringLiteral("01.mp3"));
        QCOMPARE(timeline.parts().at(1).startMs, 20000);
        QCOMPARE(timeline.parts().at(1).durationMs, 12000);
        QCOMPARE(timeline.parts().at(2).startMs, 32000);
        QCOMPARE(timeline.durationMs(), 40000);
        QCOMPARE(timeline.chapters().at(2).startMs, 25000);
        QCOMPARE(timeline.chapters().at(2).part, 1);
        QCOMPARE(timeline.partAt(31999), 1);
        QCOMPARE(timeline.partAt(32000), 2);
        QCOMPARE(timeline.chapterAt(26000), 2);
    }
};

QTEST_GUILESS_MAIN(TestAudioTimeline)
#include "tst_audiotimeline.moc"
