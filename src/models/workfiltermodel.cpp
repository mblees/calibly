#include "workfiltermodel.h"

#include <QDateTime>

#include <algorithm>
#include <limits>

#include "textutils.h"
#include "workmodel.h"

namespace {

// Series names that differ only in case or accents are one series.
QString seriesKey(const QString &name)
{
    return TextUtils::foldForSearch(name).simplified();
}

} // namespace

WorkFilterModel::WorkFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_collator.setCaseSensitivity(Qt::CaseInsensitive);
    m_collator.setNumericMode(true); // "Band 2" before "Band 10"

    // count and the selection follow every change of the visible rows.
    connect(this, &QAbstractItemModel::rowsInserted, this, &WorkFilterModel::onRowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &WorkFilterModel::onRowsChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &WorkFilterModel::onRowsChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &WorkFilterModel::onRowsChanged);

    sort(0);
}

void WorkFilterModel::setSourceModel(QAbstractItemModel *model)
{
    for (const QMetaObject::Connection &connection : std::as_const(m_sourceConnections))
        disconnect(connection);
    m_sourceConnections.clear();

    // The stacks belong to the old model.
    m_leadOfRow.clear();
    m_stacks.clear();
    QSortFilterProxyModel::setSourceModel(model);

    if (model) {
        // Connected after the proxy's own handlers, so these run when the
        // proxy has already taken over the change.
        const auto update = [this] { onSourceChanged(); };
        m_sourceConnections = {
            connect(model, &QAbstractItemModel::modelReset, this, update),
            connect(model, &QAbstractItemModel::rowsInserted, this, update),
            connect(model, &QAbstractItemModel::rowsRemoved, this, update),
            connect(model, &QAbstractItemModel::dataChanged, this, update),
        };
    }
    sort(0);
    if (rebuildStacks())
        invalidate();
    updateFormatCounts();
    onRowsChanged();
}

QHash<int, QByteArray> WorkFilterModel::roleNames() const
{
    QHash<int, QByteArray> names = QSortFilterProxyModel::roleNames();
    names.insert(SelectedRole, "selected");
    names.insert(SeriesCountRole, "seriesCount");
    names.insert(SeriesFirstRole, "seriesFirst");
    names.insert(SeriesLastRole, "seriesLast");
    return names;
}

void WorkFilterModel::setFormatFilter(FormatFilter filter)
{
    if (filter == m_formatFilter)
        return;
    m_formatFilter = filter;
    filtersChanged();
    // Works that stay visible may now be presented differently (see data()).
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0),
                         { WorkModel::HasEbookRole, WorkModel::HasAudiobookRole,
                           WorkModel::ReadProgressRole, WorkModel::ListenProgressRole,
                           WorkModel::CoverUrlRole, WorkModel::CoverIsSquareRole, WorkModel::WorkStateRole });
    }
    emit formatFilterChanged();
}

QString WorkFilterModel::workIdAt(int row) const
{
    return row >= 0 && row < rowCount() ? index(row, 0).data(WorkModel::WorkIdRole).toString() : QString();
}

int WorkFilterModel::rowOf(const QString &workId) const
{
    for (int row = 0; row < rowCount(); ++row) {
        if (index(row, 0).data(WorkModel::WorkIdRole).toString() == workId)
            return row;
    }
    return -1;
}

bool WorkFilterModel::isStack(int row) const
{
    return row >= 0 && row < rowCount() && stackOf(mapToSource(index(row, 0)).row()) != nullptr;
}

