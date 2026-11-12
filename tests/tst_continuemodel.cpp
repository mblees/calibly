#include <QSignalSpy>
#include <QTest>

#include "continuemodel.h"
#include "workfiltermodel.h"
#include "workmodel.h"
#include "workstate.h"

namespace {

QDateTime at(const QString &text)
{
    return QDateTime::fromString(text, Qt::ISODate);
}

Format ebook(double progress = 0.0, const QString &opened = QString(), int pages = 0)
{
    Format format;
    format.kind = Format::Kind::Ebook;
    format.filePath = QStringLiteral("book.epub");
    format.progress = progress;
    format.pages = pages;
    format.lastOpened = at(opened);
    return format;
}

Format audiobook(double progress = 0.0, const QString &opened = QString(), int durationSeconds = 0)
{
    Format format;
    format.kind = Format::Kind::Audiobook;
    format.filePath = QStringLiteral("book.m4b");
    format.progress = progress;
    format.durationSeconds = durationSeconds;
    format.positionMs = qint64(progress * durationSeconds * 1000);
    format.lastOpened = at(opened);
    return format;
}

Work makeWork(const QString &title, const QList<Format> &formats)
{
    Work work;
    work.id = title;
    work.title = title;
    work.formats = formats;
    for (const Format &format : formats) {
        if (format.lastOpened.isValid() && (!work.lastOpened.isValid() || format.lastOpened > work.lastOpened))
            work.lastOpened = format.lastOpened;
    }
    return work;
}

QStringList rows(const ContinueModel &model)
{
    QStringList result;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row);
        result.append(index.data(ContinueModel::TitleRole).toString() + u'/'
                      + index.data(ContinueModel::KindRole).toString());
    }
    return result;
}

} // namespace

class TestContinueModel : public QObject
{
    Q_OBJECT

private slots:
    // New until it is opened, finished when a format is, missing above all.
    void workStates()
    {
        Work work = makeWork(QStringLiteral("Faust"), { ebook(), audiobook() });
        QCOMPARE(work.state(), Work::State::New);

        work.markOpened(1, at(QStringLiteral("2026-11-10T19:00:00")));
        QCOMPARE(work.state(), Work::State::Normal);
        QCOMPARE(work.lastOpened, work.formats.at(1).lastOpened);
        QVERIFY(!work.formats.at(0).lastOpened.isValid());

        work.formats[1].progress = 1.0;
        QCOMPARE(work.state(), Work::State::Finished);
        // Presented as one format: only that format counts.
        QCOMPARE(work.state(Format::Kind::Audiobook), Work::State::Finished);
        QCOMPARE(work.state(Format::Kind::Ebook), Work::State::Normal);

        work.formats[0].fileMissing = true;
        QCOMPARE(work.state(), Work::State::FileMissing);
        QCOMPARE(work.state(Format::Kind::Ebook), Work::State::FileMissing);
        QCOMPARE(work.state(Format::Kind::Audiobook), Work::State::Finished);

        // Progress without a date (e.g. sample data) is not new either.
        QCOMPARE(makeWork(QStringLiteral("Dracula"), { audiobook(0.3) }).state(), Work::State::Normal);

        // The QML enum has the same values.
        QCOMPARE(int(WorkState::New), int(Work::State::New));
        QCOMPARE(int(WorkState::FileMissing), int(Work::State::FileMissing));
    }

    // Libraries from before phase 8 only know when the work was opened.
    void inheritsLastOpened()
    {
        Work work = makeWork(QStringLiteral("Moby Dick"), { ebook(), audiobook(0.5) });
        work.lastOpened = at(QStringLiteral("2026-11-11T08:14:00"));
        work.inheritLastOpened();
        QVERIFY(!work.formats.at(0).lastOpened.isValid()); // never read
        QCOMPARE(work.formats.at(1).lastOpened, work.lastOpened);

        // No progress anywhere: the first format was the one that was opened.
        Work opened = makeWork(QStringLiteral("Faust"), { ebook(), audiobook() });
        opened.lastOpened = at(QStringLiteral("2026-11-10T19:00:00"));
        opened.inheritLastOpened();
        QCOMPARE(opened.formats.at(0).lastOpened, opened.lastOpened);
        QVERIFY(!opened.formats.at(1).lastOpened.isValid());

        // Formats that know it keep it.
        Work known = makeWork(QStringLiteral("Dracula"), { ebook(0.2), audiobook(0.3, QStringLiteral("2026-11-09T22:40:00")) });
        known.inheritLastOpened();
        QVERIFY(!known.formats.at(0).lastOpened.isValid());
    }

