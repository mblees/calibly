// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QSignalSpy>
#include <QTest>

#include "textutils.h"
#include "workfiltermodel.h"
#include "workmodel.h"

namespace {

Work makeWork(const QString &title, const QStringList &authors, bool ebook, bool audiobook,
              const QString &lastOpened = QString(), const QStringList &narrators = {})
{
    Work work;
    work.id = title;
    work.title = title;
    work.authors = authors;
    work.narrators = narrators;
    if (ebook)
        work.formats.append({ Format::Kind::Ebook, QStringLiteral("book.epub"), 0.0 });
    if (audiobook)
        work.formats.append({ Format::Kind::Audiobook, QStringLiteral("book.m4b"), 0.0 });
    work.lastOpened = QDateTime::fromString(lastOpened, Qt::ISODate);
    return work;
}

Work seriesWork(const QString &title, const QString &series, int index, bool audiobook = false,
                const QString &lastOpened = QString())
{
    Work work = makeWork(title, { QStringLiteral("Arthur Conan Doyle") }, true, audiobook, lastOpened);
    work.series = series;
    work.seriesIndex = index;
    return work;
}

QStringList titles(const QAbstractItemModel &model)
{
    QStringList result;
    for (int row = 0; row < model.rowCount(); ++row)
        result.append(model.index(row, 0).data(WorkModel::TitleRole).toString());
    return result;
}

} // namespace

class TestWorkFilterModel : public QObject
{
    Q_OBJECT

private slots:
    void rowLookup()
    {
        WorkModel source;
        source.setWorks({ makeWork(QStringLiteral("Woyzeck"), { QStringLiteral("Georg Büchner") }, true, false),
                          makeWork(QStringLiteral("Dracula"), { QStringLiteral("Bram Stoker") }, false, true),
                          makeWork(QStringLiteral("Faust"), { QStringLiteral("Goethe") }, true, false) });
        WorkFilterModel model;
        model.setSourceModel(&source);
        model.setSortOrder(WorkFilterModel::Title);
        QCOMPARE(model.workIdAt(0), QStringLiteral("Dracula"));
        QCOMPARE(model.rowOf(QStringLiteral("Woyzeck")), 2);
        QCOMPARE(model.workIdAt(3), QString());
        QCOMPARE(model.workIdAt(-1), QString());

        model.setFormatFilter(WorkFilterModel::Ebook);
        QCOMPARE(model.rowOf(QStringLiteral("Dracula")), -1);
        QCOMPARE(model.rowOf(QStringLiteral("Woyzeck")), 1);
    }

    void init()
    {
        m_works.setWorks({
            makeWork(QStringLiteral("Moby Dick"), { QStringLiteral("Herman Melville") }, true, true,
                     QStringLiteral("2026-11-11T08:00:00"), { QStringLiteral("Andreas Lindner") }),
            makeWork(QStringLiteral("Dracula"), { QStringLiteral("Bram Stoker") }, false, true,
                     QStringLiteral("2026-11-09T22:00:00")),
            makeWork(QStringLiteral("Faust I"), { QStringLiteral("Johann Wolfgang von Goethe") }, true, false,
                     QStringLiteral("2026-11-10T19:00:00")),
            makeWork(QStringLiteral("Grimms Märchen"), { QStringLiteral("Jacob Grimm") }, true, true),
            makeWork(QStringLiteral("Die Verwandlung"), { QStringLiteral("Franz Kafka") }, true, false),
            makeWork(QStringLiteral("Der Prozess"), { QStringLiteral("Franz Kafka") }, true, true),
        });
        m_filter.setSourceModel(&m_works);
        m_filter.setFormatFilter(WorkFilterModel::AllFormats);
        m_filter.setSearchText(QString());
        m_filter.setGroupSeries(false);
        m_filter.setSeriesFilter(QString());
        m_filter.setSeriesOnly(false);
        m_filter.setOnlyIds({});
        m_filter.clearSelection();
        m_filter.setSortOrder(WorkFilterModel::LastOpened);
    }

