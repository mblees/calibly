#include "mockbackend.h"

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

    const QString coverDir = QFileInfo(m_jsonPath).path() + QStringLiteral("/covers/");
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
        work.lastOpened = QDateTime::fromString(item.value(QStringLiteral("lastOpened")).toString(),
                                                Qt::ISODate);

        const QJsonArray formats = item.value(QStringLiteral("formats")).toArray();
        for (const QJsonValue &formatValue : formats) {
            const QJsonObject formatObject = formatValue.toObject();
            Format format;
            format.kind = formatObject.value(QStringLiteral("kind")).toString() == QLatin1String("audiobook")
                              ? Format::Kind::Audiobook
                              : Format::Kind::Ebook;
            format.filePath = formatObject.value(QStringLiteral("file")).toString();
            format.progress = std::clamp(formatObject.value(QStringLiteral("progress")).toDouble(), 0.0, 1.0);
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