    void stateRoles()
    {
        WorkModel source;
        Format missing = audiobook();
        missing.fileMissing = true;
        source.setWorks({ makeWork(QStringLiteral("Neu"), { ebook() }),
                          makeWork(QStringLiteral("Fertig"), { ebook(1.0, QStringLiteral("2026-11-01T10:00:00")), audiobook() }),
                          makeWork(QStringLiteral("Fehlt"), { ebook(0.2, QStringLiteral("2026-11-02T10:00:00")), missing }) });
        QCOMPARE(source.index(0).data(WorkModel::WorkStateRole).toInt(), int(WorkState::New));
        QCOMPARE(source.index(1).data(WorkModel::WorkStateRole).toInt(), int(WorkState::Finished));
        QCOMPARE(source.index(2).data(WorkModel::WorkStateRole).toInt(), int(WorkState::FileMissing));
        QCOMPARE(source.roleNames().value(WorkModel::WorkStateRole), QByteArray("workState"));

        // The format selection shows the state of that format.
        WorkFilterModel filter;
        filter.setSourceModel(&source);
        filter.setSortOrder(WorkFilterModel::Title);
        QCOMPARE(filter.workIdAt(0), QStringLiteral("Fehlt"));
        QSignalSpy changed(&filter, &QAbstractItemModel::dataChanged);
        filter.setFormatFilter(WorkFilterModel::Ebook);
        QVERIFY(changed.count() > 0);
        QVERIFY(changed.last().at(2).value<QList<int>>().contains(WorkModel::WorkStateRole));
        QCOMPARE(filter.index(0, 0).data(WorkModel::WorkStateRole).toInt(), int(WorkState::Normal));   // Fehlt
        QCOMPARE(filter.index(1, 0).data(WorkModel::WorkStateRole).toInt(), int(WorkState::Finished)); // Fertig
        filter.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(filter.index(0, 0).data(WorkModel::WorkStateRole).toInt(), int(WorkState::FileMissing));
        QCOMPARE(filter.index(1, 0).data(WorkModel::WorkStateRole).toInt(), int(WorkState::Normal));
    }

    void init()
    {
        m_works.setWorks({
            // Listened to yesterday, read the day before: the audiobook.
            makeWork(QStringLiteral("Moby Dick"), { ebook(0.31, QStringLiteral("2026-11-09T20:00:00"), 652),
                                                    audiobook(0.5, QStringLiteral("2026-11-10T08:14:00"), 3600) }),
            // Opened, progress unknown (ebooks open in another application).
            makeWork(QStringLiteral("Faust"), { ebook(0.0, QStringLiteral("2026-11-11T19:02:00"), 210) }),
            // Finished: nothing to continue.
            makeWork(QStringLiteral("Effi Briest"), { ebook(1.0, QStringLiteral("2026-11-11T21:00:00")) }),
            // Never opened.
            makeWork(QStringLiteral("Woyzeck"), { ebook() }),
            // The audiobook is finished, the ebook is not.
            makeWork(QStringLiteral("Dracula"), { ebook(0.42, QStringLiteral("2026-11-08T10:00:00"), 400),
                                                  audiobook(1.0, QStringLiteral("2026-11-09T10:00:00"), 7200) }),
        });
        m_model.setSourceModel(&m_works);
        m_model.setFormatFilter(WorkFilterModel::AllFormats);
        m_model.setLimit(0);
    }

    // One row per work, most recently opened first, for the unfinished
    // format that was opened last.
    void rowsAndOrder()
    {
        QCOMPARE(rows(m_model), (QStringList{ QStringLiteral("Faust/ebook"), QStringLiteral("Moby Dick/audiobook"),
                                              QStringLiteral("Dracula/ebook") }));
        QCOMPARE(m_model.count(), 3);

        const QModelIndex moby = m_model.index(1);
        QCOMPARE(moby.data(ContinueModel::WorkIdRole).toString(), QStringLiteral("Moby Dick"));
        QCOMPARE(moby.data(ContinueModel::ProgressRole).toDouble(), 0.5);
        QCOMPARE(moby.data(ContinueModel::RemainingSecondsRole).toInt(), 1800);
        QCOMPARE(moby.data(ContinueModel::ChapterNumberRole).toInt(), 0); // no chapters
        QVERIFY(moby.data(ContinueModel::HasOtherFormatRole).toBool());
        QVERIFY(moby.data(ContinueModel::CoverIsSquareRole).toBool());    // audiobook placeholder
        QCOMPARE(moby.data(ContinueModel::LastOpenedRole).toDateTime(), at(QStringLiteral("2026-11-10T08:14:00")));
        QVERIFY(moby.data(ContinueModel::AvailableRole).toBool());

        const QModelIndex faust = m_model.index(0);
        QCOMPARE(faust.data(ContinueModel::PageRole).toInt(), 0); // unknown
        QCOMPARE(faust.data(ContinueModel::PagesRole).toInt(), 210);
        QVERIFY(!faust.data(ContinueModel::HasOtherFormatRole).toBool());

        const QModelIndex dracula = m_model.index(2);
        QCOMPARE(dracula.data(ContinueModel::PageRole).toInt(), 168);
    }

