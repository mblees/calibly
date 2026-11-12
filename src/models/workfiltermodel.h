#pragma once

#include <QCollator>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QSet>
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
//
// Series (phase 9):
//   groupSeries   the shown works of one series (two or more) become one
//                 row, a "stack": the row of the first volume, with the
//                 roles seriesCount (number of volumes, 0 = not a stack),
//                 seriesFirst / seriesLast (lowest / highest volume number,
//                 0 = unknown) and series (its name). hasEbook/hasAudiobook
//                 then tell what any volume has; progress and state are
//                 not reported. A stack sorts by the name of the series, or
//                 by the volume that was opened last. isStack(row) asks for it.
//   seriesFilter  only the works of this series ("" = all), in the order of
//                 their volume numbers and without stacks: an opened stack.
//   seriesOnly    only works that belong to a series; with groupSeries every
//                 series is a stack then, also one with a single volume.
//   workCount     the works behind the shown rows (count counts a stack once)
//
// Selection (phase 9): shown works can be selected; stacks cannot. The role
// "selected" tells it per row. Works that are no longer shown (filter,
// search, removed) leave the selection, so it never contains anything the
// user does not see.
//   toggleSelected(workId), selectAll(), clearSelection()
//   selectedCount, selectedIds (in the shown order)
//   canMergeSelection   "Zu einem Werk verbinden" makes sense: two or more
//                       works, or one work with several audiobook files
//
// onlyIds: shows exactly these works, in this order, ignoring every other
// filter – e.g. to step through a selection in the metadata editor.
class WorkFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(FormatFilter formatFilter READ formatFilter WRITE setFormatFilter NOTIFY formatFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(bool groupSeries READ groupSeries WRITE setGroupSeries NOTIFY groupSeriesChanged)
    Q_PROPERTY(QString seriesFilter READ seriesFilter WRITE setSeriesFilter NOTIFY seriesFilterChanged)
    Q_PROPERTY(bool seriesOnly READ seriesOnly WRITE setSeriesOnly NOTIFY seriesOnlyChanged)
    Q_PROPERTY(QStringList onlyIds READ onlyIds WRITE setOnlyIds NOTIFY onlyIdsChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int workCount READ workCount NOTIFY workCountChanged)
    Q_PROPERTY(int countAll READ countAll NOTIFY formatCountsChanged)
    Q_PROPERTY(int countEbook READ countEbook NOTIFY formatCountsChanged)
    Q_PROPERTY(int countAudiobook READ countAudiobook NOTIFY formatCountsChanged)
    Q_PROPERTY(int countBoth READ countBoth NOTIFY formatCountsChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QStringList selectedIds READ selectedIds NOTIFY selectionChanged)
    Q_PROPERTY(bool canMergeSelection READ canMergeSelection NOTIFY selectionChanged)

public:
    enum FormatFilter { AllFormats, Ebook, Audiobook, BothFormats };
    Q_ENUM(FormatFilter)

    enum SortOrder {
        LastOpened, // most recently opened first, never opened at the end
        Title,      // A–Z
        Author,     // by last name of the first author
    };
    Q_ENUM(SortOrder)

    // Roles of this model, in addition to those of WorkModel.
    enum Role {
        SelectedRole = Qt::UserRole + 1000,
        SeriesCountRole,
        SeriesFirstRole,
        SeriesLastRole,
    };
    Q_ENUM(Role)

    explicit WorkFilterModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *model) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    FormatFilter formatFilter() const { return m_formatFilter; }
    void setFormatFilter(FormatFilter filter);

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);

    SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(SortOrder order);

    bool groupSeries() const { return m_groupSeries; }
    void setGroupSeries(bool group);

    QString seriesFilter() const { return m_seriesFilter; }
    void setSeriesFilter(const QString &series);

    bool seriesOnly() const { return m_seriesOnly; }
    void setSeriesOnly(bool only);

    QStringList onlyIds() const { return m_onlyIds; }
    void setOnlyIds(const QStringList &ids);

    int count() const { return rowCount(); }
    int workCount() const { return m_workCount; }
    // Work id in this row, "" for an invalid row.
    Q_INVOKABLE QString workIdAt(int row) const;
    // Row of the work, -1 if it is not shown.
    Q_INVOKABLE int rowOf(const QString &workId) const;
    // The row stands for a series (see groupSeries).
    Q_INVOKABLE bool isStack(int row) const;
    int countAll() const { return m_countAll; }
    int countEbook() const { return m_countEbook; }
    int countAudiobook() const { return m_countAudiobook; }
    int countBoth() const { return m_countBoth; }

    int selectedCount() const { return int(m_selected.size()); }
    QStringList selectedIds() const;
    bool canMergeSelection() const;
    Q_INVOKABLE void toggleSelected(const QString &workId);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();

signals:
    void formatFilterChanged();
    void searchTextChanged();
    void sortOrderChanged();
    void groupSeriesChanged();
    void seriesFilterChanged();
    void seriesOnlyChanged();
    void onlyIdsChanged();
    void countChanged();
    void workCountChanged();
    void formatCountsChanged();
    void selectionChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    // The works of one series as one row.
    struct Stack
    {
        QString name;
        int count = 0;
        int first = 0;          // lowest volume number, 0 = unknown
        int last = 0;
        bool hasEbook = false;
        bool hasAudiobook = false;
        QDateTime lastOpened;   // of the volume that was opened last

        bool operator==(const Stack &other) const
        {
            return name == other.name && count == other.count && first == other.first && last == other.last
                && hasEbook == other.hasEbook && hasAudiobook == other.hasAudiobook
                && lastOpened == other.lastOpened;
        }
    };

    bool matchesSearch(const QModelIndex &sourceIndex) const;
    bool matchesFormat(const QModelIndex &sourceIndex) const;
    bool matchesSeries(const QModelIndex &sourceIndex) const;
    // Everything but stacking: the work is shown, as itself or in a stack.
    bool matchesFilters(const QModelIndex &sourceIndex) const;
    bool stacking() const;
    // The stack this source row stands for, nullptr if it is a plain work.
    const Stack *stackOf(int sourceRow) const;
    int compareText(const QString &left, const QString &right) const;
    int compareText(const QModelIndex &left, const QModelIndex &right, int role) const;
    // Works out the stacks again; true if they changed.
    bool rebuildStacks();
    // Call after a filter property changed.
    void filtersChanged();
    void onSourceChanged();
    void onRowsChanged();
    void updateCount();
    void updateFormatCounts();
    void pruneSelection();

    FormatFilter m_formatFilter = AllFormats;
    QString m_searchText;
    QStringList m_searchTerms;
    SortOrder m_sortOrder = LastOpened;
    bool m_groupSeries = false;
    QString m_seriesFilter;
    QString m_seriesFilterKey;  // folded, see seriesKey()
    bool m_seriesOnly = false;
    QStringList m_onlyIds;
    QCollator m_collator;
    QList<QMetaObject::Connection> m_sourceConnections;

    QList<int> m_leadOfRow;     // per source row: the row of the first volume of its stack, -1 = no stack
    QHash<int, Stack> m_stacks; // by the source row of the first volume
    QSet<QString> m_selected;

    int m_count = 0;
    int m_workCount = 0;
    int m_countAll = 0;
    int m_countEbook = 0;
    int m_countAudiobook = 0;
    int m_countBoth = 0;
};
