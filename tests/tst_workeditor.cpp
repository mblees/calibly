// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "libraryfolder.h"
#include "workdetails.h"
#include "workeditor.h"
#include "workmodel.h"

namespace {

Format makeFormat(Format::Kind kind, const QString &file, const QString &publisher)
{
    Format format;
    format.kind = kind;
    format.filePath = file;
    format.publisher = publisher;
    return format;
}

Work makeWork()
{
    Work work;
    work.id = QStringLiteral("dracula");
    work.title = QStringLiteral("Dracula");
    work.authors = { QStringLiteral("Bram Stoker") };
    work.narrators = { QStringLiteral("Maren Brandt") };
    work.tags = { QStringLiteral("Horror") };
    work.language = QStringLiteral("de");
    work.publishedYear = 1897;
    work.rating = 4;
    work.formats = {
        makeFormat(Format::Kind::Ebook, QStringLiteral("dracula.epub"), QStringLiteral("Verlag A")),
        makeFormat(Format::Kind::Ebook, QStringLiteral("dracula.pdf"), QStringLiteral("Verlag B")),
        makeFormat(Format::Kind::Audiobook, QStringLiteral("dracula.m4b"), QStringLiteral("Audio")),
    };
    return work;
}

// Remembers what was saved.
class RecordingBackend : public LibraryBackend
{
public:
    QList<Work> works;
    bool failSaving = false;
    int saveCount = 0;

    QList<Work> loadWorks() override { return works; }
    bool saveWork(const Work &work) override
    {
        if (failSaving)
            return false;
        ++saveCount;
        for (Work &existing : works) {
            if (existing.id == work.id)
                existing = work;
        }
        return true;
    }
};

} // namespace

