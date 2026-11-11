#include "audioreader.h"

#include <QFile>

#include "textutils.h"

#include <fileref.h>
#include <tpropertymap.h>
#include <tvariant.h>

namespace {

QString toQString(const TagLib::String &string)
{
    return QString::fromStdWString(string.toWString()).simplified();
}

// First value of the first of these tag names that is set.
QString firstValue(const TagLib::PropertyMap &properties, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const auto it = properties.find(key);
        if (it != properties.end() && !it->second.isEmpty()) {
            const QString value = toQString(it->second.front());
            if (!value.isEmpty())
                return value;
        }
    }
    return QString();
}

} // namespace

void readAudio(const QString &path, FileMetadata &metadata)
{
#ifdef Q_OS_WIN
    TagLib::FileRef file(reinterpret_cast<const wchar_t *>(path.utf16()));
#else
    TagLib::FileRef file(QFile::encodeName(path).constData());
#endif
    if (file.isNull()) {
        metadata.error = QStringLiteral("not a readable audio file");
        return;
    }

    if (const TagLib::AudioProperties *audio = file.audioProperties())
        metadata.durationSeconds = audio->lengthInSeconds();

    const TagLib::PropertyMap properties = file.properties();
    metadata.title = firstValue(properties, { "ALBUM", "TITLE" });
    metadata.authors = TextUtils::splitNames(firstValue(properties, { "ALBUMARTIST", "ARTIST" }));
    metadata.narrators = TextUtils::splitNames(firstValue(properties, { "NARRATOR", "NARRATEDBY", "COMPOSER" }));
    metadata.series = firstValue(properties, { "SERIES", "MOVEMENTNAME" });
    metadata.seriesIndex = int(firstValue(properties, { "SERIES-PART", "SERIESPART", "MOVEMENT", "MOVEMENTNUMBER" }).toDouble());
    metadata.description = firstValue(properties, { "DESCRIPTION", "LONGDESCRIPTION", "COMMENT" });
    metadata.publisher = firstValue(properties, { "LABEL", "PUBLISHER" });
    metadata.isbn = firstValue(properties, { "ISBN" });
    metadata.publishedYear = firstValue(properties, { "DATE", "ORIGINALDATE" }).left(4).toInt();
    metadata.language = firstValue(properties, { "LANGUAGE" }).section(u'-', 0, 0).toLower().left(3);
    const QString genre = firstValue(properties, { "GENRE" });
    if (!genre.isEmpty() && genre.compare(QLatin1String("Audiobook"), Qt::CaseInsensitive) != 0
        && genre.compare(QStringLiteral("Hörbuch"), Qt::CaseInsensitive) != 0)
        metadata.tags.append(genre);

    // Embedded cover (TagLib 2: generic "PICTURE" complex property).
    const auto pictures = file.complexProperties("PICTURE");
    if (!pictures.isEmpty()) {
        const TagLib::ByteVector data = pictures.front().value("data").toByteVector();
        metadata.cover.loadFromData(reinterpret_cast<const uchar *>(data.data()), int(data.size()));
    }
}
