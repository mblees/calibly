// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "exportcontroller.h"
#include "librarybackend.h"
#include "librarycontroller.h"
#include "libraryexport.h"

namespace {

class MemoryBackend : public LibraryBackend
{
public:
    QList<Work> works;
    QList<Work> loadWorks() override { return works; }
    bool saveWork(const Work &) override { return true; }
};

void writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

Format ebook(const QString &path)
{
    Format format;
    format.kind = Format::Kind::Ebook;
    format.filePath = path;
    return format;
}

// An audiobook of several files: one chapter per file.
Format audiobook(const QStringList &paths)
{
    Format format;
    format.kind = Format::Kind::Audiobook;
    format.filePath = paths.value(0);
    for (const QString &path : paths) {
        Chapter chapter;
        chapter.filePath = path;
        format.chapters.append(chapter);
    }
    return format;
}

Work work(const QString &id, const QList<Format> &formats)
{
    Work result;
    result.id = id;
    result.title = id;
    result.formats = formats;
    return result;
}

QStringList names(const QString &folder)
{
    return QDir(folder).entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
}

} // namespace

class TestExport : public QObject
{
    Q_OBJECT

private slots:
    void folderNames()
    {
        QTemporaryDir dir;
        QCOMPARE(LibraryExport::folderName(QDate(2026, 11, 13)), QStringLiteral("calibly-Export 2026-11-13"));
        QCOMPARE(LibraryExport::uniqueFolder(dir.path(), QStringLiteral("Export")), dir.filePath(QStringLiteral("Export")));
        QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("Export")));
        QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("Export (2)")));
        QCOMPARE(LibraryExport::uniqueFolder(dir.path(), QStringLiteral("Export")),
                 dir.filePath(QStringLiteral("Export (3)")));
    }

    // Ebooks and audiobooks go to their subfolder without further folders;
    // all files of a multi-file audiobook are exported; names that are taken
    // get " (2)"; the same file is planned once; missing files are listed.
    void plan()
    {
        QTemporaryDir dir;
        const QString lib = dir.filePath(QStringLiteral("library"));
        writeFile(lib + QStringLiteral("/Mann/Buddenbrooks/Buddenbrooks.epub"), "epub");
        writeFile(lib + QStringLiteral("/Mann/Buddenbrooks/01.mp3"), "a1");
        writeFile(lib + QStringLiteral("/Mann/Buddenbrooks/02.mp3"), "a2");
        writeFile(lib + QStringLiteral("/Kafka/Verwandlung/01.mp3"), "b1");
        writeFile(lib + QStringLiteral("/Kafka/Verwandlung/Die Verwandlung.pdf"), "pdf");
        writeFile(lib + QStringLiteral("/Kafka/Prozess/Buddenbrooks.epub"), "other epub");

        const QList<Work> works = {
            work(QStringLiteral("a"), { ebook(lib + QStringLiteral("/Mann/Buddenbrooks/Buddenbrooks.epub")),
                                        audiobook({ lib + QStringLiteral("/Mann/Buddenbrooks/01.mp3"),
                                                    lib + QStringLiteral("/Mann/Buddenbrooks/02.mp3") }) }),
            work(QStringLiteral("b"), { audiobook({ lib + QStringLiteral("/Kafka/Verwandlung/01.mp3") }),
                                        ebook(lib + QStringLiteral("/Kafka/Verwandlung/Die Verwandlung.pdf")) }),
            work(QStringLiteral("c"), { ebook(lib + QStringLiteral("/Kafka/Prozess/Buddenbrooks.epub")),
                                        ebook(lib + QStringLiteral("/Kafka/Prozess/fehlt.epub")) }),
            // The same file again (e.g. the same work twice): planned once.
            work(QStringLiteral("d"), { ebook(lib + QStringLiteral("/Mann/Buddenbrooks/Buddenbrooks.epub")) }),
        };
        const QString out = dir.filePath(QStringLiteral("out/calibly-Export"));
        const LibraryExport::Plan plan = LibraryExport::plan(works, out);

        QStringList targets;
        for (const LibraryExport::File &file : plan.files)
            targets.append(QDir(out).relativeFilePath(file.target));
        QCOMPARE(targets, QStringList({ QStringLiteral("Ebooks/Buddenbrooks.epub"), QStringLiteral("Audiobooks/01.mp3"),
                                        QStringLiteral("Audiobooks/02.mp3"), QStringLiteral("Audiobooks/01 (2).mp3"),
                                        QStringLiteral("Ebooks/Die Verwandlung.pdf"),
                                        QStringLiteral("Ebooks/Buddenbrooks (2).epub") }));
        QCOMPARE(plan.missing, QStringList({ lib + QStringLiteral("/Kafka/Prozess/fehlt.epub") }));
        QCOMPARE(plan.totalBytes, qint64(4 + 2 + 2 + 2 + 3 + 10));
        QVERIFY(!QFileInfo::exists(out)); // planning creates nothing
    }

    void run()
    {
        QTemporaryDir dir;
        const QString a = dir.filePath(QStringLiteral("lib/a.epub"));
        const QString b = dir.filePath(QStringLiteral("lib/b.m4b"));
        const QByteArray big(3 * 1024 * 1024 + 17, 'x'); // more than one piece
        writeFile(a, "ebook");
        writeFile(b, big);
        const QString out = dir.filePath(QStringLiteral("out/calibly-Export"));
        const LibraryExport::Plan plan
            = LibraryExport::plan({ work(QStringLiteral("w"), { ebook(a), audiobook({ b }) }) }, out);

        qint64 lastBytes = 0;
        int calls = 0;
        std::atomic_bool cancel = false;
        const LibraryExport::Outcome outcome = LibraryExport::run(
            plan, [&](qint64 bytes, int, const QString &) { lastBytes = bytes; ++calls; }, cancel);
        QCOMPARE(outcome.result, LibraryExport::Result::Done);
        QCOMPARE(outcome.copied, 2);
        QCOMPARE(lastBytes, plan.totalBytes);
        QVERIFY(calls > 2);
        QCOMPARE(names(out), QStringList({ QStringLiteral("Audiobooks"), QStringLiteral("Ebooks") }));
        QCOMPARE(names(out + QStringLiteral("/Ebooks")), QStringList({ QStringLiteral("a.epub") }));
        QCOMPARE(names(out + QStringLiteral("/Audiobooks")), QStringList({ QStringLiteral("b.m4b") }));
        QCOMPARE(readFile(out + QStringLiteral("/Audiobooks/b.m4b")), big);
        QCOMPARE(readFile(a), QByteArray("ebook")); // the library is untouched

        // The folder exists now: a second run into it is refused.
        QCOMPARE(LibraryExport::run(plan, {}, cancel).result, LibraryExport::Result::CannotCreateFolder);
    }

    // Cancelling removes the new folder; files of the library stay.
    void cancel()
    {
        QTemporaryDir dir;
        const QString a = dir.filePath(QStringLiteral("lib/a.m4b"));
        writeFile(a, QByteArray(3 * 1024 * 1024, 'y'));
        const QString out = dir.filePath(QStringLiteral("out/Export"));
        const LibraryExport::Plan plan = LibraryExport::plan({ work(QStringLiteral("w"), { audiobook({ a }) }) }, out);
        std::atomic_bool cancel = false;
        const LibraryExport::Outcome outcome
            = LibraryExport::run(plan, [&](qint64, int, const QString &) { cancel = true; }, cancel);
        QCOMPARE(outcome.result, LibraryExport::Result::Cancelled);
        QVERIFY(!QFileInfo::exists(out));
        QVERIFY(QFileInfo::exists(a));
    }

    // The controller: errors before starting, the export in its thread,
    // the remembered folder.
    void controller()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_export"));
        QSettings().clear();

        QTemporaryDir dir;
        const QString a = dir.filePath(QStringLiteral("lib/Faust.epub"));
        const QString b = dir.filePath(QStringLiteral("lib/Faust.mp3"));
        writeFile(a, "faust");
        writeFile(b, "faust audio");
        MemoryBackend backend;
        backend.works = { work(QStringLiteral("faust"), { ebook(a), audiobook({ b }) }),
                          work(QStringLiteral("gone"), { ebook(dir.filePath(QStringLiteral("lib/fehlt.epub"))) }) };
        LibraryController library(backend, nullptr);
        ExportController exporter(library, nullptr);
        const QString target = dir.filePath(QStringLiteral("target"));
        QVERIFY(QDir().mkpath(target));

        // Nothing to export, or no such folder: Failed, nothing created.
        QVERIFY(!exporter.exportWorksTo({ QStringLiteral("gone") }, target));
        QCOMPARE(exporter.state(), ExportController::Failed);
        QVERIFY(!exporter.errorText().isEmpty());
        QVERIFY(names(target).isEmpty());
        QVERIFY(!exporter.exportWorksTo({ QStringLiteral("faust") }, dir.filePath(QStringLiteral("nirgends"))));
        QCOMPARE(exporter.state(), ExportController::Failed);
        exporter.dismiss();
        QCOMPARE(exporter.state(), ExportController::Idle);

        QSignalSpy changed(&exporter, &ExportController::changed);
        QVERIFY(exporter.exportWorksTo({ QStringLiteral("faust"), QStringLiteral("gone") }, target));
        QVERIFY(exporter.running());
        QVERIFY(!exporter.exportWorksTo({ QStringLiteral("faust") }, target)); // one at a time
        QTRY_COMPARE(exporter.state(), ExportController::Done);
        QCOMPARE(exporter.fileCount(), 2);
        QCOMPARE(exporter.copiedCount(), 2);
        QCOMPARE(exporter.missingCount(), 1);
        QCOMPARE(exporter.progress(), 1.0);
        QCOMPARE(exporter.folderName(), LibraryExport::folderName(QDate::currentDate()));
        QCOMPARE(readFile(exporter.folder() + QStringLiteral("/Ebooks/Faust.epub")), QByteArray("faust"));
        QCOMPARE(readFile(exporter.folder() + QStringLiteral("/Audiobooks/Faust.mp3")), QByteArray("faust audio"));
        QCOMPARE(exporter.lastFolder(), QUrl::fromLocalFile(QDir::cleanPath(target)));

        // A second export the same day gets its own folder.
        QVERIFY(exporter.exportWorksTo({ QStringLiteral("faust") }, target));
        QTRY_COMPARE(exporter.state(), ExportController::Done);
        QCOMPARE(exporter.folderName(), LibraryExport::folderName(QDate::currentDate()) + QStringLiteral(" (2)"));
        QCOMPARE(names(target).size(), 2);

        QSettings().clear();
    }
};

QTEST_MAIN(TestExport)
#include "tst_export.moc"