    void chapters()
    {
        Format format = audiobook(0.0, QStringLiteral("2026-11-10T08:14:00"), 300);
        format.chapters = { { QStringLiteral("Eins"), QString(), 0, 100000 },
                            { QStringLiteral("Zwei"), QString(), 100000, 100000 },
                            { QString(), QString(), 200000, 100000 } };
        format.positionMs = 150000;
        format.progress = 0.5;
        format.fileMissing = true;
        m_works.setWorks({ makeWork(QStringLiteral("Der Marsianer"), { format }) });
        const QModelIndex index = m_model.index(0);
        QCOMPARE(index.data(ContinueModel::ChapterNumberRole).toInt(), 2);
        QCOMPARE(index.data(ContinueModel::ChapterTitleRole).toString(), QStringLiteral("Zwei"));
        QCOMPARE(index.data(ContinueModel::RemainingSecondsRole).toInt(), 150);
        QVERIFY(!index.data(ContinueModel::AvailableRole).toBool());
    }

    void formatFilterAndLimit()
    {
        m_model.setFormatFilter(WorkFilterModel::Ebook);
        QCOMPARE(rows(m_model), (QStringList{ QStringLiteral("Faust/ebook"), QStringLiteral("Moby Dick/ebook"),
                                              QStringLiteral("Dracula/ebook") }));
        m_model.setFormatFilter(WorkFilterModel::Audiobook);
        QCOMPARE(rows(m_model), QStringList{ QStringLiteral("Moby Dick/audiobook") });
        m_model.setFormatFilter(WorkFilterModel::BothFormats);
        QCOMPARE(rows(m_model), (QStringList{ QStringLiteral("Moby Dick/audiobook"), QStringLiteral("Dracula/ebook") }));

        m_model.setFormatFilter(WorkFilterModel::AllFormats);
        QSignalSpy countChanged(&m_model, &ContinueModel::countChanged);
        m_model.setLimit(2);
        QCOMPARE(rows(m_model), (QStringList{ QStringLiteral("Faust/ebook"), QStringLiteral("Moby Dick/audiobook") }));
        QCOMPARE(countChanged.count(), 1);
    }

    // Formats opened at the same moment (taken over from the work): the audiobook.
    void sameTimePrefersAudiobook()
    {
        const QString time = QStringLiteral("2026-11-10T08:14:00");
        m_works.setWorks({ makeWork(QStringLiteral("Moby Dick"), { ebook(0.3, time), audiobook(0.5, time, 3600) }) });
        QCOMPARE(rows(m_model), QStringList{ QStringLiteral("Moby Dick/audiobook") });
    }

    // The model follows the library: progress changes keep the rows (views
    // keep their place), finishing or opening a title changes them.
    void followsSource()
    {
        QSignalSpy reset(&m_model, &QAbstractItemModel::modelReset);
        QSignalSpy changed(&m_model, &QAbstractItemModel::dataChanged);

        Work moby = *m_works.workAt(0);
        moby.formats[1].progress = 0.75;
        moby.formats[1].positionMs = 2700000;
        m_works.updateWork(moby);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(m_model.index(1).data(ContinueModel::ProgressRole).toDouble(), 0.75);
        QCOMPARE(m_model.index(1).data(ContinueModel::RemainingSecondsRole).toInt(), 900);

        // Listened to the end: the ebook is what is left to continue.
        moby.formats[1].progress = 1.0;
        m_works.updateWork(moby);
        QCOMPARE(reset.count(), 1);
        QCOMPARE(rows(m_model), (QStringList{ QStringLiteral("Faust/ebook"), QStringLiteral("Moby Dick/ebook"),
                                              QStringLiteral("Dracula/ebook") }));

        // A title that is opened for the first time comes first.
        Work woyzeck = *m_works.workAt(3);
        woyzeck.markOpened(0, at(QStringLiteral("2026-11-12T09:00:00")));
        m_works.updateWork(woyzeck);
        QCOMPARE(rows(m_model).first(), QStringLiteral("Woyzeck/ebook"));
        QCOMPARE(m_model.count(), 4);

        m_model.setSourceModel(nullptr);
        QCOMPARE(m_model.count(), 0);
    }

private:
    WorkModel m_works;
    ContinueModel m_model;
};

QTEST_GUILESS_MAIN(TestContinueModel)
#include "tst_continuemodel.moc"
