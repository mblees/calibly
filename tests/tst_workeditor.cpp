#include <QSignalSpy>
#include <QTest>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
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

private:
    std::unique_ptr<RecordingBackend> m_backend;
    std::unique_ptr<LibraryController> m_controller;
};

QTEST_GUILESS_MAIN(TestWorkEditor)
#include "tst_workeditor.moc"