    void foldForSearch()
    {
        QCOMPARE(TextUtils::foldForSearch(QStringLiteral("Märchen")), QStringLiteral("marchen"));
        QCOMPARE(TextUtils::foldForSearch(QStringLiteral("BÜCHNER")), QStringLiteral("buchner"));
        QCOMPARE(TextUtils::foldForSearch(QStringLiteral("Straße")), QStringLiteral("strasse"));
        QCOMPARE(TextUtils::searchTerms(QStringLiteral("  Franz   Kafka ")),
                 (QStringList{ QStringLiteral("franz"), QStringLiteral("kafka") }));
    }

    void languageName()
    {
        QCOMPARE(TextUtils::languageName(QStringLiteral("de")), QStringLiteral("Deutsch"));
        QCOMPARE(TextUtils::languageName(QStringLiteral("en")), QStringLiteral("English"));
        QCOMPARE(TextUtils::languageName(QString()), QString());
    }

    void nameSortKey()
    {
        QCOMPARE(TextUtils::nameSortKey(QStringLiteral("Johann Wolfgang von Goethe")), QStringLiteral("Goethe"));
        QCOMPARE(TextUtils::nameSortKey(QStringLiteral("Homer")), QStringLiteral("Homer"));
        QCOMPARE(TextUtils::nameSortKey(QString()), QString());
    }

    void formatFilter_data()
    {
        QTest::addColumn<WorkFilterModel::FormatFilter>("filter");
        QTest::addColumn<int>("expected");
        QTest::newRow("all") << WorkFilterModel::AllFormats << 6;
        QTest::newRow("ebook") << WorkFilterModel::Ebook << 5;
        QTest::newRow("audiobook") << WorkFilterModel::Audiobook << 4;
        QTest::newRow("both") << WorkFilterModel::BothFormats << 3;
    }

    void formatFilter()
    {
        QFETCH(WorkFilterModel::FormatFilter, filter);
        QFETCH(int, expected);
        m_filter.setFormatFilter(filter);
        QCOMPARE(m_filter.count(), expected);
    }

    void search()
    {
        m_filter.setSearchText(QStringLiteral("kafka"));
        QCOMPARE(m_filter.count(), 2);

        // Every word must match, in any field.
        m_filter.setSearchText(QStringLiteral("kafka prozess"));
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Der Prozess") });

        // Accents and case are ignored.
        m_filter.setSearchText(QStringLiteral("MARCHEN"));
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Grimms Märchen") });

        // Narrators are searched too.
        m_filter.setSearchText(QStringLiteral("lindner"));
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Moby Dick") });

        m_filter.setSearchText(QStringLiteral("nothing like this"));
        QCOMPARE(m_filter.count(), 0);
    }

    void searchAndFormatCombine()
    {
        m_filter.setSearchText(QStringLiteral("kafka"));
        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Der Prozess") });
    }

