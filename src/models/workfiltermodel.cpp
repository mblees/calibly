#include "workfiltermodel.h"

#include <QDateTime>

#include "textutils.h"
#include "workmodel.h"

WorkFilterModel::WorkFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_collator.setCaseSensitivity(Qt::CaseInsensitive);
    m_collator.setNumericMode(true); // "Band 2" before "Band 10"

    // count follows every change of the visible rows.
    connect(this, &QAbstractItemModel::rowsInserted, this, &WorkFilterModel::updateCount);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &WorkFilterModel::updateCount);
    connect(this, &QAbstractItemModel::modelReset, this, &WorkFilterModel::updateCount);
    connect(this, &QAbstractItemModel::layoutChanged, this, &WorkFilterModel::updateCount);

    sort(0);
}

void WorkFilterModel::setSourceModel(QAbstractItemModel *model)
{
    for (const QMetaObject::Connection &connection : std::as_const(m_sourceConnections))
        disconnect(connection);
    m_sourceConnections.clear();

    QSortFilterProxyModel::setSourceModel(model);

    if (model) {
        const auto update = [this] { updateFormatCounts(); };
        m_sourceConnections = {
            connect(model, &QAbstractItemModel::modelReset, this, update),
            connect(model, &QAbstractItemModel::rowsInserted, this, update),
            connect(model, &QAbstractItemModel::rowsRemoved, this, update),
            connect(model, &QAbstractItemModel::dataChanged, this, update),
        };
    }
    sort(0);
    updateFormatCounts();
    updateCount();
}

void WorkFilterModel::setFormatFilter(FormatFilter filter)
{
    if (filter == m_formatFilter)
        return;
    m_formatFilter = filter;
    refilter();
    // Works that stay visible may now be presented differently (see data()).
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0),
                         { WorkModel::HasEbookRole, WorkModel::HasAudiobookRole,
                           WorkModel::ReadProgressRole, WorkModel::ListenProgressRole,
                           WorkModel::CoverUrlRole, WorkModel::CoverIsSquareRole });
    }
    emit formatFilterChanged();
}

QVariant WorkFilterModel::data(const QModelIndex &index, int role) const
{
    // Present the works as the selected format only.
    if (role == WorkModel::CoverUrlRole && (m_formatFilter == Ebook || m_formatFilter == Audiobook)) {
        return QSortFilterProxyModel::data(index, m_formatFilter == Ebook ? WorkModel::EbookCoverUrlRole
                                                                          : WorkModel::AudiobookCoverUrlRole);
    }
    if (role == WorkModel::CoverIsSquareRole && (m_formatFilter == Ebook || m_formatFilter == Audiobook)) {
        return QSortFilterProxyModel::data(index, m_formatFilter == Ebook ? WorkModel::EbookCoverIsSquareRole
                                                                          : WorkModel::AudiobookCoverIsSquareRole);
    }
    if (m_formatFilter == Audiobook && (role == WorkModel::HasEbookRole || role == WorkModel::ReadProgressRole))
        return role == WorkModel::HasEbookRole ? QVariant(false) : QVariant(0.0);
    if (m_formatFilter == Ebook && (role == WorkModel::HasAudiobookRole || role == WorkModel::ListenProgressRole))
        return role == WorkModel::HasAudiobookRole ? QVariant(false) : QVariant(0.0);
    return QSortFilterProxyModel::data(index, role);
}

void WorkFilterModel::setSearchText(const QString &text)
{
    if (text == m_searchText)
        return;
    m_searchText = text;
    m_searchTerms = TextUtils::searchTerms(text);
    refilter();
    updateFormatCounts();
    emit searchTextChanged();
}

void WorkFilterModel::setSortOrder(SortOrder order)
{
    if (order == m_sortOrder)
        return;
    m_sortOrder = order;
    m_collator.setLocale(QLocale()); // follow the current UI language
    invalidate();
    sort(0);
    emit sortOrderChanged();
}

void WorkFilterModel::refilter()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateRowsFilter();
#endif
    updateCount();
}

void WorkFilterModel::updateCount()
{
    const int newCount = rowCount();
    if (newCount == m_count)
        return;
    m_count = newCount;
    emit countChanged();
}

bool WorkFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    return matchesFormat(index) && matchesSearch(index);
}

bool WorkFilterModel::matchesSearch(const QModelIndex &sourceIndex) const
{
    if (m_searchTerms.isEmpty())
        return true;
    const QString haystack = sourceIndex.data(WorkModel::SearchTextRole).toString();
    for (const QString &term : m_searchTerms) {
        if (!haystack.contains(term))
            return false;
    }
    return true;
}

bool WorkFilterModel::matchesFormat(const QModelIndex &sourceIndex) const
{
    const bool ebook = sourceIndex.data(WorkModel::HasEbookRole).toBool();
    const bool audiobook = sourceIndex.data(WorkModel::HasAudiobookRole).toBool();
    switch (m_formatFilter) {
    case Ebook:
        return ebook;
    case Audiobook:
        return audiobook;
    case BothFormats:
        return ebook && audiobook;
    case AllFormats:
        break;
    }
    return true;
}

int WorkFilterModel::compareText(const QModelIndex &left, const QModelIndex &right, int role) const
{
    return m_collator.compare(left.data(role).toString(), right.data(role).toString());
}

bool WorkFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (m_sortOrder) {
    case LastOpened: {
        const QDateTime a = left.data(WorkModel::LastOpenedRole).toDateTime();
        const QDateTime b = right.data(WorkModel::LastOpenedRole).toDateTime();
        if (a.isValid() != b.isValid())
            return a.isValid(); // opened works before never opened ones
        if (a != b)
            return a > b;       // most recent first
        break;
    }
    case Author: {
        const int byAuthor = compareText(left, right, WorkModel::AuthorSortRole);
        if (byAuthor != 0)
            return byAuthor < 0;
        break;
    }
    case Title:
        break;
    }
    // Title as the (secondary) order, then the author for equal titles.
    const int byTitle = compareText(left, right, WorkModel::TitleRole);
    if (byTitle != 0)
        return byTitle < 0;
    return compareText(left, right, WorkModel::AuthorSortRole) < 0;
}

void WorkFilterModel::updateFormatCounts()
{
    int all = 0, ebook = 0, audiobook = 0, both = 0;
    if (QAbstractItemModel *model = sourceModel()) {
        const int rows = model->rowCount();
        for (int row = 0; row < rows; ++row) {
            const QModelIndex index = model->index(row, 0);
            if (!matchesSearch(index))
                continue;
            const bool hasEbook = index.data(WorkModel::HasEbookRole).toBool();
            const bool hasAudiobook = index.data(WorkModel::HasAudiobookRole).toBool();
            ++all;
            ebook += hasEbook;
            audiobook += hasAudiobook;
            both += hasEbook && hasAudiobook;
        }
    }

    if (all == m_countAll && ebook == m_countEbook && audiobook == m_countAudiobook && both == m_countBoth)
        return;
    m_countAll = all;
    m_countEbook = ebook;
    m_countAudiobook = audiobook;
    m_countBoth = both;
    emit formatCountsChanged();
}
