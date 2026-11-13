#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QPointer>
#include <QQmlEngine>
#include <QUrl>

#include "workfiltermodel.h"
#include "workmodel.h"

// The titles to continue with (the row "Weitermachen" in the library and
// the view of the same name): one row per work with a format that was
// started – opened, or with progress – and is not finished, most recently
// opened first. A row stands for one format of the work: the unfinished one
// that was opened last (the audiobook if both were opened at the same time).
// A work whose started formats are all finished is not listed.
// It follows every change of the source model.
//
// In QML (import Calibly.Models):
//   ContinueModel {
//       sourceModel: LibraryController.works
//       formatFilter: WorkFilterModel.Audiobook   // only audiobooks to continue
//       limit: 12                                 // 0 = all
//   }
//
// formatFilter works like in WorkFilterModel: "Ebook" / "Audiobook" look at
// that format only, "BothFormats" at works that have both.
//
// Roles:
//   workId, title, authorText
//   kind               "ebook" or "audiobook": the format to continue with
//   coverUrl, coverIsSquare   cover of that format
//   progress           0.0 – 1.0
//   lastOpened         when that format was opened
//   volumeNumber       ebooks of a combined work: the current volume, 1-based;
//                      0 = a single book
//   chapterNumber      audiobooks: current chapter, 1-based; 0 = no chapters
//   chapterTitle       its title, "" = untitled; ebooks read in calibly: the
//                      chapter where the user stopped (EPUB, text)
//   remainingSeconds   audiobooks: time left at normal speed, 0 = unknown
//   page, pages        ebooks: current page (0 = unknown; a PDF read in calibly:
//                      the stored page) of pages (0 = unknown)
//   hasOtherFormat     the work also has the other format
//   available          the file of the format was found
class ContinueModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(WorkModel *sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
    Q_PROPERTY(WorkFilterModel::FormatFilter formatFilter READ formatFilter WRITE setFormatFilter
               NOTIFY formatFilterChanged)
    Q_PROPERTY(int limit READ limit WRITE setLimit NOTIFY limitChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        WorkIdRole = Qt::UserRole + 1,
        TitleRole,
        AuthorTextRole,
        KindRole,
        CoverUrlRole,
        CoverIsSquareRole,
        ProgressRole,
        LastOpenedRole,
        ChapterNumberRole,
        ChapterTitleRole,
        RemainingSecondsRole,
        PageRole,
        PagesRole,
        HasOtherFormatRole,
        AvailableRole,
        VolumeNumberRole,
    };
    Q_ENUM(Role)

    explicit ContinueModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    WorkModel *sourceModel() const { return m_source; }
    void setSourceModel(WorkModel *model);

    WorkFilterModel::FormatFilter formatFilter() const { return m_formatFilter; }
    void setFormatFilter(WorkFilterModel::FormatFilter filter);

    int limit() const { return m_limit; }
    void setLimit(int limit);

    int count() const { return int(m_entries.size()); }

signals:
    void sourceModelChanged();
    void formatFilterChanged();
    void limitChanged();
    void countChanged();

private:
    struct Entry
    {
        QString workId;
        QString title;
        QString authorText;
        Format::Kind kind = Format::Kind::Ebook;
        QUrl coverUrl;
        bool coverIsSquare = false;
        double progress = 0.0;
        QDateTime lastOpened;
        int chapterNumber = 0;
        QString chapterTitle;
        int remainingSeconds = 0;
        int page = 0;
        int pages = 0;
        bool hasOtherFormat = false;
        bool available = true;
        int volumeNumber = 0;
    };

    void rebuild();
    QList<Entry> entries() const;
    // The format of the work to continue with, -1 if there is none.
    int formatToContinue(const Work &work) const;

    QPointer<WorkModel> m_source;
    QList<QMetaObject::Connection> m_connections;
    WorkFilterModel::FormatFilter m_formatFilter = WorkFilterModel::AllFormats;
    int m_limit = 0;
    QList<Entry> m_entries;
};
