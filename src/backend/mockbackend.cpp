#include "mockbackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <utility>

namespace {

QStringList toStringList(const QJsonValue &value)
{
    QStringList list;
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array)
        list.append(item.toString());
    return list;
}

bool isSquareImage(const QString &path)
{
    const QSize size = QImageReader(path).size();
    if (!size.isValid() || size.height() == 0)
        return false;
    const double ratio = double(size.width()) / size.height();
    return ratio > 0.9 && ratio < 1.1;
}

} // namespace

MockBackend::MockBackend(QString jsonPath)
    : m_jsonPath(std::move(jsonPath))
{
}

QList<Work> MockBackend::loadWorks()
{
    if (!m_loaded) {
        m_works = readJson();
        m_loaded = true;
    }
    return m_works;
}

bool MockBackend::saveWork(const Work &work)
{
    for (Work &existing : m_works) {
        if (existing.id == work.id) {
            existing = work;
            return true;
        }
    }
    m_works.append(work);
    return true;
}

QList<Work> MockBackend::readJson() const
{
    QFile file(m_jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("MockBackend: cannot open %s", qPrintable(m_jsonPath));
        return {};
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning("MockBackend: %s: %s", qPrintable(m_jsonPath), qPrintable(error.errorString()));
        return {};
    }

    const QDir jsonDir = QFileInfo(m_jsonPath).dir();
    const QString coverDir = jsonDir.path() + QStringLiteral("/covers/");
    QList<Work> works;

    const QJsonArray items = document.object().value(QStringLiteral("works")).toArray();
    for (const QJsonValue &itemValue : items) {
        const QJsonObject item = itemValue.toObject();

        Work work;
        work.id = item.value(QStringLiteral("id")).toString();
        work.title = item.value(QStringLiteral("title")).toString();
        work.authors = toStringList(item.value(QStringLiteral("authors")));
        work.narrators = toStringList(item.value(QStringLiteral("narrators")));
        work.series = item.value(QStringLiteral("series")).toString();
        work.seriesIndex = item.value(QStringLiteral("seriesIndex")).toInt();
        work.description = item.value(QStringLiteral("description")).toString();
        work.tags = toStringList(item.value(QStringLiteral("tags")));
        work.rating = std::clamp(item.value(QStringLiteral("rating")).toInt(), 0, 5);
        work.language = item.value(QStringLiteral("language")).toString();
        work.publishedYear = item.value(QStringLiteral("published")).toInt();
        // A "publisher" of the work applies to all its formats that have none of their own.
        const QString publisher = item.value(QStringLiteral("publisher")).toString();
        work.added = QDateTime::fromString(item.value(QStringLiteral("added")).toString(), Qt::ISODate);
        work.lastOpened = QDateTime::fromString(item.value(QStringLiteral("lastOpened")).toString(),
                                                Qt::ISODate);

        const QJsonArray formats = item.value(QStringLiteral("formats")).toArray();
        for (const QJsonValue &formatValue : formats) {
            const QJsonObject formatObject = formatValue.toObject();
            Format format;
            format.kind = formatObject.value(QStringLiteral("kind")).toString() == QLatin1String("audiobook")
                              ? Format::Kind::Audiobook
                              : Format::Kind::Ebook;
            const QString file = formatObject.value(QStringLiteral("file")).toString();
            format.filePath = file.isEmpty() || QDir::isAbsolutePath(file) ? file : jsonDir.filePath(file);
            format.progress = std::clamp(formatObject.value(QStringLiteral("progress")).toDouble(), 0.0, 1.0);
            format.fileSize = formatObject.value(QStringLiteral("size")).toInteger();
            if (format.fileSize == 0)
                format.fileSize = QFileInfo(format.filePath).size(); // 0 if missing
            format.pages = formatObject.value(QStringLiteral("pages")).toInt();
            format.durationSeconds = formatObject.value(QStringLiteral("duration")).toInt();
            format.publisher = formatObject.value(QStringLiteral("publisher")).toString(publisher);
            format.isbn = formatObject.value(QStringLiteral("isbn")).toString();
            // Chapters: [{ "title", "start" and "duration" in seconds, optional "file" }]
            const QJsonArray chapters = formatObject.value(QStringLiteral("chapters")).toArray();
            for (const QJsonValue &chapterValue : chapters) {
                const QJsonObject chapterObject = chapterValue.toObject();
                const QString chapterFile = chapterObject.value(QStringLiteral("file")).toString();
                format.chapters.append({ chapterObject.value(QStringLiteral("title")).toString(),
                                         chapterFile.isEmpty() || QDir::isAbsolutePath(chapterFile)
                                             ? chapterFile : jsonDir.filePath(chapterFile),
                                         qint64(chapterObject.value(QStringLiteral("start")).toDouble() * 1000),
                                         qint64(chapterObject.value(QStringLiteral("duration")).toDouble() * 1000) });
            }
            format.positionMs = qint64(format.progress * format.durationSeconds * 1000);
            const QString cover = formatObject.value(QStringLiteral("cover")).toString();
            if (!cover.isEmpty()) {
                format.coverPath = coverDir + cover;
                format.coverIsSquare = isSquareImage(format.coverPath);
            }
            work.formats.append(format);
        }

        works.append(work);
    }
    return works;
}
