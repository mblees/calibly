#include "workmodel.h"

#include <QUrl>

#include <utility>

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
        if (work.coverPath.isEmpty())
            return QUrl();
        // Paths starting with ":" are embedded resources.
        return work.coverPath.startsWith(u':') ? QUrl(QStringLiteral("qrc") + work.coverPath)
                                               : QUrl::fromLocalFile(work.coverPath);
    case CoverIsSquareRole:
        return work.coverIsSquare;
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
    };
}

void WorkModel::setWorks(QList<Work> works)
{
    const int oldCount = count();
    beginResetModel();
    m_works = std::move(works);
    endResetModel();
    if (count() != oldCount)
        emit countChanged();
}