QVariant WorkFilterModel::data(const QModelIndex &index, int role) const
{
    const bool oneFormat = m_formatFilter == Ebook || m_formatFilter == Audiobook;
    switch (role) {
    case SelectedRole:
        return m_selected.contains(QSortFilterProxyModel::data(index, WorkModel::WorkIdRole).toString());
    case SeriesCountRole:
    case SeriesFirstRole:
    case SeriesLastRole: {
        const Stack *stack = stackOf(mapToSource(index).row());
        if (!stack)
            return 0;
        return role == SeriesCountRole ? stack->count : role == SeriesFirstRole ? stack->first : stack->last;
    }
    // Present the works as the selected format only.
    case WorkModel::CoverUrlRole:
        if (oneFormat) {
            return QSortFilterProxyModel::data(index, m_formatFilter == Ebook ? WorkModel::EbookCoverUrlRole
                                                                              : WorkModel::AudiobookCoverUrlRole);
        }
        break;
    case WorkModel::CoverIsSquareRole:
        if (oneFormat) {
            return QSortFilterProxyModel::data(index, m_formatFilter == Ebook ? WorkModel::EbookCoverIsSquareRole
                                                                              : WorkModel::AudiobookCoverIsSquareRole);
        }
        break;
    case WorkModel::HasEbookRole:
    case WorkModel::HasAudiobookRole: {
        const bool ebook = role == WorkModel::HasEbookRole;
        if (m_formatFilter == (ebook ? Audiobook : Ebook))
            return false;
        // A stack has what any of its volumes has.
        if (const Stack *stack = stackOf(mapToSource(index).row()))
            return ebook ? stack->hasEbook : stack->hasAudiobook;
        break;
    }
    case WorkModel::ReadProgressRole:
    case WorkModel::ListenProgressRole:
        if (m_formatFilter == (role == WorkModel::ReadProgressRole ? Audiobook : Ebook)
            || stackOf(mapToSource(index).row())) {
            return 0.0;
        }
        break;
    case WorkModel::WorkStateRole:
        if (stackOf(mapToSource(index).row()))
            return 0; // WorkState.Normal: a stack is not new, finished or missing
        if (oneFormat) {
            return QSortFilterProxyModel::data(index, m_formatFilter == Ebook ? WorkModel::EbookStateRole
                                                                              : WorkModel::AudiobookStateRole);
        }
        break;
    default:
        break;
    }
    return QSortFilterProxyModel::data(index, role);
}