class TestWorkEditor : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_backend = std::make_unique<RecordingBackend>();
        m_backend->works = { makeWork() };
        // WorkEditor finds the library through LibraryController::instance().
        m_controller = std::make_unique<LibraryController>(*m_backend, nullptr);
    }

    void cleanup()
    {
        m_controller.reset();
        m_backend.reset();
    }

    void loadsWork()
    {
        WorkEditor editor;
        QVERIFY(!editor.valid());
        editor.setWorkId(QStringLiteral("dracula"));
        QVERIFY(editor.valid());
        QVERIFY(!editor.modified());
        QVERIFY(!editor.canSave());
        QCOMPARE(editor.title(), QStringLiteral("Dracula"));
        QCOMPARE(editor.authors(), QStringLiteral("Bram Stoker"));
        QCOMPARE(editor.narrators(), QStringLiteral("Maren Brandt"));
        QCOMPARE(editor.publishedYear(), 1897);
        QCOMPARE(editor.ebookPublisher(), QStringLiteral("Verlag A")); // first ebook format
        QCOMPARE(editor.audiobookPublisher(), QStringLiteral("Audio"));
        QVERIFY(editor.hasEbook());
        QVERIFY(editor.hasAudiobook());

        editor.setWorkId(QStringLiteral("missing"));
        QVERIFY(!editor.valid());
        QCOMPARE(editor.title(), QString());
    }

    void tracksModification()
    {
        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        QSignalSpy modified(&editor, &WorkEditor::modifiedChanged);

        editor.setTitle(QStringLiteral("Dracula "));   // only whitespace: same value when saved
        QVERIFY(!editor.modified());
        editor.setAuthors(QStringLiteral("Bram Stoker ,")); // same names
        QVERIFY(!editor.modified());
        QCOMPARE(modified.count(), 0);

        editor.setTitle(QStringLiteral("Dracula – Roman"));
        QVERIFY(editor.modified());
        QVERIFY(editor.canSave());
        editor.setTitle(QStringLiteral("Dracula"));
        QVERIFY(!editor.modified());
        QCOMPARE(modified.count(), 2);

        editor.setTitle(QStringLiteral("   "));
        QVERIFY(editor.modified());
        QVERIFY(!editor.canSave()); // a title is required
        QVERIFY(!editor.save());

        editor.revert();
        QCOMPARE(editor.title(), QStringLiteral("Dracula"));
        QVERIFY(!editor.modified());
    }

    void normalizesValues()
    {
        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setTags({ QStringLiteral(" Klassiker "), QStringLiteral("horror"), QString(), QStringLiteral("Horror") });
        QCOMPARE(editor.tags(), (QStringList{ QStringLiteral("Klassiker"), QStringLiteral("horror") }));
        editor.setRating(9);
        QCOMPARE(editor.rating(), 5);
        editor.setPublishedYear(-3);
        QCOMPARE(editor.publishedYear(), 0);
    }

    void savesIntoLibrary()
    {
        WorkModel *works = m_controller->works();
        WorkDetails details;
        details.setModel(works);
        details.setWorkId(QStringLiteral("dracula"));

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setTitle(QStringLiteral("  Dracula. Roman "));
        editor.setAuthors(QStringLiteral("Bram Stoker; Jemand & Anderer"));
        editor.setSeries(QStringLiteral("Klassiker des Grauens"));
        editor.setSeriesIndex(2);
        editor.setEbookIsbn(QStringLiteral("978-3-16-148410-0"));
        editor.setDescription(QStringLiteral("Ein Vampirroman.\n"));

        // Meanwhile the progress changed (e.g. the book was opened): it is kept.
        Work opened = *works->workAt(0);
        opened.formats[2].progress = 0.5;
        works->updateWork(opened);

        QSignalSpy detailsChanged(&details, &WorkDetails::changed);
        QVERIFY(editor.save());
        QVERIFY(!editor.modified());
        QCOMPARE(m_backend->saveCount, 1);
        QVERIFY(detailsChanged.count() > 0);

        const Work &saved = m_backend->works.first();
        QCOMPARE(saved.title, QStringLiteral("Dracula. Roman"));
        QCOMPARE(saved.authors, (QStringList{ QStringLiteral("Bram Stoker"), QStringLiteral("Jemand"),
                                              QStringLiteral("Anderer") }));
        QCOMPARE(saved.series, QStringLiteral("Klassiker des Grauens"));
        QCOMPARE(saved.seriesIndex, 2);
        QCOMPARE(saved.description, QStringLiteral("Ein Vampirroman."));
        QCOMPARE(saved.formats.at(2).progress, 0.5);
        // A changed format field goes to every format of that kind …
        QCOMPARE(saved.formats.at(0).isbn, QStringLiteral("978-3-16-148410-0"));
        QCOMPARE(saved.formats.at(1).isbn, QStringLiteral("978-3-16-148410-0"));
        QCOMPARE(saved.formats.at(2).isbn, QString());
        // … unchanged ones keep their own values.
        QCOMPARE(saved.formats.at(1).publisher, QStringLiteral("Verlag B"));

        // The model and so the gallery and the detail view show the change.
        QCOMPARE(works->index(0).data(WorkModel::TitleRole).toString(), QStringLiteral("Dracula. Roman"));
        QCOMPARE(details.title(), QStringLiteral("Dracula. Roman"));
        QCOMPARE(details.isbn(), QStringLiteral("978-3-16-148410-0"));

        // Saving without changes stores nothing.
        QVERIFY(editor.save());
        QCOMPARE(m_backend->saveCount, 1);
    }

    void seriesIndexNeedsSeries()
    {
        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setSeriesIndex(3);
        QVERIFY(editor.save());
        QCOMPARE(m_backend->works.first().seriesIndex, 0);
    }

    void keepsChangesWhenSavingFails()
    {
        m_backend->failSaving = true;
        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setTitle(QStringLiteral("Nosferatu"));
        QVERIFY(!editor.save());
        QVERIFY(editor.modified());
        QCOMPARE(editor.title(), QStringLiteral("Nosferatu"));
        QCOMPARE(m_controller->works()->workAt(0)->title, QStringLiteral("Dracula"));
    }

    void languagesIncludeOwn()
    {
        Work work = makeWork();
        work.language = QStringLiteral("fi");
        m_backend->works = { work };
        m_controller->reload();

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        QStringList codes;
        for (const QVariant &entry : editor.languages())
            codes.append(entry.toMap().value(QStringLiteral("code")).toString());
        QVERIFY(codes.contains(QStringLiteral("de")));
        QVERIFY(codes.contains(QStringLiteral("fi")));
        QVERIFY(!codes.contains(QString()));
    }

    // --- covers (phase 14) ---------------------------------------------------

    void newCoverNames()
    {
        QTemporaryDir dir;
        const QString book = dir.filePath(QStringLiteral("book.epub"));
        const QImage image = coloredImage(120, 180, Qt::red);

        const QString first = LibraryFolder::saveNewCover(image, book, QString());
        QCOMPARE(first, book + QStringLiteral(".cover.jpg"));
        const QString second = LibraryFolder::saveNewCover(image, book, first);
        QCOMPARE(second, book + QStringLiteral(".cover-2.jpg"));
        // Counts on from the old one, even when an older name is free again.
        QFile::remove(first);
        QCOMPARE(LibraryFolder::saveNewCover(image, book, second), book + QStringLiteral(".cover-3.jpg"));
        // A name that is taken is skipped.
        QFile(book + QStringLiteral(".cover-5.jpg")).open(QIODevice::WriteOnly);
        QCOMPARE(LibraryFolder::saveNewCover(image, book, book + QStringLiteral(".cover-4.jpg")),
                 book + QStringLiteral(".cover-6.jpg"));
        QVERIFY(LibraryFolder::saveNewCover(QImage(), book, QString()).isEmpty());

        QVERIFY(LibraryFolder::isOwnCover(first, book));
        QVERIFY(LibraryFolder::isOwnCover(second, book));
        QVERIFY(!LibraryFolder::isOwnCover(dir.filePath(QStringLiteral("other.jpg")), book));
        QVERIFY(!LibraryFolder::isOwnCover(book + QStringLiteral(".cover-x.jpg"), book));
        QVERIFY(!LibraryFolder::isOwnCover(QStringLiteral(":/mock/covers/a.jpg"), book));
        QVERIFY(!LibraryFolder::isOwnCover(QString(), book));
    }

    void savesCoverForAllFormats()
    {
        QTemporaryDir dir;
        Work work = coverWork(dir);
        const QString oldCover = work.formats[0].filePath + QStringLiteral(".cover.jpg");
        QVERIFY(coloredImage(100, 150, Qt::blue).save(oldCover));
        work.formats[0].coverPath = oldCover;
        m_backend->works = { work };
        m_controller->reload();

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        QCOMPARE(editor.ebookCoverUrl(), QUrl::fromLocalFile(oldCover));
        QVERIFY(editor.audiobookCoverUrl().isEmpty());
        QVERIFY(editor.audiobookCoverIsSquare()); // placeholder of an audiobook

        QSignalSpy changed(&editor, &WorkEditor::coverChanged);
        editor.setCover(WorkEditor::AllFormats, coloredImage(200, 300, Qt::red));
        QCOMPARE(changed.count(), 1);
        QVERIFY(editor.coverModified());
        QVERIFY(editor.modified());
        QVERIFY(editor.canSave());
        // The preview is a file of its own, the stored cover is unchanged.
        QVERIFY(editor.ebookCoverUrl().isLocalFile());
        QVERIFY(QFileInfo::exists(editor.ebookCoverUrl().toLocalFile()));
        QVERIFY(editor.ebookCoverUrl() != QUrl::fromLocalFile(oldCover));
        QVERIFY(!editor.audiobookCoverIsSquare());
        QCOMPARE(m_backend->saveCount, 0);

        QVERIFY(editor.save());
        QVERIFY(!editor.modified());
        QVERIFY(!editor.coverModified());
        const Work &saved = m_backend->works.first();
        const QString epub = saved.formats[0].filePath;
        QCOMPARE(saved.formats[0].coverPath, epub + QStringLiteral(".cover-2.jpg"));
        QVERIFY(QFileInfo::exists(saved.formats[0].coverPath));
        QVERIFY(!QFileInfo::exists(oldCover)); // replaced: deleted
        QCOMPARE(saved.formats[1].coverPath, saved.formats[1].filePath + QStringLiteral(".cover.jpg"));
        QCOMPARE(saved.formats[2].coverPath, saved.formats[2].filePath + QStringLiteral(".cover.jpg"));
        QVERIFY(!saved.formats[2].coverIsSquare);
        QCOMPARE(QImage(saved.formats[2].coverPath).size(), QSize(200, 300));
        // The editor shows what is stored now.
        QCOMPARE(editor.ebookCoverUrl(), QUrl::fromLocalFile(saved.formats[0].coverPath));
    }

    void changesCoverOfOneFormat()
    {
        QTemporaryDir dir;
        Work work = coverWork(dir);
        const QString ebookCover = dir.filePath(QStringLiteral("ebook.jpg"));
        QVERIFY(coloredImage(100, 150, Qt::blue).save(ebookCover));
        work.formats[0].coverPath = ebookCover;
        m_backend->works = { work };
        m_controller->reload();

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        QVERIFY(editor.canUseOtherCover(WorkEditor::AudiobookCover)); // the ebook has one
        QVERIFY(!editor.canUseOtherCover(WorkEditor::EbookCover));    // the audiobook has none
        QVERIFY(!editor.canUseOtherCover(WorkEditor::AllFormats));

        editor.setCover(WorkEditor::AudiobookCover, coloredImage(300, 300, Qt::green));
        QVERIFY(editor.audiobookCoverIsSquare());
        QCOMPARE(editor.ebookCoverUrl(), QUrl::fromLocalFile(ebookCover)); // untouched
        QVERIFY(editor.canUseOtherCover(WorkEditor::EbookCover));          // the new one counts

        QVERIFY(editor.useOtherCover(WorkEditor::EbookCover));
        QVERIFY(editor.ebookCoverIsSquare());
        editor.removeCover(WorkEditor::AudiobookCover);
        QVERIFY(editor.audiobookCoverUrl().isEmpty());
        QVERIFY(editor.audiobookCoverIsSquare());

        editor.revert();
        QVERIFY(!editor.coverModified());
        QVERIFY(!editor.modified());
        QCOMPARE(editor.ebookCoverUrl(), QUrl::fromLocalFile(ebookCover));

        // Removing a cover the user brought in keeps that file.
        editor.removeCover(WorkEditor::EbookCover);
        QVERIFY(editor.save());
        QVERIFY(m_backend->works.first().formats[0].coverPath.isEmpty());
        QVERIFY(m_backend->works.first().formats[1].coverPath.isEmpty());
        QVERIFY(QFileInfo::exists(ebookCover));
    }

    void combinedWorkChangesFirstVolume()
    {
        QTemporaryDir dir;
        Work work = coverWork(dir);
        work.formats.removeLast(); // ebooks only: two volumes
        WorkPart first;
        first.id = QStringLiteral("dracula");
        first.files = { work.formats[0].filePath };
        WorkPart second;
        second.id = QStringLiteral("dracula-2");
        second.files = { work.formats[1].filePath };
        work.parts = { first, second };
        m_backend->works = { work };
        m_controller->reload();

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setCover(WorkEditor::EbookCover, coloredImage(100, 150, Qt::red));
        QVERIFY(editor.save());
        QVERIFY(!m_backend->works.first().formats[0].coverPath.isEmpty());
        QVERIFY(m_backend->works.first().formats[1].coverPath.isEmpty());
    }

    void coverSources()
    {
        QTemporaryDir dir;
        m_backend->works = { coverWork(dir) };
        m_controller->reload();
        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));

        const QString picture = dir.filePath(QStringLiteral("picture.png"));
        QVERIFY(coloredImage(60, 90, Qt::yellow).save(picture));
        const QString text = dir.filePath(QStringLiteral("notes.txt"));
        QFile file(text);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("no picture");
        file.close();

        QVERIFY(!editor.setCoverFromUrl(WorkEditor::EbookCover, QUrl::fromLocalFile(text)));
        QVERIFY(!editor.setCoverFromUrl(WorkEditor::EbookCover, QUrl(QStringLiteral("https://example.org/a.jpg"))));
        QVERIFY(!editor.coverModified());
        QVERIFY(editor.setCoverFromUrl(WorkEditor::EbookCover, QUrl::fromLocalFile(picture)));
        QVERIFY(editor.coverModified());
        QVERIFY(editor.audiobookCoverUrl().isEmpty());

        // The format files of this work do not exist: nothing to read.
        editor.revert();
        QVERIFY(!editor.coverFromFile(WorkEditor::AllFormats));
        QVERIFY(!editor.coverModified());
    }

    void coverThatCannotBeSaved()
    {
        Work work = makeWork(); // its files are in no folder that exists
        for (Format &format : work.formats)
            format.filePath = QStringLiteral("Z:/does/not/exist/") + format.filePath;
        m_backend->works = { work };
        m_controller->reload();

        WorkEditor editor;
        editor.setWorkId(QStringLiteral("dracula"));
        editor.setCover(WorkEditor::AllFormats, coloredImage(100, 150, Qt::red));
        QVERIFY(!editor.save());
        QCOMPARE(m_backend->saveCount, 0);
        QVERIFY(editor.coverModified()); // kept for another try
    }

private:
    static QImage coloredImage(int width, int height, Qt::GlobalColor color)
    {
        QImage image(width, height, QImage::Format_RGB32);
        image.fill(color);
        return image;
    }

    // makeWork() with its format files in dir.
    static Work coverWork(const QTemporaryDir &dir)
    {
        Work work = makeWork();
        for (Format &format : work.formats)
            format.filePath = dir.filePath(format.filePath);
        return work;
    }

    std::unique_ptr<RecordingBackend> m_backend;
    std::unique_ptr<LibraryController> m_controller;
};

QTEST_GUILESS_MAIN(TestWorkEditor)
#include "tst_workeditor.moc"
