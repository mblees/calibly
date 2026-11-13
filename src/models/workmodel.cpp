// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "workmodel.h"

#include <QUrl>

#include "textutils.h"

#include <utility>

namespace {

QUrl coverUrl(const QString &path)
{
    if (path.isEmpty())
        return QUrl();
    // Paths starting with ":" are embedded resources.
    return path.startsWith(u':') ? QUrl(QStringLiteral("qrc") + path) : QUrl::fromLocalFile(path);
}

} // namespace

WorkModel::WorkModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WorkModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant WorkModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};

    const Work &work = m_works.at(index.row());
    switch (role) {
    case WorkIdRole:
        return work.id;
    case Qt::DisplayRole:
    case TitleRole:
        return work.title;
    case AuthorsRole:
        return work.authors;
    case AuthorTextRole:
        return work.authors.join(QStringLiteral(", "));
    case NarratorsRole:
        return work.narrators;
    case SeriesRole:
        return work.series;
    case SeriesIndexRole:
        return work.seriesIndex;
    case CoverUrlRole:
        return coverUrl(work.coverPath(work.primaryKind()));
    case CoverIsSquareRole:
        return work.coverIsSquare(work.primaryKind());
    case EbookCoverUrlRole:
        return coverUrl(work.coverPath(Format::Kind::Ebook));
    case EbookCoverIsSquareRole:
        return work.coverIsSquare(Format::Kind::Ebook);
    case AudiobookCoverUrlRole:
        return coverUrl(work.coverPath(Format::Kind::Audiobook));
    case AudiobookCoverIsSquareRole:
        return work.coverIsSquare(Format::Kind::Audiobook);
    case HasEbookRole:
        return work.hasEbook();
    case HasAudiobookRole:
        return work.hasAudiobook();
    case ReadProgressRole:
        return work.readProgress();
    case ListenProgressRole:
        return work.listenProgress();
    case LastOpenedRole:
        return work.lastOpened;
    case WorkStateRole:
        return int(work.state());
    case EbookStateRole:
        return int(work.state(Format::Kind::Ebook));
    case AudiobookStateRole:
        return int(work.state(Format::Kind::Audiobook));
    case CanSplitRole:
        return work.canSplit();
    case AuthorSortRole:
        return work.authors.isEmpty() ? QString() : TextUtils::nameSortKey(work.authors.first());
    case SearchTextRole:
        return m_searchTexts.at(index.row());
    default:
        return {};
    }
}

QHash<int, QByteArray> WorkModel::roleNames() const
{
    return {
        { WorkIdRole, "workId" },
        { TitleRole, "title" },
        { AuthorsRole, "authors" },
        { AuthorTextRole, "authorText" },
        { NarratorsRole, "narrators" },
        { SeriesRole, "series" },
        { SeriesIndexRole, "seriesIndex" },
        { CoverUrlRole, "coverUrl" },
        { CoverIsSquareRole, "coverIsSquare" },
        { HasEbookRole, "hasEbook" },
        { HasAudiobookRole, "hasAudiobook" },
        { ReadProgressRole, "readProgress" },
        { ListenProgressRole, "listenProgress" },
        { LastOpenedRole, "lastOpened" },
        { AuthorSortRole, "authorSort" },
        { SearchTextRole, "searchText" },
        { EbookCoverUrlRole, "ebookCoverUrl" },
        { EbookCoverIsSquareRole, "ebookCoverIsSquare" },
        { AudiobookCoverUrlRole, "audiobookCoverUrl" },
        { AudiobookCoverIsSquareRole, "audiobookCoverIsSquare" },
        { WorkStateRole, "workState" },
        { EbookStateRole, "ebookState" },
        { AudiobookStateRole, "audiobookState" },
        { CanSplitRole, "canSplit" },
    };
}

void WorkModel::updateWork(const Work &work)
{
    const QStringList parts = QStringList{ work.title, work.series } + work.authors + work.narrators + work.tags;
    const QString searchText = TextUtils::foldForSearch(parts.join(u' '));
    const int row = rowOf(work.id);
    if (row < 0) {
        beginInsertRows(QModelIndex(), count(), count());
        m_works.append(work);
        m_searchTexts.append(searchText);
        endInsertRows();
        emit countChanged();
        return;
    }
    m_works[row] = work;
    m_searchTexts[row] = searchText;
    emit dataChanged(index(row), index(row));
}

bool WorkModel::removeWork(const QString &workId)
{
    const int row = rowOf(workId);
    if (row < 0)
        return false;
    beginRemoveRows(QModelIndex(), row, row);
    m_works.removeAt(row);
    m_searchTexts.removeAt(row);
    endRemoveRows();
    emit countChanged();
    return true;
}

int WorkModel::rowOf(const QString &workId) const
{
    for (int row = 0; row < m_works.size(); ++row) {
        if (m_works.at(row).id == workId)
            return row;
    }
    return -1;
}

const Work *WorkModel::workAt(int row) const
{
    return row >= 0 && row < m_works.size() ? &m_works.at(row) : nullptr;
}

void WorkModel::setWorks(QList<Work> works)
{
    const int oldCount = count();
    beginResetModel();
    m_works = std::move(works);
    m_searchTexts.clear();
    m_searchTexts.reserve(m_works.size());
    for (const Work &work : std::as_const(m_works)) {
        const QStringList parts = QStringList{ work.title, work.series } + work.authors + work.narrators + work.tags;
        m_searchTexts.append(TextUtils::foldForSearch(parts.join(u' ')));
    }
    endResetModel();
    if (count() != oldCount)
        emit countChanged();
}