void WorkFilterModel::setSearchText(const QString &text)
{
    if (text == m_searchText)
        return;
    m_searchText = text;
    m_searchTerms = TextUtils::searchTerms(text);
    filtersChanged();
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

void WorkFilterModel::setGroupSeries(bool group)
{
    if (group == m_groupSeries)
        return;
    m_groupSeries = group;
    filtersChanged();
    emit groupSeriesChanged();
}

void WorkFilterModel::setSeriesFilter(const QString &series)
{
    if (series == m_seriesFilter)
        return;
    m_seriesFilter = series;
    m_seriesFilterKey = seriesKey(series);
    rebuildStacks();
    invalidate(); // the volumes of a series have their own order
    updateFormatCounts();
    onRowsChanged();
    emit seriesFilterChanged();
}

void WorkFilterModel::setSeriesOnly(bool only)
{
    if (only == m_seriesOnly)
        return;
    m_seriesOnly = only;
    filtersChanged();
    emit seriesOnlyChanged();
}

void WorkFilterModel::setOnlyIds(const QStringList &ids)
{
    if (ids == m_onlyIds)
        return;
    m_onlyIds = ids;
    rebuildStacks();
    invalidate(); // the order of the list
    updateFormatCounts();
    onRowsChanged();
    emit onlyIdsChanged();
}

void WorkFilterModel::filtersChanged()
{
    if (rebuildStacks()) {
        invalidate(); // stacks also sort differently than their first volume
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateRowsFilter();
#endif
    }
    updateFormatCounts();
    onRowsChanged();
}

void WorkFilterModel::onSourceChanged()
{
    if (rebuildStacks())
        invalidate();
    updateFormatCounts();
    onRowsChanged();
    // E.g. the formats of the selected work changed (canMergeSelection).
    if (!m_selected.isEmpty())
        emit selectionChanged();
}

void WorkFilterModel::onRowsChanged()
{
    updateCount();
    pruneSelection();
}

void WorkFilterModel::updateCount()
{
    const int newCount = rowCount();
    if (newCount == m_count)
        return;
    m_count = newCount;
    emit countChanged();
}

bool WorkFilterModel::stacking() const
{
    return m_groupSeries && m_seriesFilterKey.isEmpty() && m_onlyIds.isEmpty();
}

const WorkFilterModel::Stack *WorkFilterModel::stackOf(int sourceRow) const
{
    const auto it = m_stacks.constFind(sourceRow);
    return it == m_stacks.constEnd() ? nullptr : &*it;
}

bool WorkFilterModel::rebuildStacks()
{
    const QAbstractItemModel *model = sourceModel();
    const int rows = model ? model->rowCount() : 0;
    QList<int> leadOfRow(rows, -1);
    QHash<int, Stack> stacks;
    int workCount = 0;

    QHash<QString, QList<int>> rowsOfSeries;
    QStringList seriesInOrder; // so the result does not depend on hashing
    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = model->index(row, 0);
        if (!matchesFilters(index))
            continue;
        ++workCount;
        if (!stacking())
            continue;
        const QString key = seriesKey(index.data(WorkModel::SeriesRole).toString());
        if (key.isEmpty())
            continue;
        if (!rowsOfSeries.contains(key))
            seriesInOrder.append(key);
        rowsOfSeries[key].append(row);
    }

    // A single volume is only a stack where everything is a series.
    const int smallest = m_seriesOnly ? 1 : 2;
    for (const QString &key : std::as_const(seriesInOrder)) {
        const QList<int> members = rowsOfSeries.value(key);
        if (members.size() < smallest)
            continue;

        // The first volume stands for the series: lowest number, unnumbered last.
        const auto number = [model](int row) {
            const int index = model->index(row, 0).data(WorkModel::SeriesIndexRole).toInt();
            return index > 0 ? index : std::numeric_limits<int>::max();
        };
        const int lead = *std::min_element(members.cbegin(), members.cend(), [&](int a, int b) {
            if (number(a) != number(b))
                return number(a) < number(b);
            const int byTitle = compareText(model->index(a, 0), model->index(b, 0), WorkModel::TitleRole);
            return byTitle != 0 ? byTitle < 0 : a < b;
        });

        Stack stack;
        stack.name = model->index(lead, 0).data(WorkModel::SeriesRole).toString();
        stack.count = int(members.size());
        for (int row : members) {
            const QModelIndex index = model->index(row, 0);
            const int volume = index.data(WorkModel::SeriesIndexRole).toInt();
            if (volume > 0) {
                stack.first = stack.first == 0 ? volume : std::min(stack.first, volume);
                stack.last = std::max(stack.last, volume);
            }
            stack.hasEbook = stack.hasEbook || index.data(WorkModel::HasEbookRole).toBool();
            stack.hasAudiobook = stack.hasAudiobook || index.data(WorkModel::HasAudiobookRole).toBool();
            const QDateTime opened = index.data(WorkModel::LastOpenedRole).toDateTime();
            if (opened.isValid() && (!stack.lastOpened.isValid() || opened > stack.lastOpened))
                stack.lastOpened = opened;
            leadOfRow[row] = lead;
        }
        stacks.insert(lead, stack);
    }

    // Without stacks before and after nothing changed, whatever the rows are.
    const bool changed = !(stacks.isEmpty() && m_stacks.isEmpty())
                         && (leadOfRow != m_leadOfRow || stacks != m_stacks);
    m_leadOfRow = leadOfRow;
    m_stacks = stacks;
    if (workCount != m_workCount) {
        m_workCount = workCount;
        emit workCountChanged();
    }
    return changed;
}

bool WorkFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!matchesFilters(index))
        return false;
    // Of a stack only the first volume is shown.
    const int lead = m_leadOfRow.value(sourceRow, -1);
    return lead < 0 || lead == sourceRow;
}

