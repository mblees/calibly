#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "importcontroller.h"
#include "importqueuemodel.h"
#include "librarybackend.h"
#include "librarycontroller.h"

namespace {

// MP3 of silence (MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono), no tags.
void writeMp3(const QString &path, int durationMs)
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

void writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
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
};

using Status = ImportQueueModel::Status;

Status statusAt(const ImportQueueModel &queue, int row)
{
    return Status(queue.data(queue.index(row), ImportQueueModel::StatusRole).toInt());
}

QVariant roleAt(const ImportQueueModel &queue, int row, ImportQueueModel::Role role)
{
    return queue.data(queue.index(row), role);
}

} // namespace

class TestImportController : public QObject
{
    Q_OBJECT

private slots:
    void queueModel()
    {
        ImportQueueModel queue;
        QSignalSpy countChanged(&queue, &ImportQueueModel::countChanged);

        ImportQueueModel::Entry job;
        job.title = QStringLiteral("Neue Bücher");
        job.status = ImportQueueModel::Reading;
        const int jobId = queue.append(job);
        QCOMPARE(queue.count(), 1);
        QVERIFY(queue.active());

        // The job row is replaced by its works, in its place.
        ImportQueueModel::Entry done;
        done.title = QStringLiteral("Faust");
        done.status = ImportQueueModel::Done;
        ImportQueueModel::Entry review;
        review.title = QStringLiteral("scan 042");
        review.status = ImportQueueModel::NeedsReview;
        const QList<int> ids = queue.replace(jobId, { done, review });
        QCOMPARE(ids.size(), 2);
        QCOMPARE(queue.count(), 2);
        QVERIFY(!queue.entry(jobId));
        QCOMPARE(roleAt(queue, 0, ImportQueueModel::TitleRole).toString(), QStringLiteral("Faust"));
        QCOMPARE(queue.finishedCount(), 2);
        QCOMPARE(queue.attentionCount(), 1);
        QVERIFY(!queue.active());

        queue.markReviewed(ids.at(1));
        QCOMPARE(statusAt(queue, 1), ImportQueueModel::Done);
        QCOMPARE(queue.attentionCount(), 0);

        // Running rows stay when the finished ones are cleared.
        ImportQueueModel::Entry waiting;
        queue.append(waiting);
        queue.clearFinished();
        QCOMPARE(queue.count(), 1);
        QCOMPARE(statusAt(queue, 0), ImportQueueModel::Waiting);
        QVERIFY(countChanged.count() > 0);
    }

    // A folder dropped on free space becomes a new work, a file dropped onto
    // its cover a format of it; a file without metadata needs review; the
    // same folder again is skipped. Jobs dropped while another one runs wait.
    void importJobs()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString folder = dir.filePath(QStringLiteral("in/Dracula - Bram Stoker"));
        QVERIFY(QDir().mkpath(folder));
        writeMp3(folder + QStringLiteral("/01.mp3"), 1000);
        writeMp3(folder + QStringLiteral("/02.mp3"), 1500);
        const QString scan = dir.filePath(QStringLiteral("in/scan_042.mobi"));
        writeFile(scan, "scan");
        const QString ebook = dir.filePath(QStringLiteral("in/irgendwas.mobi"));
        writeFile(ebook, "dracula ebook");

        MemoryBackend backend;
        backend.root = dir.filePath(QStringLiteral("library"));
        LibraryController library(backend, nullptr);
        ImportController importer(library, nullptr);
        ImportQueueModel &queue = *importer.queue();
        QSignalSpy finished(&importer, &ImportController::importFinished);

        const QVariantMap summary = importer.inspect({ QUrl::fromLocalFile(folder), QUrl::fromLocalFile(scan) });
        QCOMPARE(summary.value(QStringLiteral("audiobookCount")).toInt(), 2);
        QCOMPARE(summary.value(QStringLiteral("ebookCount")).toInt(), 1);
        QCOMPARE(summary.value(QStringLiteral("name")).toString(), QStringLiteral("Dracula - Bram Stoker"));
        QCOMPARE(summary.value(QStringLiteral("typesText")).toString(), QStringLiteral("1 MOBI, 2 MP3"));

        importer.importFiles({ QUrl::fromLocalFile(folder) });
        importer.importFiles({ QUrl::fromLocalFile(scan) });
        QVERIFY(importer.busy());
        QCOMPARE(queue.count(), 2);
        QCOMPARE(statusAt(queue, 1), ImportQueueModel::Waiting);

        QTRY_COMPARE(finished.count(), 2);
        QVERIFY(!importer.busy());
        QCOMPARE(library.works()->count(), 2);

        // The MP3 folder: one new work named after the folder.
        QCOMPARE(queue.count(), 2);
        QCOMPARE(statusAt(queue, 0), ImportQueueModel::Done);
        QCOMPARE(roleAt(queue, 0, ImportQueueModel::TitleRole).toString(), QStringLiteral("Dracula"));
        QVERIFY(roleAt(queue, 0, ImportQueueModel::MergedRole).toBool());
        QVERIFY(roleAt(queue, 0, ImportQueueModel::NewWorkRole).toBool());
        QCOMPARE(roleAt(queue, 0, ImportQueueModel::FileTypesRole).toString(), QStringLiteral("MP3"));
        const QString draculaId = roleAt(queue, 0, ImportQueueModel::WorkIdRole).toString();
        const Work *dracula = library.works()->workAt(library.works()->rowOf(draculaId));
        QVERIFY(dracula);
        QCOMPARE(dracula->authors, QStringList{ QStringLiteral("Bram Stoker") });
        QCOMPARE(dracula->formats.size(), 1);
        QCOMPARE(dracula->formats.first().chapters.size(), 2);

        // The scan: imported, but its title is only the file name.
        QCOMPARE(statusAt(queue, 1), ImportQueueModel::NeedsReview);
        QCOMPARE(queue.attentionCount(), 1);

        // A file dropped onto the cover of Dracula becomes its ebook.
        queue.clearFinished();
        importer.importFiles({ QUrl::fromLocalFile(ebook) }, draculaId);
        QTRY_COMPARE(finished.count(), 3);
        QCOMPARE(library.works()->count(), 2);
        QCOMPARE(queue.count(), 1);
        QCOMPARE(statusAt(queue, 0), ImportQueueModel::Done);
        QVERIFY(!roleAt(queue, 0, ImportQueueModel::NewWorkRole).toBool());
        QCOMPARE(roleAt(queue, 0, ImportQueueModel::TitleRole).toString(), QStringLiteral("Dracula"));
        dracula = library.works()->workAt(library.works()->rowOf(draculaId));
        QVERIFY(dracula->hasEbook());
        QVERIFY(dracula->hasAudiobook());
        QCOMPARE(dracula->title, QStringLiteral("Dracula"));

        // The same folder again: nothing new, one "skipped" row.
        queue.clearFinished();
        importer.importFiles({ QUrl::fromLocalFile(folder) });
        QTRY_COMPARE(finished.count(), 4);
        QCOMPARE(finished.last().at(0).toInt(), 0);
        QCOMPARE(finished.last().at(1).toInt(), 2);
        QCOMPARE(queue.count(), 1);
        QCOMPARE(statusAt(queue, 0), ImportQueueModel::Skipped);
        QCOMPARE(roleAt(queue, 0, ImportQueueModel::FileCountRole).toInt(), 2);
        QCOMPARE(library.works()->count(), 2);
    }
};

QTEST_GUILESS_MAIN(TestImportController)
#include "tst_importcontroller.moc"
