// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "librarylock.h"
#include "continuemodel.h"
#include "playercontroller.h"

namespace {

// MP3 of silence (MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono), no tags.
bool writeMp3(const QString &path, int durationMs)
{
    QFile file(path);
    if (!QDir().mkpath(QFileInfo(path).absolutePath()) || !file.open(QIODevice::WriteOnly))
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

bool writeFile(const QString &path)
{
    QFile file(path);
    return QDir().mkpath(QFileInfo(path).absolutePath()) && file.open(QIODevice::WriteOnly) && file.write("x") == 1;
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
            if (existing.id == work.id) {
                existing = work;
                return true;
            }
        }
        works.append(work);
        return true;
    }
    bool removeWork(const QString &workId) override
    {
        return works.removeIf([&workId](const Work &work) { return work.id == workId; }) > 0;
    }
    bool has(const QString &workId) const
    {
        return std::any_of(works.cbegin(), works.cend(), [&workId](const Work &work) { return work.id == workId; });
    }
};

} // namespace

class TestLibraryController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Own settings, so the tests neither see nor change the app's last audiobook.
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_librarycontroller"));
    }

    // A library folder with an ebook, the audiobook of the same book (two
    // files) and another book.
    void init()
    {
        QSettings().clear();
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        m_root = m_dir->filePath(QStringLiteral("library"));
        m_epub = m_root + QStringLiteral("/Bram Stoker/Dracula/dracula.epub");
        m_part1 = m_root + QStringLiteral("/Bram Stoker/Dracula (Hörbuch)/01.mp3");
        m_part2 = m_root + QStringLiteral("/Bram Stoker/Dracula (Hörbuch)/02.mp3");
        m_faust = m_root + QStringLiteral("/Goethe/Faust/faust.epub");
        QVERIFY(writeFile(m_epub));
        QVERIFY(writeFile(m_epub + QStringLiteral(".cover.jpg")));
        QVERIFY(writeMp3(m_part1, 2000));
        QVERIFY(writeMp3(m_part2, 2000));
        QVERIFY(writeFile(m_part1 + QStringLiteral(".cover.jpg")));
        QVERIFY(writeFile(m_faust));

        Work book;
        book.id = QStringLiteral("book");
        book.title = QStringLiteral("Dracula");
        book.authors = { QStringLiteral("Bram Stoker") };
        Format ebook;
        ebook.kind = Format::Kind::Ebook;
        ebook.filePath = m_epub;
        ebook.coverPath = m_epub + QStringLiteral(".cover.jpg");
        book.formats = { ebook };

        Work audio;
        audio.id = QStringLiteral("audio");
        audio.title = QStringLiteral("Dracula (Hörbuch)");
        audio.narrators = { QStringLiteral("Maren Brandt") };
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = m_part1;
        audiobook.coverPath = m_part1 + QStringLiteral(".cover.jpg");
        audiobook.durationSeconds = 4;
        audiobook.chapters = { { QStringLiteral("Eins"), QString(), 0, 2000 },
                               { QStringLiteral("Zwei"), m_part2, 0, 2000 } };
        audio.formats = { audiobook };

        Work faust;
        faust.id = QStringLiteral("faust");
        faust.title = QStringLiteral("Faust");
        Format faustEbook;
        faustEbook.kind = Format::Kind::Ebook;
        faustEbook.filePath = m_faust;
        faust.formats = { faustEbook };

        m_backend = std::make_unique<MemoryBackend>();
        m_backend->root = m_root;
        m_backend->works = { audio, book, faust };
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        m_library->setDeleteFilesPermanently(true); // not into the recycle bin of whoever runs the tests
    }

    void cleanup()
    {
        m_library.reset();
        m_backend.reset();
        m_dir.reset();
    }

    // "Done when" of phase 9: a standalone ebook and a standalone audiobook
    // become one work.
    void mergesEbookAndAudiobook()
    {
        QSignalSpy aboutToChange(m_library.get(), &LibraryController::worksAboutToChange);
        const QStringList ids = { QStringLiteral("audio"), QStringLiteral("book") };
        QCOMPARE(m_library->mergeTitle(ids), QStringLiteral("Dracula"));
        QCOMPARE(m_library->mergeWorks(ids), QStringLiteral("book"));
        QCOMPARE(aboutToChange.count(), 1);
        QCOMPARE(aboutToChange.first().first().toStringList(), ids);

        const WorkModel *works = m_library->works();
        QCOMPARE(works->count(), 2);
        QCOMPARE(works->rowOf(QStringLiteral("audio")), -1);
        const Work *merged = works->workAt(works->rowOf(QStringLiteral("book")));
        QVERIFY(merged);
        QCOMPARE(merged->title, QStringLiteral("Dracula"));
        QVERIFY(merged->hasEbook());
        QVERIFY(merged->hasAudiobook());
        QCOMPARE(merged->narrators, QStringList{ QStringLiteral("Maren Brandt") });

        // Stored, and the files are where they were.
        QCOMPARE(m_backend->works.size(), 2);
        QVERIFY(!m_backend->has(QStringLiteral("audio")));
        QVERIFY(QFileInfo::exists(m_part1));
        QVERIFY(QFileInfo::exists(m_part2));
        QVERIFY(QFileInfo::exists(m_epub));

        // Nothing to merge.
        QVERIFY(m_library->mergeWorks({ QStringLiteral("unknown") }).isEmpty());
        QVERIFY(m_library->mergeWorks({}).isEmpty());
        QVERIFY(m_library->mergeTitle({}).isEmpty());
    }

    // Removing takes the works out of the library and their files out of
    // the library folder, with the folders that are empty afterwards.
    void removesWorksAndFiles()
    {
        QCOMPARE(m_library->removeWorks({ QStringLiteral("faust"), QStringLiteral("unknown") }), 1);
        QCOMPARE(m_library->works()->count(), 2);
        QVERIFY(!m_backend->has(QStringLiteral("faust")));
        QVERIFY(!QFileInfo::exists(m_faust));
        QVERIFY(!QFileInfo::exists(m_root + QStringLiteral("/Goethe"))); // author and title folder are gone
        QVERIFY(QFileInfo::exists(m_epub));                             // other works are untouched

        // A merged work: the files of all its formats, the parts of the
        // audiobook and the covers.
        QCOMPARE(m_library->mergeWorks({ QStringLiteral("audio"), QStringLiteral("book") }), QStringLiteral("book"));
        QCOMPARE(m_library->removeWorks({ QStringLiteral("book") }), 1);
        QCOMPARE(m_library->works()->count(), 0);
        QVERIFY(!QFileInfo::exists(m_epub));
        QVERIFY(!QFileInfo::exists(m_part1));
        QVERIFY(!QFileInfo::exists(m_part2));
        QVERIFY(!QFileInfo::exists(m_root + QStringLiteral("/Bram Stoker")));
        QVERIFY(QFileInfo::exists(m_root)); // the library folder itself stays

        QCOMPARE(m_library->removeWorks({ QStringLiteral("book") }), 0);
    }

    // Removing in the tab "E-Books" or "Hörbücher": only that format and its
    // files; a work without the other format goes as a whole.
    void removesOneFormat()
    {
        QCOMPARE(m_library->mergeWorks({ QStringLiteral("audio"), QStringLiteral("book") }), QStringLiteral("book"));
        const WorkModel *works = m_library->works();
        const auto book = [works] { return works->workAt(works->rowOf(QStringLiteral("book"))); };

        QCOMPARE(m_library->removeWorks({ QStringLiteral("book") }, QStringLiteral("audiobook")), 1);
        QVERIFY(book());
        QVERIFY(book()->hasEbook());
        QVERIFY(!book()->hasAudiobook());
        QVERIFY(m_backend->has(QStringLiteral("book")));
        QVERIFY(!QFileInfo::exists(m_part1));
        QVERIFY(!QFileInfo::exists(m_part2));
        QVERIFY(!QFileInfo::exists(m_part1 + QStringLiteral(".cover.jpg")));
        QVERIFY(QFileInfo::exists(m_epub)); // the ebook stays
        QVERIFY(QFileInfo::exists(m_epub + QStringLiteral(".cover.jpg")));

        // The ebook is all that is left: the work goes.
        QCOMPARE(m_library->removeWorks({ QStringLiteral("book") }, QStringLiteral("ebook")), 1);
        QVERIFY(!book());
        QVERIFY(!m_backend->has(QStringLiteral("book")));
        QVERIFY(!QFileInfo::exists(m_epub));

        // "" removes the whole works.
        QCOMPARE(m_library->removeWorks({ QStringLiteral("faust") }, QString()), 1);
        QCOMPARE(works->count(), 0);
    }

    // Sample data (no library folder) owns no files: nothing is deleted.
    void removingSampleDataKeepsFiles()
    {
        m_library.reset();
        m_backend->root.clear();
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        QCOMPARE(m_library->removeWorks({ QStringLiteral("faust") }), 1);
        QCOMPARE(m_library->works()->count(), 2);
        QVERIFY(QFileInfo::exists(m_faust));
    }

    // The player lets go of an audiobook that is merged or removed, after
    // storing where it was.
    void playerLetsGo()
    {
        PlayerController player(*m_library, nullptr);
        QVERIFY(player.load(QStringLiteral("audio")));
        player.seek(1000);
        QVERIFY(player.hasWork());

        QCOMPARE(m_library->mergeWorks({ QStringLiteral("audio"), QStringLiteral("book") }), QStringLiteral("book"));
        QVERIFY(!player.hasWork());
        const Work *merged = m_library->works()->workAt(m_library->works()->rowOf(QStringLiteral("book")));
        QCOMPARE(merged->format(Format::Kind::Audiobook)->positionMs, 1000); // the position came along

        // The merged work plays again, and is let go when it is removed.
        QVERIFY(player.load(QStringLiteral("book")));
        QCOMPARE(player.position(), 1000.0);
        QCOMPARE(m_library->removeWorks({ QStringLiteral("book") }), 1);
        QVERIFY(!player.hasWork());

        // Other works do not disturb the player.
        QVERIFY(!player.load(QStringLiteral("faust"))); // no audiobook
    }

    // Opening a library: a new one, an existing one, the errors, switching.
    void openLibrary()
    {
        m_library.reset();
        QTemporaryDir dir;
        const QString first = dir.filePath(QStringLiteral("Erste"));
        const QString second = dir.filePath(QStringLiteral("Zweite"));
        LibraryController library(nullptr);
        QCOMPARE(library.libraryRoot(), QString());
        QVERIFY(!library.canImport());

        // A library that is not there is not created unless asked for.
        QSignalSpy changed(&library, &LibraryController::libraryChanged);
        QCOMPARE(library.openLibrary(first, false), LibraryController::NotFound);
        QCOMPARE(library.libraryError(), LibraryController::NotFound);
        QCOMPARE(library.libraryPath(), QDir::cleanPath(first));
        QVERIFY(!QFileInfo::exists(first));
        QVERIFY(changed.count() > 0);

        // A new one, then something in it.
        QCOMPARE(library.openLibrary(first, true), LibraryController::NoError);
        QCOMPARE(library.libraryError(), LibraryController::NoError);
        QVERIFY(library.canImport());
        Work work;
        work.id = QStringLiteral("w1");
        work.title = QStringLiteral("Dracula");
        QVERIFY(library.saveWork(work));
        QVERIFY(QFileInfo::exists(first + QStringLiteral("/calibly.lock")));

        // Switching: the other library's works replace these; who uses a
        // work is told first.
        QSignalSpy aboutToChange(&library, &LibraryController::worksAboutToChange);
        QCOMPARE(library.openLibrary(second, true), LibraryController::NoError);
        QCOMPARE(aboutToChange.count(), 1);
        QCOMPARE(library.works()->count(), 0);
        QCOMPARE(library.libraryRoot(), QDir::cleanPath(second));
        QVERIFY(!QFileInfo::exists(first + QStringLiteral("/calibly.lock"))); // the old one is free again

        // A library that cannot be opened leaves the open one as it is.
        QCOMPARE(library.openLibrary(dir.filePath(QStringLiteral("gibt-es-nicht")), false), LibraryController::NotFound);
        QCOMPARE(library.libraryError(), LibraryController::NoError);
        QCOMPARE(library.libraryRoot(), QDir::cleanPath(second));

        // Back to the first one, with its work.
        QCOMPARE(library.openLibraryUrl(QUrl::fromLocalFile(first), false), LibraryController::NoError);
        QCOMPARE(library.works()->count(), 1);
    }

    // A library is used by one calibly at a time; a second start asks the
    // first one to come to the front.
    void libraryInUse()
    {
        m_library.reset();
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("Bibliothek"));
        QVERIFY(QDir().mkpath(path));
        LibraryLock other(path);
        QCOMPARE(other.acquire(), LibraryLock::Acquired);

        {
            LibraryController library(nullptr);
            QCOMPARE(library.openLibrary(path, true), LibraryController::InUse);
            QCOMPARE(library.libraryError(), LibraryController::InUse);
        }

        QSignalSpy activation(&other, &LibraryLock::activationRequested);
        QVERIFY(LibraryLock::activateOwner(path));
        QVERIFY(activation.count() > 0 || activation.wait(2000));

        // Still held while the first one runs.
        LibraryLock second(path);
        QCOMPARE(second.acquire(), LibraryLock::HeldElsewhere);
    }

    // The folder of an opened library is remembered for the next start.
    void remembersLibrary()
    {
        m_library.reset();
        QTemporaryDir dir;
        QSettings().remove(QStringLiteral("library/path"));
        {
            LibraryController library(nullptr);
            QCOMPARE(library.openLibrary(dir.path(), true), LibraryController::NoError);
            QVERIFY(!QSettings().contains(QStringLiteral("library/path"))); // off by default
        }
        LibraryController library(nullptr);
        library.setRememberLibrary(true);
        QCOMPARE(library.openLibrary(dir.path(), true), LibraryController::NoError);
        QCOMPARE(QSettings().value(QStringLiteral("library/path")).toString(), QDir::cleanPath(dir.path()));
        QSettings().remove(QStringLiteral("library/path"));
    }

    // Phase 12: all selected titles as finished, then as new again.
    void marksWorks()
    {
        const QStringList ids = { QStringLiteral("book"), QStringLiteral("faust") };
        QSignalSpy aboutToChange(m_library.get(), &LibraryController::worksAboutToChange);
        QCOMPARE(m_library->markWorks(ids, true), 2);
        QCOMPARE(aboutToChange.count(), 1);
        const WorkModel *works = m_library->works();
        for (const QString &id : ids)
            QCOMPARE(works->workAt(works->rowOf(id))->state(), Work::State::Finished);

        QCOMPARE(m_library->markWorks(ids, false), 2);
        for (const QString &id : ids) {
            const Work *work = works->workAt(works->rowOf(id));
            QCOMPARE(work->state(), Work::State::New);
            QCOMPARE(work->formats.first().progress, 0.0);
            QVERIFY(!work->lastOpened.isValid());
        }
        QCOMPARE(m_library->markWorks({ QStringLiteral("unknown") }, true), 0);
    }

    // "Als Serie zusammenfassen": the name for all, numbers kept or given.
    void setsSeries()
    {
        Work audio = *m_library->works()->workAt(m_library->works()->rowOf(QStringLiteral("audio")));
        audio.seriesIndex = 2;
        QVERIFY(m_library->saveWork(audio));
        QSignalSpy namesChanged(m_library.get(), &LibraryController::seriesNamesChanged);

        const QStringList ids = { QStringLiteral("book"), QStringLiteral("faust"), QStringLiteral("audio") };
        QCOMPARE(m_library->setSeries(ids, QStringLiteral("  Klassiker ")), 3);
        const WorkModel *works = m_library->works();
        const auto work = [works](const char *id) { return works->workAt(works->rowOf(QLatin1String(id))); };
        QCOMPARE(work("book")->series, QStringLiteral("Klassiker"));
        QCOMPARE(work("audio")->seriesIndex, 2); // kept
        QCOMPARE(work("book")->seriesIndex, 1);  // the free numbers in the given order
        QCOMPARE(work("faust")->seriesIndex, 3);
        QVERIFY(namesChanged.count() > 0);
        QCOMPARE(m_library->seriesNames(), QStringList{ QStringLiteral("Klassiker") });
        QCOMPARE(m_library->suggestedSeries({ QStringLiteral("faust") }), QStringLiteral("Klassiker"));
        QCOMPARE(m_library->setSeries(ids, QStringLiteral("   ")), 0); // no name
    }

    // Titles without volume numbers, with gaps or doubles: the user gives the
    // order, and they get the free numbers in that order.
    void ordersSeries()
    {
        const WorkModel *works = m_library->works();
        const auto work = [works](const char *id) { return works->workAt(works->rowOf(QLatin1String(id))); };
        const auto number = [this, &work](const char *id, int index, const QString &series = QString()) {
            Work changed = *work(id);
            changed.seriesIndex = index;
            changed.series = series;
            QVERIFY(m_library->saveWork(changed));
        };
        const QStringList ids = { QStringLiteral("book"), QStringLiteral("faust"), QStringLiteral("audio") };
        const QString name = QStringLiteral("Klassiker");

        QVERIFY(m_library->seriesNeedsOrder(ids, name)); // no numbers
        QVERIFY(!m_library->seriesNeedsOrder({ QStringLiteral("book") }, name)); // one title
        number("book", 1);
        number("faust", 2);
        number("audio", 3);
        QVERIFY(!m_library->seriesNeedsOrder(ids, name)); // 1, 2, 3
        number("audio", 4);
        QVERIFY(m_library->seriesNeedsOrder(ids, name)); // a gap
        number("audio", 2);
        QVERIFY(m_library->seriesNeedsOrder(ids, name)); // twice 2

        // Another volume of the series has number 2: it is left out.
        number("audio", 2, name);
        const QStringList two = { QStringLiteral("faust"), QStringLiteral("book") };
        number("book", 3);
        number("faust", 1);
        QVERIFY(!m_library->seriesNeedsOrder(two, name)); // 1, 2, 3 together
        number("faust", 2);
        QVERIFY(m_library->seriesNeedsOrder(two, name)); // twice 2
        QCOMPARE(m_library->seriesNumbers(two, name), (QList<int>{ 1, 3 }));

        QCOMPARE(m_library->setSeriesOrder(two, QStringLiteral(" Klassiker ")), 2);
        QCOMPARE(work("faust")->seriesIndex, 1);
        QCOMPARE(work("book")->seriesIndex, 3);
        QCOMPARE(work("book")->series, name);
        QCOMPARE(work("audio")->seriesIndex, 2); // untouched
        QCOMPARE(m_library->setSeriesOrder(two, QStringLiteral("  ")), 0);
    }

    // Phase 13: a series can be dissolved again, or works taken out of it.
    void removesSeries()
    {
        const QStringList ids = { QStringLiteral("book"), QStringLiteral("faust"), QStringLiteral("audio") };
        QCOMPARE(m_library->setSeries(ids, QStringLiteral("Klassiker")), 3);
        const WorkModel *works = m_library->works();
        const auto work = [works](const char *id) { return works->workAt(works->rowOf(QLatin1String(id))); };

        QCOMPARE(m_library->clearSeries({ QStringLiteral("audio") }), 1);
        QVERIFY(work("audio")->series.isEmpty());
        QCOMPARE(work("audio")->seriesIndex, 0);
        QCOMPARE(work("book")->series, QStringLiteral("Klassiker"));
        QCOMPARE(m_library->clearSeries({ QStringLiteral("audio") }), 0); // nothing left to clear

        // Compared like the stacks: without case and accents.
        QCOMPARE(m_library->removeSeries(QStringLiteral("KLASSÌKER")), 2);
        QVERIFY(work("book")->series.isEmpty());
        QCOMPARE(work("faust")->seriesIndex, 0);
        QVERIFY(m_library->seriesNames().isEmpty());
        QCOMPARE(m_library->removeSeries(QString()), 0);
    }

    // Taking back "finished" at the end: back at the start, and no longer
    // "started", so it does not show under "Weitermachen" at 0 %. The work
    // stays opened (not "NEU").
    void unfinishedAtStartIsNotStarted()
    {
        const WorkModel *works = m_library->works();
        const auto work = [works](const char *id) { return works->workAt(works->rowOf(QLatin1String(id))); };
        Work faust = *work("faust");
        faust.markOpened(0, QDateTime::currentDateTime());
        faust.formats[0].readingPosition = QStringLiteral("end");
        QVERIFY(m_library->saveWork(faust));

        QVERIFY(m_library->setFinished(QStringLiteral("faust"), QStringLiteral("ebook"), true));
        QVERIFY(m_library->setFinished(QStringLiteral("faust"), QStringLiteral("ebook"), false));
        QCOMPARE(work("faust")->formats.first().progress, 0.0);
        QVERIFY(!work("faust")->formats.first().lastOpened.isValid());
        QVERIFY(work("faust")->formats.first().readingPosition.isEmpty());
        QVERIFY(work("faust")->lastOpened.isValid());
        QCOMPARE(work("faust")->state(), Work::State::Normal);

        // An audiobook stopped in the middle goes back there and stays started.
        Work audio = *work("audio");
        audio.markOpened(0, QDateTime::currentDateTime());
        audio.formats[0].positionMs = 1000;
        audio.formats[0].progress = 0.25;
        QVERIFY(m_library->saveWork(audio));
        QVERIFY(m_library->setFinished(QStringLiteral("audio"), QStringLiteral("audiobook"), true));
        QVERIFY(m_library->setFinished(QStringLiteral("audio"), QStringLiteral("audiobook"), false));
        QCOMPARE(work("audio")->formats.first().progress, 0.25);
        QVERIFY(work("audio")->formats.first().lastOpened.isValid());
    }

    // "Aus Weitermachen entfernen": that format back to the start, so it
    // leaves "Weitermachen"; the other format and the work's "last opened" stay.
    void removesFromContinue()
    {
        QCOMPARE(m_library->mergeWorks({ QStringLiteral("audio"), QStringLiteral("book") }), QStringLiteral("book"));
        const WorkModel *works = m_library->works();
        const auto book = [works] { return works->workAt(works->rowOf(QStringLiteral("book"))); };
        Work started = *book();
        const QDateTime now = QDateTime::currentDateTime();
        for (int i = 0; i < started.formats.size(); ++i) {
            Format &format = started.formats[i];
            started.markOpened(i, now);
            format.progress = 0.4;
            if (format.kind == Format::Kind::Audiobook)
                format.positionMs = 1500;
            else
                format.readingPosition = QStringLiteral("chapter-3");
        }
        QVERIFY(m_library->saveWork(started));

        ContinueModel continueModel;
        continueModel.setSourceModel(m_library->works());
        continueModel.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(continueModel.count(), 1);

        QSignalSpy aboutToChange(m_library.get(), &LibraryController::worksAboutToChange);
        QVERIFY(m_library->removeFromContinue(QStringLiteral("book"), QStringLiteral("audiobook")));
        QCOMPARE(aboutToChange.count(), 1);
        QCOMPARE(continueModel.count(), 0);
        const Format *audiobook = book()->format(Format::Kind::Audiobook);
        QCOMPARE(audiobook->progress, 0.0);
        QCOMPARE(audiobook->positionMs, 0);
        QVERIFY(!audiobook->lastOpened.isValid());
        const Format *ebook = book()->format(Format::Kind::Ebook);
        QCOMPARE(ebook->progress, 0.4); // the ebook is untouched
        QCOMPARE(ebook->readingPosition, QStringLiteral("chapter-3"));
        QVERIFY(book()->lastOpened.isValid());
        QCOMPARE(book()->state(), Work::State::Normal);

        continueModel.setFormatFilter(WorkFilterModel::Ebook);
        QCOMPARE(continueModel.count(), 1);
        QVERIFY(m_library->removeFromContinue(QStringLiteral("book"), QStringLiteral("ebook")));
        QCOMPARE(continueModel.count(), 0);
        QVERIFY(book()->format(Format::Kind::Ebook)->readingPosition.isEmpty());

        QVERIFY(!m_library->removeFromContinue(QStringLiteral("faust"), QStringLiteral("audiobook"))); // none
        QVERIFY(!m_library->removeFromContinue(QStringLiteral("unknown"), QStringLiteral("ebook")));
    }

    // "Kombinieren" two books into volumes, mark one volume, and dissolve
    // the combination again.
    void combinesAndSplits()
    {
        const QString id = m_library->mergeWorks({ QStringLiteral("faust"), QStringLiteral("book") });
        QCOMPARE(id, QStringLiteral("faust"));
        const WorkModel *works = m_library->works();
        const auto combined = [works, &id] { return works->workAt(works->rowOf(id)); };
        QCOMPARE(works->count(), 2);
        QVERIFY(combined()->isCombined());
        // Volumes in title order (no numbers): Dracula, then Faust.
        QCOMPARE(combined()->formats.at(0).filePath, m_epub);
        QCOMPARE(combined()->formats.at(1).filePath, m_faust);

        // One volume: only that one changes.
        QVERIFY(m_library->setFormatFinished(id, 1, true));
        QCOMPARE(combined()->formats.at(0).progress, 0.0);
        QCOMPARE(combined()->formats.at(1).progress, 1.0);
        QVERIFY(combined()->state() != Work::State::Finished);
        QCOMPARE(combined()->currentFormat(Format::Kind::Ebook), 0);
        // The kind: all volumes.
        QVERIFY(m_library->setFinished(id, QStringLiteral("ebook"), true));
        QCOMPARE(combined()->formats.at(0).progress, 1.0);
        QCOMPARE(combined()->state(), Work::State::Finished);

        QSignalSpy aboutToChange(m_library.get(), &LibraryController::worksAboutToChange);
        QCOMPARE(m_library->splitWork(id), 2);
        QCOMPARE(aboutToChange.count(), 1);
        QCOMPARE(works->count(), 3);
        const Work *book = works->workAt(works->rowOf(QStringLiteral("book")));
        const Work *faust = works->workAt(works->rowOf(QStringLiteral("faust")));
        QVERIFY(book && faust);
        QCOMPARE(book->title, QStringLiteral("Dracula"));
        QCOMPARE(book->formats.size(), 1);
        QCOMPARE(book->formats.first().progress, 1.0); // its progress stays with it
        QCOMPARE(faust->title, QStringLiteral("Faust"));
        QVERIFY(!faust->isCombined());
        QVERIFY(m_backend->has(QStringLiteral("book")));
        QCOMPARE(m_library->splitWork(QStringLiteral("book")), 0); // nothing to dissolve
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    QString m_root;
    QString m_epub;
    QString m_part1;
    QString m_part2;
    QString m_faust;
    std::unique_ptr<MemoryBackend> m_backend;
    std::unique_ptr<LibraryController> m_library;
};

QTEST_GUILESS_MAIN(TestLibraryController)
#include "tst_librarycontroller.moc"