bool WorkFilterModel::matchesFilters(const QModelIndex &sourceIndex) const
{
    if (!m_onlyIds.isEmpty())
        return m_onlyIds.contains(sourceIndex.data(WorkModel::WorkIdRole).toString());
    return matchesFormat(sourceIndex) && matchesSearch(sourceIndex) && matchesSeries(sourceIndex);
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

bool WorkFilterModel::matchesSeries(const QModelIndex &sourceIndex) const
{
    if (m_seriesFilterKey.isEmpty() && !m_seriesOnly)
        return true;
    const QString key = seriesKey(sourceIndex.data(WorkModel::SeriesRole).toString());
    return m_seriesFilterKey.isEmpty() ? !key.isEmpty() : key == m_seriesFilterKey;
}

int WorkFilterModel::compareText(const QString &left, const QString &right) const
{
    return m_collator.compare(left, right);
}

int WorkFilterModel::compareText(const QModelIndex &left, const QModelIndex &right, int role) const
{
    return compareText(left.data(role).toString(), right.data(role).toString());
}

bool WorkFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // A given list: its order.
    if (!m_onlyIds.isEmpty()) {
        return m_onlyIds.indexOf(left.data(WorkModel::WorkIdRole).toString())
             < m_onlyIds.indexOf(right.data(WorkModel::WorkIdRole).toString());
    }
    // The volumes of one series: by number, unnumbered ones at the end.
    if (!m_seriesFilterKey.isEmpty()) {
        const int a = left.data(WorkModel::SeriesIndexRole).toInt();
        const int b = right.data(WorkModel::SeriesIndexRole).toInt();
        if (a != b)
            return b == 0 || (a != 0 && a < b);
        const int byTitle = compareText(left, right, WorkModel::TitleRole);
        return byTitle != 0 ? byTitle < 0 : compareText(left, right, WorkModel::AuthorSortRole) < 0;
    }

    const Stack *leftStack = stackOf(left.row());
    const Stack *rightStack = stackOf(right.row());
    switch (m_sortOrder) {
    case LastOpened: {
        const QDateTime a = leftStack ? leftStack->lastOpened : left.data(WorkModel::LastOpenedRole).toDateTime();
        const QDateTime b = rightStack ? rightStack->lastOpened : right.data(WorkModel::LastOpenedRole).toDateTime();
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
    // Title (of a stack: the series) as the (secondary) order, then the
    // author for equal titles.
    const int byTitle = compareText(leftStack ? leftStack->name : left.data(WorkModel::TitleRole).toString(),
                                    rightStack ? rightStack->name : right.data(WorkModel::TitleRole).toString());
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
            if (!matchesSearch(index) || !matchesSeries(index))
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

// --- selection ---------------------------------------------------------------

QStringList WorkFilterModel::selectedIds() const
{
    QStringList ids;
    if (m_selected.isEmpty())
        return ids;
    for (int row = 0; row < rowCount(); ++row) {
        const QString id = workIdAt(row);
        if (m_selected.contains(id))
            ids.append(id);
    }
    return ids;
}

bool WorkFilterModel::canMergeSelection() const
{
    if (m_selected.size() >= 2)
        return true;
    if (m_selected.isEmpty())
        return false;
    // One work: only if it has audiobook files to join into one audiobook.
    const auto *works = qobject_cast<const WorkModel *>(sourceModel());
    const Work *work = works ? works->workAt(works->rowOf(*m_selected.cbegin())) : nullptr;
    if (!work)
        return false;
    return std::count_if(work->formats.cbegin(), work->formats.cend(),
                         [](const Format &format) { return format.kind == Format::Kind::Audiobook; }) >= 2;
}

void WorkFilterModel::toggleSelected(const QString &workId)
{
    const int row = rowOf(workId);
    if (row < 0 || isStack(row))
        return;
    if (!m_selected.remove(workId))
        m_selected.insert(workId);
    emit dataChanged(index(row, 0), index(row, 0), { SelectedRole });
    emit selectionChanged();
}

void WorkFilterModel::selectAll()
{
    const qsizetype before = m_selected.size();
    for (int row = 0; row < rowCount(); ++row) {
        if (!isStack(row))
            m_selected.insert(workIdAt(row));
    }
    if (m_selected.size() == before)
        return;
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { SelectedRole });
    emit selectionChanged();
}

void WorkFilterModel::clearSelection()
{
    if (m_selected.isEmpty())
        return;
    m_selected.clear();
    if (rowCount() > 0)
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { SelectedRole });
    emit selectionChanged();
}

void WorkFilterModel::pruneSelection()
{
    if (m_selected.isEmpty())
        return;
    QSet<QString> shown;
    for (int row = 0; row < rowCount(); ++row) {
        if (!isStack(row))
            shown.insert(workIdAt(row));
    }
    const qsizetype before = m_selected.size();
    m_selected.intersect(shown);
    if (m_selected.size() == before)
        return;
    // A work that became the first volume of a stack is still a row.
    if (rowCount() > 0)
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { SelectedRole });
    emit selectionChanged();
}