    void formatCountsFollowSearch()
    {
        QCOMPARE(m_filter.countAll(), 6);
        QCOMPARE(m_filter.countEbook(), 5);
        QCOMPARE(m_filter.countAudiobook(), 4);
        QCOMPARE(m_filter.countBoth(), 3);

        QSignalSpy spy(&m_filter, &WorkFilterModel::formatCountsChanged);
        m_filter.setSearchText(QStringLiteral("kafka"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m_filter.countAll(), 2);
        QCOMPARE(m_filter.countEbook(), 2);
        QCOMPARE(m_filter.countAudiobook(), 1);
        QCOMPARE(m_filter.countBoth(), 1);

        // The format filter itself does not change the counts.
        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(m_filter.countAll(), 2);
    }

    void sortLastOpened()
    {
        // Opened works first (newest first), then never opened ones by title.
        QCOMPARE(titles(m_filter), (QStringList{
                                       QStringLiteral("Moby Dick"), QStringLiteral("Faust I"),
                                       QStringLiteral("Dracula"), QStringLiteral("Der Prozess"),
                                       QStringLiteral("Die Verwandlung"), QStringLiteral("Grimms Märchen") }));
    }

    void sortTitle()
    {
        m_filter.setSortOrder(WorkFilterModel::Title);
        QCOMPARE(titles(m_filter), (QStringList{
                                       QStringLiteral("Der Prozess"), QStringLiteral("Die Verwandlung"),
                                       QStringLiteral("Dracula"), QStringLiteral("Faust I"),
                                       QStringLiteral("Grimms Märchen"), QStringLiteral("Moby Dick") }));
    }

    void sortAuthor()
    {
        m_filter.setSortOrder(WorkFilterModel::Author);
        // Goethe, Grimm, Kafka (2, by title), Melville, Stoker
        QCOMPARE(titles(m_filter), (QStringList{
                                       QStringLiteral("Faust I"), QStringLiteral("Grimms Märchen"),
                                       QStringLiteral("Der Prozess"), QStringLiteral("Die Verwandlung"),
                                       QStringLiteral("Moby Dick"), QStringLiteral("Dracula") }));
    }

    void singleFormatPresentation()
    {
        // Moby Dick has both formats and progress in both.
        auto mobyDick = [this]() -> QModelIndex {
            for (int row = 0; row < m_filter.rowCount(); ++row) {
                const QModelIndex index = m_filter.index(row, 0);
                if (index.data(WorkModel::TitleRole).toString() == QLatin1String("Moby Dick"))
                    return index;
            }
            return {};
        };
        m_works.setWorks({ [] {
            Work work = makeWork(QStringLiteral("Moby Dick"), { QStringLiteral("Herman Melville") }, true, true);
            work.formats[0].progress = 0.31;
            work.formats[1].progress = 0.58;
            return work;
        }() });

        QCOMPARE(mobyDick().data(WorkModel::HasEbookRole).toBool(), true);
        QCOMPARE(mobyDick().data(WorkModel::HasAudiobookRole).toBool(), true);

        QSignalSpy spy(&m_filter, &QAbstractItemModel::dataChanged);
        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QVERIFY(spy.count() > 0);
        QCOMPARE(mobyDick().data(WorkModel::HasEbookRole).toBool(), false);
        QCOMPARE(mobyDick().data(WorkModel::ReadProgressRole).toDouble(), 0.0);
        QCOMPARE(mobyDick().data(WorkModel::HasAudiobookRole).toBool(), true);
        QCOMPARE(mobyDick().data(WorkModel::ListenProgressRole).toDouble(), 0.58);

        m_filter.setFormatFilter(WorkFilterModel::Ebook);
        QCOMPARE(mobyDick().data(WorkModel::HasEbookRole).toBool(), true);
        QCOMPARE(mobyDick().data(WorkModel::ReadProgressRole).toDouble(), 0.31);
        QCOMPARE(mobyDick().data(WorkModel::HasAudiobookRole).toBool(), false);
        QCOMPARE(mobyDick().data(WorkModel::ListenProgressRole).toDouble(), 0.0);

        m_filter.setFormatFilter(WorkFilterModel::BothFormats);
        QCOMPARE(mobyDick().data(WorkModel::HasEbookRole).toBool(), true);
        QCOMPARE(mobyDick().data(WorkModel::HasAudiobookRole).toBool(), true);

        // The counts of the chips still count works with both formats.
        QCOMPARE(m_filter.countEbook(), 1);
        QCOMPARE(m_filter.countAudiobook(), 1);
    }

    void coverFollowsFormat()
    {
        Work withCovers = makeWork(QStringLiteral("Moby Dick"), { QStringLiteral("Herman Melville") }, true, true);
        withCovers.formats[0].coverPath = QStringLiteral(":/mock/covers/book.jpg");
        withCovers.formats[1].coverPath = QStringLiteral(":/mock/covers/audio.jpg");
        withCovers.formats[1].coverIsSquare = true;
        Work withoutCovers = makeWork(QStringLiteral("Der Prozess"), { QStringLiteral("Franz Kafka") }, true, true);
        m_works.setWorks({ withCovers, withoutCovers });
        m_filter.setSortOrder(WorkFilterModel::Title); // Der Prozess, Moby Dick

        const auto cover = [this](int row) { return m_filter.index(row, 0).data(WorkModel::CoverUrlRole).toUrl(); };
        const auto square = [this](int row) { return m_filter.index(row, 0).data(WorkModel::CoverIsSquareRole).toBool(); };
        const QUrl book(QStringLiteral("qrc:/mock/covers/book.jpg"));
        const QUrl audio(QStringLiteral("qrc:/mock/covers/audio.jpg"));

        // No format selected: the ebook represents the work.
        QCOMPARE(cover(1), book);
        QCOMPARE(square(1), false);
        QCOMPARE(cover(0), QUrl());
        QCOMPARE(square(0), false);

        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(cover(1), audio);
        QCOMPARE(square(1), true);
        QCOMPARE(cover(0), QUrl()); // no audiobook cover: square placeholder
        QCOMPARE(square(0), true);

        m_filter.setFormatFilter(WorkFilterModel::Ebook);
        QCOMPARE(cover(1), book);
        QCOMPARE(square(1), false);
        QCOMPARE(square(0), false);

        m_filter.setFormatFilter(WorkFilterModel::BothFormats);
        QCOMPARE(cover(1), book);
    }

    void countSignal()
    {
        QSignalSpy spy(&m_filter, &WorkFilterModel::countChanged);
        m_filter.setFormatFilter(WorkFilterModel::BothFormats);
        QVERIFY(spy.count() > 0);
        QCOMPARE(m_filter.count(), 3);
    }

    void sourceReset()
    {
        m_works.setWorks({ makeWork(QStringLiteral("Woyzeck"), { QStringLiteral("Georg Büchner") }, true, false) });
        QCOMPARE(m_filter.count(), 1);
        QCOMPARE(m_filter.countAll(), 1);
        QCOMPARE(m_filter.countAudiobook(), 0);
    }

    // Phase 9: shown works can be selected; what is no longer shown leaves
    // the selection.
    void selection()
    {
        m_filter.setSortOrder(WorkFilterModel::Title);
        QSignalSpy changed(&m_filter, &WorkFilterModel::selectionChanged);
        QVERIFY(m_filter.roleNames().values().contains("selected"));
        QVERIFY(!m_filter.canMergeSelection());

        m_filter.toggleSelected(QStringLiteral("Faust I"));
        m_filter.toggleSelected(QStringLiteral("Dracula"));
        m_filter.toggleSelected(QStringLiteral("unknown"));
        QCOMPARE(changed.count(), 2);
        QCOMPARE(m_filter.selectedCount(), 2);
        // In the shown order, not in the order of the clicks.
        QCOMPARE(m_filter.selectedIds(), (QStringList{ QStringLiteral("Dracula"), QStringLiteral("Faust I") }));
        const int dracula = m_filter.rowOf(QStringLiteral("Dracula"));
        QVERIFY(m_filter.index(dracula, 0).data(WorkFilterModel::SelectedRole).toBool());
        QVERIFY(!m_filter.index(m_filter.rowOf(QStringLiteral("Moby Dick")), 0).data(WorkFilterModel::SelectedRole).toBool());
        QVERIFY(m_filter.canMergeSelection());

        m_filter.toggleSelected(QStringLiteral("Dracula"));
        QCOMPARE(m_filter.selectedIds(), QStringList{ QStringLiteral("Faust I") });
        QVERIFY(!m_filter.canMergeSelection()); // one work with one file

        // "Faust I" has no audiobook: hidden, so no longer selected.
        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(m_filter.selectedCount(), 0);

        m_filter.selectAll();
        QCOMPARE(m_filter.selectedCount(), m_filter.count());
        m_filter.setSearchText(QStringLiteral("kafka"));
        QCOMPARE(m_filter.selectedIds(), QStringList{ QStringLiteral("Der Prozess") });
        m_filter.clearSelection();
        QCOMPARE(m_filter.selectedCount(), 0);
        QVERIFY(!m_filter.index(0, 0).data(WorkFilterModel::SelectedRole).toBool());
    }

    void selectionFollowsTheLibrary()
    {
        m_filter.toggleSelected(QStringLiteral("Dracula"));
        m_filter.toggleSelected(QStringLiteral("Moby Dick"));
        QVERIFY(m_works.removeWork(QStringLiteral("Dracula")));
        QCOMPARE(m_filter.selectedIds(), QStringList{ QStringLiteral("Moby Dick") });
        QVERIFY(!m_works.removeWork(QStringLiteral("Dracula")));

        // One work with several audiobook files can be "merged" with itself.
        Work parts = makeWork(QStringLiteral("Teile"), {}, false, true);
        parts.formats.append({ Format::Kind::Audiobook, QStringLiteral("02.mp3"), 0.0 });
        m_works.updateWork(parts);
        m_filter.clearSelection();
        m_filter.toggleSelected(QStringLiteral("Teile"));
        QVERIFY(m_filter.canMergeSelection());
    }

    // Phase 9: the volumes of a series are one row, a stack.
    void seriesStacks()
    {
        m_works.setWorks({
            seriesWork(QStringLiteral("Der Hund der Baskervilles"), QStringLiteral("Sherlock Holmes"), 3, true,
                       QStringLiteral("2026-11-08T21:15:00")),
            seriesWork(QStringLiteral("Eine Studie in Scharlachrot"), QStringLiteral("Sherlock Holmes"), 1, false,
                       QStringLiteral("2026-08-02T20:10:00")),
            seriesWork(QStringLiteral("Abenteuer"), QStringLiteral("sherlock holmes"), 0), // same series, no number
            seriesWork(QStringLiteral("Einzelband"), QStringLiteral("Solo"), 1, false, QStringLiteral("2026-09-01T10:00:00")),
            makeWork(QStringLiteral("Zauberberg"), { QStringLiteral("Thomas Mann") }, true, false),
        });
        m_filter.setSortOrder(WorkFilterModel::Title);
        QCOMPARE(m_filter.count(), 5);
        QCOMPARE(m_filter.workCount(), 5);
        QVERIFY(!m_filter.isStack(0));

        QSignalSpy countChanged(&m_filter, &WorkFilterModel::countChanged);
        m_filter.setGroupSeries(true);
        QVERIFY(countChanged.count() > 0);
        // The stack sorts by the name of the series; one volume is no stack.
        QCOMPARE(titles(m_filter), (QStringList{ QStringLiteral("Einzelband"), QStringLiteral("Eine Studie in Scharlachrot"),
                                                 QStringLiteral("Zauberberg") }));
        QCOMPARE(m_filter.count(), 3);
        QCOMPARE(m_filter.workCount(), 5);
        QVERIFY(!m_filter.isStack(0));
        QVERIFY(m_filter.isStack(1));
        QVERIFY(!m_filter.isStack(7));

        // The first volume stands for the series.
        const QModelIndex stack = m_filter.index(1, 0);
        QCOMPARE(stack.data(WorkModel::SeriesRole).toString(), QStringLiteral("Sherlock Holmes"));
        QCOMPARE(stack.data(WorkFilterModel::SeriesCountRole).toInt(), 3);
        QCOMPARE(stack.data(WorkFilterModel::SeriesFirstRole).toInt(), 1);
        QCOMPARE(stack.data(WorkFilterModel::SeriesLastRole).toInt(), 3);
        QVERIFY(stack.data(WorkModel::HasEbookRole).toBool());
        QVERIFY(stack.data(WorkModel::HasAudiobookRole).toBool()); // volume 3 has one
        QCOMPARE(stack.data(WorkModel::WorkStateRole).toInt(), 0);
        QCOMPARE(m_filter.index(0, 0).data(WorkFilterModel::SeriesCountRole).toInt(), 0);

        // By "last opened" the stack counts with its latest volume.
        m_filter.setSortOrder(WorkFilterModel::LastOpened);
        QVERIFY(m_filter.isStack(0));
        QCOMPARE(m_filter.workIdAt(1), QStringLiteral("Einzelband"));

        // Phase 13: selecting a stack selects all its volumes (by number,
        // the unnumbered one last), together with single works.
        m_filter.selectAll();
        QCOMPARE(m_filter.selectedIds(),
                 (QStringList{ QStringLiteral("Eine Studie in Scharlachrot"), QStringLiteral("Der Hund der Baskervilles"),
                               QStringLiteral("Abenteuer"), QStringLiteral("Einzelband"), QStringLiteral("Zauberberg") }));
        QVERIFY(m_filter.index(0, 0).data(WorkFilterModel::SelectedRole).toBool());
        QSignalSpy selectionChanged(&m_filter, &WorkFilterModel::selectionChanged);
        m_filter.toggleSelected(m_filter.workIdAt(0)); // all were selected: none
        QCOMPARE(m_filter.selectedCount(), 2);
        QVERIFY(!m_filter.index(0, 0).data(WorkFilterModel::SelectedRole).toBool());
        m_filter.toggleSelected(m_filter.workIdAt(0));
        QCOMPARE(m_filter.selectedCount(), 5);
        QCOMPARE(selectionChanged.count(), 2);
        // A hidden volume of a shown stack stays selected; a hidden stack's do not.
        m_filter.setSearchText(QStringLiteral("zauberberg"));
        QCOMPARE(m_filter.selectedIds(), QStringList{ QStringLiteral("Zauberberg") });
        m_filter.setSearchText(QString());
        m_filter.clearSelection();

        // With the format selection the stack only has what its volumes have there.
        m_filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Der Hund der Baskervilles") }); // one volume: no stack
        QVERIFY(!m_filter.isStack(0));
        m_filter.setFormatFilter(WorkFilterModel::AllFormats);

        // An opened stack: its volumes by number, the unnumbered one last.
        m_filter.setSeriesFilter(QStringLiteral("Sherlock Holmes"));
        QCOMPARE(titles(m_filter), (QStringList{ QStringLiteral("Eine Studie in Scharlachrot"),
                                                 QStringLiteral("Der Hund der Baskervilles"), QStringLiteral("Abenteuer") }));
        QVERIFY(!m_filter.isStack(0));
        QCOMPARE(m_filter.workCount(), 3);
        QCOMPARE(m_filter.countAll(), 3);
        QCOMPARE(m_filter.countAudiobook(), 1);
        m_filter.setSeriesFilter(QString());
        QCOMPARE(m_filter.count(), 3);

        // Only series: every series is a stack, also a single volume.
        m_filter.setSeriesOnly(true);
        m_filter.setSortOrder(WorkFilterModel::Title);
        QCOMPARE(m_filter.count(), 2);
        QVERIFY(m_filter.isStack(0));
        QVERIFY(m_filter.isStack(1));
        QCOMPARE(m_filter.index(1, 0).data(WorkModel::SeriesRole).toString(), QStringLiteral("Solo"));
        QCOMPARE(m_filter.index(1, 0).data(WorkFilterModel::SeriesCountRole).toInt(), 1);
        QCOMPARE(m_filter.workCount(), 4);
        m_filter.setSeriesOnly(false);

        // A search that leaves one volume shows that volume.
        m_filter.setSearchText(QStringLiteral("hund"));
        QCOMPARE(titles(m_filter), QStringList{ QStringLiteral("Der Hund der Baskervilles") });
        QVERIFY(!m_filter.isStack(0));
        m_filter.setSearchText(QString());

        // The stack follows the library: a volume leaves the series.
        Work study = *m_works.workAt(1);
        study.series.clear();
        m_works.updateWork(study);
        QCOMPARE(m_filter.count(), 4);
        const int row = m_filter.rowOf(QStringLiteral("Der Hund der Baskervilles")); // the first volume now
        QVERIFY(m_filter.isStack(row));
        QCOMPARE(m_filter.index(row, 0).data(WorkFilterModel::SeriesCountRole).toInt(), 2);
        QVERIFY(!m_filter.isStack(m_filter.rowOf(QStringLiteral("Eine Studie in Scharlachrot"))));

        // And a removed one.
        QVERIFY(m_works.removeWork(QStringLiteral("Abenteuer")));
        QCOMPARE(m_filter.count(), 4);
        QVERIFY(!m_filter.isStack(m_filter.rowOf(QStringLiteral("Der Hund der Baskervilles"))));
    }

    // A fixed list of works in its order, e.g. the selection for the metadata editor.
    void onlyIds()
    {
        m_filter.setSearchText(QStringLiteral("kafka"));
        m_filter.setOnlyIds({ QStringLiteral("Moby Dick"), QStringLiteral("Dracula"), QStringLiteral("Faust I") });
        QCOMPARE(titles(m_filter), (QStringList{ QStringLiteral("Moby Dick"), QStringLiteral("Dracula"),
                                                 QStringLiteral("Faust I") }));
        QCOMPARE(m_filter.rowOf(QStringLiteral("Dracula")), 1);
        m_filter.setOnlyIds({});
        QCOMPARE(m_filter.count(), 2); // the search again
    }

private:
    WorkModel m_works;
    WorkFilterModel m_filter;
};

QTEST_GUILESS_MAIN(TestWorkFilterModel)
#include "tst_workfiltermodel.moc"
