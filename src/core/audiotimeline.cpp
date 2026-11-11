#include "audiotimeline.h"

#include <algorithm>

AudioTimeline::AudioTimeline(const Format &format)
{
    if (format.chapters.isEmpty()) {
        if (format.filePath.isEmpty())
            return;
        m_durationMs = qint64(format.durationSeconds) * 1000;
        m_parts.append({ format.filePath, 0, m_durationMs });
        m_chapters.append({ QString(), 0, m_durationMs, 0 });
        return;
    }

    for (const Chapter &chapter : format.chapters) {
        const QString path = chapter.filePath.isEmpty() ? format.filePath : chapter.filePath;
        if (m_parts.isEmpty() || m_parts.last().filePath != path) {
            const qint64 start = m_parts.isEmpty() ? 0 : m_parts.last().startMs + m_parts.last().durationMs;
            m_parts.append({ path, start, 0 });
        }
        Part &part = m_parts.last();
        const qint64 start = part.startMs + chapter.startMs;
        const qint64 duration = std::max<qint64>(0, chapter.durationMs);
        part.durationMs = std::max(part.durationMs, chapter.startMs + duration);
        m_chapters.append({ chapter.title, start, duration, int(m_parts.size()) - 1 });
    }
    m_durationMs = m_parts.last().startMs + m_parts.last().durationMs;
}

namespace {

// Index of the last item whose start is <= ms (clamped to the first one).
template <typename List>
int indexAt(const List &items, qint64 ms)
{
    if (items.isEmpty())
        return -1;
    const auto it = std::upper_bound(items.cbegin(), items.cend(), ms,
                                     [](qint64 value, const auto &item) { return value < item.startMs; });
    return std::max(0, int(it - items.cbegin()) - 1);
}

} // namespace

int AudioTimeline::chapterAt(qint64 ms) const
{
    return indexAt(m_chapters, ms);
}

int AudioTimeline::partAt(qint64 ms) const
{
    return indexAt(m_parts, ms);
}
