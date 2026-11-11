#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "workdetails.h"
#include "workmodel.h"

namespace {

Work makeWork(const QString &id, const QString &title)
{
    Work work;
    work.id = id;
    work.title = title;
    work.authors = { QStringLiteral("Herman Melville") };
    work.narrators = { QStringLiteral("Andreas Lindner") };
    work.description = QStringLiteral("Ein Wal.");
    work.tags = { QStringLiteral("Klassiker"), QStringLiteral("Meer") };
    work.rating = 5;
    work.language = QStringLiteral("de");
    work.publishedYear = 1851;
    Format ebook;
    ebook.kind = Format::Kind::Ebook;
    ebook.filePath = QStringLiteral("does/not/exist/moby-dick.epub");
    ebook.pages = 652;
    ebook.fileSize = 1669200;
    ebook.progress = 0.31;
    Format audiobook;
    audiobook.kind = Format::Kind::Audiobook;
    audiobook.filePath = QStringLiteral("does/not/exist/moby-dick.m4b");
    audiobook.durationSeconds = 89340;
    work.formats = { ebook, audiobook };
    return work;
}

class FixedBackend : public LibraryBackend
{
public:
    QList<Work> works;
    QList<Work> loadWorks() override { return works; }
};

} // namespace

class TestWorkDetails : public QObject
{
    Q_OBJECT

private slots:
    void detailsOfWork()
    {
        WorkModel model;
        model.setWorks({ makeWork(QStringLiteral("a"), QStringLiteral("Dracula")),
                         makeWork(QStringLiteral("moby"), QStringLiteral("Moby Dick")) });
        WorkDetails details;
        details.setModel(&model);
        QVERIFY(!details.valid());

        details.setWorkId(QStringLiteral("moby"));
        QVERIFY(details.valid());
        QCOMPARE(details.title(), QStringLiteral("Moby Dick"));
        QCOMPARE(details.narrators(), QStringList{ QStringLiteral("Andreas Lindner") });
        QCOMPARE(details.tags().size(), 2);
        QCOMPARE(details.rating(), 5);
        QCOMPARE(details.publishedYear(), 1851);

        const QVariantList formats = details.formats();
        QCOMPARE(formats.size(), 2);
        const QVariantMap ebook = formats.at(0).toMap();
        QCOMPARE(ebook.value(QStringLiteral("kind")).toString(), QStringLiteral("ebook"));
        QCOMPARE(ebook.value(QStringLiteral("fileName")).toString(), QStringLiteral("moby-dick.epub"));
        QCOMPARE(ebook.value(QStringLiteral("fileType")).toString(), QStringLiteral("EPUB"));
        QCOMPARE(ebook.value(QStringLiteral("pages")).toInt(), 652);
        QCOMPARE(ebook.value(QStringLiteral("available")).toBool(), false);
        const QVariantMap audiobook = formats.at(1).toMap();
        QCOMPARE(audiobook.value(QStringLiteral("kind")).toString(), QStringLiteral("audiobook"));
        QCOMPARE(audiobook.value(QStringLiteral("durationSeconds")).toInt(), 89340);
    }

    void followsModelChanges()
    {
        WorkModel model;
        model.setWorks({ makeWork(QStringLiteral("moby"), QStringLiteral("Moby Dick")) });
        WorkDetails details;
        details.setModel(&model);
        details.setWorkId(QStringLiteral("moby"));

        QSignalSpy spy(&details, &WorkDetails::changed);
        model.setWorks({ makeWork(QStringLiteral("moby"), QStringLiteral("Moby-Dick oder Der Wal")) });
        QVERIFY(spy.count() > 0);
        QCOMPARE(details.title(), QStringLiteral("Moby-Dick oder Der Wal"));

        model.setWorks({});
        QVERIFY(!details.valid());
        QCOMPARE(details.title(), QString());
        QVERIFY(details.formats().isEmpty());
    }

    void availableWhenFileExists()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("book.epub"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        Work work = makeWork(QStringLiteral("moby"), QStringLiteral("Moby Dick"));
        work.formats[0].filePath = path;
        WorkModel model;
        model.setWorks({ work });
        WorkDetails details;
        details.setModel(&model);
        details.setWorkId(QStringLiteral("moby"));
        QCOMPARE(details.formats().at(0).toMap().value(QStringLiteral("available")).toBool(), true);
        QCOMPARE(details.formats().at(1).toMap().value(QStringLiteral("available")).toBool(), false);
    }

    void openFormatFailsForMissingFiles()
    {
        FixedBackend backend;
        Work ebookOnly = makeWork(QStringLiteral("moby"), QStringLiteral("Moby Dick"));
        ebookOnly.formats.removeLast();
        backend.works = { ebookOnly };
        LibraryController controller(backend, nullptr);

        QCOMPARE(controller.openFormat(QStringLiteral("unknown"), QStringLiteral("ebook")), false);
        QCOMPARE(controller.openFormat(QStringLiteral("moby"), QStringLiteral("ebook")), false);     // file missing
        QCOMPARE(controller.openFormat(QStringLiteral("moby"), QStringLiteral("audiobook")), false); // no audiobook
    }
};

QTEST_GUILESS_MAIN(TestWorkDetails)
#include "tst_workdetails.moc"
