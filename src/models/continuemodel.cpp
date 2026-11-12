#include "continuemodel.h"

#include <QCollator>

#include <algorithm>
#include <cmath>

#include "audiotimeline.h"
#include "readingposition.h"

ContinueModel::ContinueModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContinueModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant ContinueModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case WorkIdRole:
        return entry.workId;
    case Qt::DisplayRole:
    case TitleRole:
        return entry.title;
    case AuthorTextRole:
        return entry.authorText;
    case KindRole:
        return entry.kind == Format::Kind::Audiobook ? QStringLiteral("audiobook") : QStringLiteral("ebook");
    case CoverUrlRole:
        return entry.coverUrl;
    case CoverIsSquareRole:
        return entry.coverIsSquare;
    case ProgressRole:
        return entry.progress;
    case LastOpenedRole:
        return entry.lastOpened;
    case ChapterNumberRole:
        return entry.chapterNumber;
    case ChapterTitleRole:
        return entry.chapterTitle;
    case RemainingSecondsRole:
        return entry.remainingSeconds;
    case PageRole:
        return entry.page;
    case PagesRole:
        return entry.pages;
    case HasOtherFormatRole:
        return entry.hasOtherFormat;
    case AvailableRole:
        return entry.available;
    default:
        return {};
    }
}

QHash<int, QByteArray> ContinueModel::roleNames() const
{
    return {
        { WorkIdRole, "workId" },
        { TitleRole, "title" },
        { AuthorTextRole, "authorText" },
        { KindRole, "kind" },
        { CoverUrlRole, "coverUrl" },
        { CoverIsSquareRole, "coverIsSquare" },
        { ProgressRole, "progress" },
        { LastOpenedRole, "lastOpened" },
        { ChapterNumberRole, "chapterNumber" },
        { ChapterTitleRole, "chapterTitle" },
        { RemainingSecondsRole, "remainingSeconds" },
        { PageRole, "page" },
        { PagesRole, "pages" },
        { HasOtherFormatRole, "hasOtherFormat" },
        { AvailableRole, "available" },
    };
}

void ContinueModel::setSourceModel(WorkModel *model)
{
    if (model == m_source)
        return;
    for (const QMetaObject::Connection &connection : std::as_const(m_connections))
        disconnect(connection);
    m_connections.clear();

    m_source = model;
    if (m_source) {
        const auto update = [this] { rebuild(); };
        m_connections = {
            connect(m_source, &QAbstractItemModel::modelReset, this, update),
            connect(m_source, &QAbstractItemModel::rowsInserted, this, update),
            connect(m_source, &QAbstractItemModel::rowsRemoved, this, update),
            connect(m_source, &QAbstractItemModel::dataChanged, this, update),
        };
    }
    emit sourceModelChanged();
    rebuild();
}

void ContinueModel::setFormatFilter(WorkFilterModel::FormatFilter filter)
{
    if (filter == m_formatFilter)
        return;
    m_formatFilter = filter;
    emit formatFilterChanged();
    rebuild();
}

void ContinueModel::setLimit(int limit)
{
    limit = std::max(0, limit);
    if (limit == m_limit)
        return;
    m_limit = limit;
    emit limitChanged();
    rebuild();
}

int ContinueModel::formatToContinue(const Work &work) const
{
    if (m_formatFilter == WorkFilterModel::BothFormats && !(work.hasEbook() && work.hasAudiobook()))
        return -1;
    int best = -1;
    for (int i = 0; i < work.formats.size(); ++i) {
        const Format &format = work.formats.at(i);
        if (m_formatFilter == WorkFilterModel::Ebook && format.kind != Format::Kind::Ebook)
            continue;
        if (m_formatFilter == WorkFilterModel::Audiobook && format.kind != Format::Kind::Audiobook)
            continue;
        const bool started = format.lastOpened.isValid() || format.progress > 0.0;
        if (!started || format.finished())
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        const Format &current = work.formats.at(best);
        // Opened at the same time (e.g. taken over from the work): the
        // audiobook, because its position is known exactly.
        if (format.lastOpened > current.lastOpened
            || (format.lastOpened == current.lastOpened && format.kind == Format::Kind::Audiobook
                && current.kind != Format::Kind::Audiobook)) {
            best = i;
        }
    }
    return best;
}

