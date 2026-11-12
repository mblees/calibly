#pragma once

#include <QCollator>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QStringList>

// Filtered and sorted view on a WorkModel. All filter and sort logic lives
// here, the QML views only set the properties (UI Development Plan, rule 6).
//
// In QML (import Calibly.Models):
//   WorkFilterModel {
//       sourceModel: LibraryController.works
//       formatFilter: WorkFilterModel.Audiobook
//       searchText: "kafka"
//       sortOrder: WorkFilterModel.Title
//   }
//
// Format filter: "Ebook" = works that have an ebook (they may also have an
// audiobook), "Audiobook" accordingly, "BothFormats" = works with both.
// With "Ebook" or "Audiobook" the works are presented as that format only:
// coverUrl/coverIsSquare report the cover of the selected format, workState
// its state (e.g. finished only if that format is finished), and the
// roles of the other format (hasAudiobook/listenProgress or
// hasEbook/readProgress) report false / 0, so the tiles show only the cover,
// badge and progress of the selected format.
// Search: every word must appear in the title, an author, a narrator or the
// series; case and accents are ignored ("marchen" finds "Märchen").
// The count* properties count the works that match the search, per format,
// so the format chips can show how many works each of them would show.
// workIdAt(row) and rowOf(workId) step through the shown works, e.g. for
// "previous / next title" in the metadata editor.
class WorkFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(FormatFilter formatFilter READ formatFilter WRITE setFormatFilter NOTIFY formatFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int countAll READ countAll NOTIFY formatCountsChanged)
    Q_PROPERTY(int countEbook READ countEbook NOTIFY formatCountsChanged)
    Q_PROPERTY(int countAudiobook READ countAudiobook NOTIFY formatCountsChanged)
    Q_PROPERTY(int countBoth READ countBoth NOTIFY formatCountsChanged)

public:
    enum FormatFilter { AllFormats, Ebook, Audiobook, BothFormats };
    Q_ENUM(FormatFilter)

    enum SortOrder {
        LastOpened, // most recently opened first, never opened at the end
        Title,      // A–Z
        Author,     // by last name of the first author
    };
    Q_ENUM(SortOrder)

    explicit WorkFilterModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *model) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    FormatFilter formatFilter() const { return m_formatFilter; }
    void setFormatFilter(FormatFilter filter);

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);

    SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(SortOrder order);

    int count() const { return rowCount(); }
    // Work id in this row, "" for an invalid row.
    Q_INVOKABLE QString workIdAt(int row) const;
    // Row of the work, -1 if it is not shown.
    Q_INVOKABLE int rowOf(const QString &workId) const;
    int countAll() const { return m_countAll; }
    int countEbook() const { return m_countEbook; }
    int countAudiobook() const { return m_countAudiobook; }
    int countBoth() const { return m_countBoth; }

signals:
    void formatFilterChanged();
    void searchTextChanged();
    void sortOrderChanged();
    void countChanged();
    void formatCountsChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool matchesSearch(const QModelIndex &sourceIndex) const;
    bool matchesFormat(const QModelIndex &sourceIndex) const;
    int compareText(const QModelIndex &left, const QModelIndex &right, int role) const;
    void refilter();
    void updateCount();
    void updateFormatCounts();

    FormatFilter m_formatFilter = AllFormats;
    QString m_searchText;
    QStringList m_searchTerms;
    SortOrder m_sortOrder = LastOpened;
    QCollator m_collator;
    QList<QMetaObject::Connection> m_sourceConnections;

    int m_count = 0;
    int m_countAll = 0;
    int m_countEbook = 0;
    int m_countAudiobook = 0;
    int m_countBoth = 0;
};
