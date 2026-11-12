#include "importqueuemodel.h"

#include <algorithm>

ImportQueueModel::ImportQueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ImportQueueModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant ImportQueueModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case EntryIdRole:
        return entry.id;
    case TitleRole:
        return entry.title;
    case StatusRole:
        return entry.status;
    case NewWorkRole:
        return entry.newWork;
    case FileTypesRole:
        return entry.fileTypes.join(QStringLiteral(" + "));
    case FileCountRole:
        return entry.fileCount;
    case MergedRole:
        return entry.merged;
    case HasEbookRole:
        return entry.hasEbook;
    case HasAudiobookRole:
        return entry.hasAudiobook;
    case ProgressRole:
        return entry.progress;
    case CoverUrlRole:
        return entry.coverUrl;
    case CoverIsSquareRole:
        return entry.coverIsSquare;
    case WorkIdRole:
        return entry.workId;
    default:
        return {};
    }
}

QHash<int, QByteArray> ImportQueueModel::roleNames() const
{
    return {
        { EntryIdRole, "entryId" },
        { TitleRole, "title" },
        { StatusRole, "status" },
        { NewWorkRole, "newWork" },
        { FileTypesRole, "fileTypes" },
        { FileCountRole, "fileCount" },
        { MergedRole, "merged" },
        { HasEbookRole, "hasEbook" },
        { HasAudiobookRole, "hasAudiobook" },
        { ProgressRole, "progress" },
        { CoverUrlRole, "coverUrl" },
        { CoverIsSquareRole, "coverIsSquare" },
        { WorkIdRole, "workId" },
    };
}

int ImportQueueModel::finishedCount() const
{
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(),
                             [](const Entry &entry) { return !isRunning(entry.status); }));
}

int ImportQueueModel::attentionCount() const
{
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(), [](const Entry &entry) {
        return entry.status == NeedsReview || entry.status == Failed;
    }));
}

bool ImportQueueModel::active() const
{
    return std::any_of(m_entries.cbegin(), m_entries.cend(),
                       [](const Entry &entry) { return isRunning(entry.status); });
}

int ImportQueueModel::append(Entry entry)
{
    entry.id = m_nextId++;
    const int row = count();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append(entry);
    endInsertRows();
    emit countChanged();
    emit summaryChanged();
    return entry.id;
}

QList<int> ImportQueueModel::replace(int id, QList<Entry> entries)
{
    QList<int> ids;
    const int row = rowOf(id);
    if (row < 0)
        return ids;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();

    if (!entries.isEmpty()) {
        beginInsertRows(QModelIndex(), row, row + int(entries.size()) - 1);
        for (int i = 0; i < entries.size(); ++i) {
            entries[i].id = m_nextId++;
            ids.append(entries.at(i).id);
            m_entries.insert(row + i, entries.at(i));
        }
        endInsertRows();
    }
    if (entries.size() != 1)
        emit countChanged();
    emit summaryChanged();
    return ids;
}

void ImportQueueModel::update(int id, const std::function<void(Entry &)> &change)
{
    const int row = rowOf(id);
    if (row < 0)
        return;
    const Status before = m_entries.at(row).status;
    change(m_entries[row]);
    m_entries[row].id = id;
    emit dataChanged(index(row), index(row));
    if (m_entries.at(row).status != before)
        emit summaryChanged();
}

const ImportQueueModel::Entry *ImportQueueModel::entry(int id) const
{
    const int row = rowOf(id);
    return row >= 0 ? &m_entries.at(row) : nullptr;
}

void ImportQueueModel::markReviewed(int entryId)
{
    update(entryId, [](Entry &entry) {
        if (entry.status == NeedsReview)
            entry.status = Done;
    });
}

void ImportQueueModel::clearFinished()
{
    for (int row = count() - 1; row >= 0; --row) {
        if (isRunning(m_entries.at(row).status))
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_entries.removeAt(row);
        endRemoveRows();
    }
    emit countChanged();
    emit summaryChanged();
}

int ImportQueueModel::rowOf(int id) const
{
    for (int row = 0; row < m_entries.size(); ++row) {
        if (m_entries.at(row).id == id)
            return row;
    }
    return -1;
}
