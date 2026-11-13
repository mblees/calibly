// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "playercontroller.h"

namespace {

// MP3 of silence (MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono), no tags.
bool writeMp3(const QString &path, int durationMs)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QByteArray frame(417, '\0');
    frame[0] = char(0xFF);
    frame[1] = char(0xFB);
    frame[2] = char(0x90);
    frame[3] = char(0xC4);
    const int frames = int(durationMs / (1152.0 / 44.1));
    for (int i = 0; i < frames; ++i)
        file.write(frame);
    return true;
}

// Keeps the works in memory, like the database would.
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
            if (existing.id == work.id)
                existing = work;
        }
        return true;
    }
};

} // namespace

class TestPlayer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Own settings, so the tests neither see nor change the app's last audiobook.
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_player"));
        QVERIFY(m_dir.isValid());
        // An audiobook of two files with three chapters (two in the first file).
        QVERIFY(writeMp3(m_dir.filePath(QStringLiteral("01.mp3")), 2000));
        QVERIFY(writeMp3(m_dir.filePath(QStringLiteral("02.mp3")), 2000));
    }

    void init()
    {
        QSettings().clear();
        m_backend = std::make_unique<MemoryBackend>();
        m_backend->root = m_dir.path();
        Work work;
        work.id = QStringLiteral("marsianer");
        work.title = QStringLiteral("Der Marsianer");
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = m_dir.filePath(QStringLiteral("01.mp3"));
        audiobook.durationSeconds = 4;
        audiobook.chapters = { { QStringLiteral("Prolog"), QString(), 0, 500 },
                               { QStringLiteral("Sol 6"), QString(), 500, 1500 },
                               { QStringLiteral("Sol 7"), m_dir.filePath(QStringLiteral("02.mp3")), 0, 2000 } };
        work.formats = { audiobook };
        Work missing;
        missing.id = QStringLiteral("missing");
        missing.title = QStringLiteral("Fehlt");
        Format gone;
        gone.kind = Format::Kind::Audiobook;
        gone.filePath = m_dir.filePath(QStringLiteral("does-not-exist.m4b"));
        missing.formats = { gone };
        m_backend->works = { work, missing };
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
    }

    void cleanup()
    {
        m_library.reset();
        m_backend.reset();
    }

    void missingFile()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(!player.play(QStringLiteral("missing")));
        QCOMPARE(player.error(), PlayerController::FileMissing);
        QVERIFY(!player.hasWork());
        QVERIFY(!player.play(QStringLiteral("unknown")));
        // The gallery now shows the cover as "file missing".
        QCOMPARE(m_library->works()->workAt(1)->state(), Work::State::FileMissing);
        QCOMPARE(m_library->works()->workAt(0)->state(), Work::State::New);
    }

    // Playing remembers when the audiobook was opened, for "Continue".
    void playMarksFormatOpened()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(!m_library->works()->workAt(0)->formats.first().lastOpened.isValid());
        QVERIFY(player.play(QStringLiteral("marsianer")));
        player.pause();
        const Work *work = m_library->works()->workAt(0);
        QVERIFY(work->formats.first().lastOpened.isValid());
        QCOMPARE(work->formats.first().lastOpened, work->lastOpened);
        QCOMPARE(m_backend->works.first().formats.first().lastOpened, work->lastOpened);
        QCOMPARE(work->state(), Work::State::Normal);
    }

    // "Mark as finished" stays when the player, which has the audiobook
    // loaded but does not play it, saves on quit; taking it back returns to
    // the listening position.
    void finishedMarkSurvivesPlayer()
    {
        const auto saved = [this]() -> const Format & { return m_backend->works.first().formats.first(); };
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(player.load(QStringLiteral("marsianer")));
            player.seek(1000);
            QCOMPARE(saved().progress, 0.25);
            QVERIFY(m_library->setFinished(QStringLiteral("marsianer"), QStringLiteral("audiobook"), true));
            QCOMPARE(m_library->works()->workAt(0)->state(), Work::State::Finished);
        }
        QCOMPARE(saved().progress, 1.0);
        QCOMPARE(saved().positionMs, 1000);

        // "Restart": the finished audiobook is loaded at its start, and
        // stays finished as long as it is not played.
        m_library.reset();
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(player.hasWork());
            QCOMPARE(player.position(), 0.0);
        }
        QCOMPARE(saved().progress, 1.0);

        QVERIFY(m_library->setFinished(QStringLiteral("marsianer"), QStringLiteral("audiobook"), false));
        QCOMPARE(saved().progress, 0.25);
        QCOMPARE(saved().positionMs, 1000);
        QCOMPARE(m_library->works()->workAt(0)->state(), Work::State::Normal);

        // A position at the very end has nothing to return to: back to the start.
        Work atEnd = *m_library->works()->workAt(0);
        atEnd.formats.first().positionMs = 4000;
        atEnd.formats.first().progress = 1.0;
        QVERIFY(m_library->saveWork(atEnd));
        QVERIFY(m_library->setFinished(QStringLiteral("marsianer"), QStringLiteral("audiobook"), false));
        QCOMPARE(saved().progress, 0.0);
        QCOMPARE(saved().positionMs, 0);

        QVERIFY(!m_library->setFinished(QStringLiteral("unknown"), QStringLiteral("audiobook"), true));
        QVERIFY(!m_library->setFinished(QStringLiteral("marsianer"), QStringLiteral("ebook"), true)); // has no ebook
    }

    void chaptersAndSeeking()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(player.load(QStringLiteral("marsianer")));
        QVERIFY(player.hasWork());
        QCOMPARE(player.duration(), 4000.0);
        QCOMPARE(player.chapters().size(), 3);
        QCOMPARE(player.chapterIndex(), 0);

        player.seek(2500); // in the second file
        QCOMPARE(player.position(), 2500.0);
        QCOMPARE(player.chapterIndex(), 2);
        QCOMPARE(player.chapterStart(), 2000.0);
        player.previousChapter(); // more than 3 s in? no: back to the previous chapter
        QCOMPARE(player.chapterIndex(), 1);
        player.nextChapter();
        QCOMPARE(player.position(), 2000.0);
        player.skip(-30);
        QCOMPARE(player.position(), 0.0);
        player.nextChapter();
        QCOMPARE(player.chapterIndex(), 1);
    }

    // The position is stored in the library and the last audiobook is loaded
    // again on the next start, at the same position.
    void resumesAfterRestart()
    {
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(player.load(QStringLiteral("marsianer")));
            player.seek(2600);
        }
        const Format &saved = m_backend->works.first().formats.first();
        QCOMPARE(saved.positionMs, 2600);
        QCOMPARE(saved.progress, 0.65);
        QCOMPARE(m_library->works()->workAt(0)->formats.first().positionMs, 2600); // the gallery shows it

        // "Restart": a new library and player on the same data.
        m_library.reset(); // only one library at a time
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(player.hasWork());
            QCOMPARE(player.workId(), QStringLiteral("marsianer"));
            QCOMPARE(player.position(), 2600.0);
            QVERIFY(!player.playing());

            player.close();
            QVERIFY(!player.hasWork());
        }
        PlayerController afterClose(*m_library, nullptr);
        QVERIFY(!afterClose.hasWork());
    }

    // Sample data and other libraries do not take over the last audiobook.
    void remembersPerLibrary()
    {
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(player.load(QStringLiteral("marsianer")));
        }
        m_library.reset();
        m_backend->root = m_dir.filePath(QStringLiteral("other"));
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        {
            PlayerController player(*m_library, nullptr);
            QVERIFY(!player.hasWork());
        }
        m_library.reset();
        m_backend->root.clear(); // like --mock
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        PlayerController player(*m_library, nullptr);
        QVERIFY(!player.hasWork());
        QVERIFY(player.load(QStringLiteral("marsianer")));
        QCOMPARE(QSettings().value(QStringLiteral("player/library")).toString(), m_dir.path());
    }

    void settings()
    {
        {
            PlayerController player(*m_library, nullptr);
            player.setSpeed(1.5);
            player.setSpeed(9);
            QCOMPARE(player.speed(), 3.0);
            player.setSpeed(1.25);
            player.setSleepTimer(15);
            QCOMPARE(player.sleepMinutesLeft(), 15);
            player.setSleepAtChapterEnd();
            QCOMPARE(player.sleepMinutesLeft(), 0);
            QVERIFY(player.sleepAtChapterEnd());
        }
        PlayerController player(*m_library, nullptr);
        QCOMPARE(player.speed(), 1.25); // kept between starts
    }

    // Real playback: runs only where audio can be played.
    void playsAcrossFiles()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(player.play(QStringLiteral("marsianer")));
        player.setSpeed(2.0);
        player.seek(1500);
        QTest::qWait(1500);
        if (player.error() != PlayerController::NoError || player.position() <= 1500)
            QSKIP("no audio playback available here");
        // The first file ends at 2 s, the second one continues.
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 2300, 5000);
        player.pause();
        QCOMPARE(player.chapterIndex(), 2);
        const qint64 saved = m_backend->works.first().formats.first().positionMs;
        QVERIFY2(saved > 2000, qPrintable(QStringLiteral("saved %1, position %2").arg(saved).arg(player.position())));
    }

    // Jumping into another file while playing continues there, not at its start.
    void seeksIntoOtherFileWhilePlaying()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(player.play(QStringLiteral("marsianer")));
        QTest::qWait(500);
        if (player.error() != PlayerController::NoError || player.position() <= 0)
            QSKIP("no audio playback available here");
        player.seek(3000);
        QTest::qWait(400);
        QVERIFY2(player.position() >= 3000 && player.position() < 3800, qPrintable(QString::number(player.position())));
        QTRY_VERIFY_WITH_TIMEOUT(player.position() > 3100, 2000); // it plays on
        player.pause();
    }

private:
    QTemporaryDir m_dir;
    std::unique_ptr<MemoryBackend> m_backend;
    std::unique_ptr<LibraryController> m_library;
};

QTEST_GUILESS_MAIN(TestPlayer)
#include "tst_player.moc"