QList<ContinueModel::Entry> ContinueModel::entries() const
{
    QList<Entry> result;
    if (!m_source)
        return result;

    for (int row = 0; row < m_source->count(); ++row) {
        const Work &work = *m_source->workAt(row);
        const int formatIndex = formatToContinue(work);
        if (formatIndex < 0)
            continue;
        const Format &format = work.formats.at(formatIndex);
        const bool audiobook = format.kind == Format::Kind::Audiobook;
        const QModelIndex index = m_source->index(row);

        Entry entry;
        entry.workId = work.id;
        entry.title = work.title;
        entry.authorText = index.data(WorkModel::AuthorTextRole).toString();
        entry.kind = format.kind;
        entry.coverUrl = index.data(audiobook ? WorkModel::AudiobookCoverUrlRole : WorkModel::EbookCoverUrlRole).toUrl();
        entry.coverIsSquare = work.coverIsSquare(format.kind);
        entry.progress = format.progress;
        entry.lastOpened = format.lastOpened;
        entry.hasOtherFormat = audiobook ? work.hasEbook() : work.hasAudiobook();
        entry.available = !format.fileMissing;
        if (audiobook) {
            const AudioTimeline timeline(format);
            const qint64 durationMs = timeline.durationMs() > 0 ? timeline.durationMs()
                                                                : qint64(format.durationSeconds) * 1000;
            const qint64 positionMs = format.positionMs > 0 ? format.positionMs
                                                            : qint64(std::llround(format.progress * double(durationMs)));
            entry.remainingSeconds = int(std::max<qint64>(0, durationMs - positionMs) / 1000);
            if (timeline.chapters().size() > 1) {
                const int chapter = timeline.chapterAt(positionMs);
                entry.chapterNumber = chapter + 1;
                entry.chapterTitle = timeline.chapters().at(chapter).title;
            }
        } else {
            // Read in calibly: the stored place; a PDF knows its page, an
            // EPUB the chapter (its pages depend on the text settings).
            const ReadingPosition position = ReadingPosition::fromString(format.readingPosition);
            entry.pages = format.pages;
            if (position.page >= 0) {
                entry.page = position.page + 1;
            } else if (position.chapter >= 0) {
                entry.chapterTitle = position.label;
            } else if (format.pages > 0 && format.progress > 0.0) {
                entry.page = std::max(1, int(std::lround(format.pages * format.progress)));
            }
        }
        result.append(entry);
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(result.begin(), result.end(), [&collator](const Entry &a, const Entry &b) {
        if (a.lastOpened != b.lastOpened)
            return a.lastOpened > b.lastOpened; // most recent first
        return collator.compare(a.title, b.title) < 0;
    });
    if (m_limit > 0 && result.size() > m_limit)
        result.resize(m_limit);
    return result;
}

void ContinueModel::rebuild()
{
    QList<Entry> updated = entries();

    // The same rows in the same order (e.g. the player saved its position):
    // only their data changed, so views keep their place.
    const bool sameRows = updated.size() == m_entries.size()
        && std::equal(updated.cbegin(), updated.cend(), m_entries.cbegin(), [](const Entry &a, const Entry &b) {
               return a.workId == b.workId && a.kind == b.kind;
           });
    if (sameRows) {
        m_entries = std::move(updated);
        if (!m_entries.isEmpty())
            emit dataChanged(index(0), index(count() - 1));
        return;
    }

    const int oldCount = count();
    beginResetModel();
    m_entries = std::move(updated);
    endResetModel();
    if (count() != oldCount)
        emit countChanged();
}
