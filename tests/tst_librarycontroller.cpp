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
