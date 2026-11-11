#include "metadatareader.h"

#include <QFileInfo>

#include "audioreader.h"
#include "epubreader.h"
#include "filenameparser.h"
#include "pdfreader.h"

QStringList ebookSuffixes()
{
    return { QStringLiteral("epub"), QStringLiteral("pdf"), QStringLiteral("mobi"), QStringLiteral("azw3") };
}

QStringList audiobookSuffixes()
{
    return { QStringLiteral("m4b"), QStringLiteral("m4a"), QStringLiteral("mp3"), QStringLiteral("aac"),
             QStringLiteral("ogg"), QStringLiteral("opus"), QStringLiteral("flac") };
}

bool isSupportedFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return ebookSuffixes().contains(suffix) || audiobookSuffixes().contains(suffix);
}

FileMetadata readMetadata(const QString &path)
{
    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();

    FileMetadata metadata;
    metadata.sourcePath = info.absoluteFilePath();
    metadata.fileSize = info.size();
    metadata.kind = audiobookSuffixes().contains(suffix) ? Format::Kind::Audiobook : Format::Kind::Ebook;

    if (!info.isFile()) {
        metadata.error = QStringLiteral("file not found");
        return metadata;
    }

    if (suffix == QLatin1String("epub"))
        readEpub(metadata.sourcePath, metadata);
    else if (suffix == QLatin1String("pdf"))
        readPdf(metadata.sourcePath, metadata);
    else if (metadata.kind == Format::Kind::Audiobook)
        readAudio(metadata.sourcePath, metadata);
    // MOBI / AZW3: no reader yet, the file name is used.

    // Titles that carry the file naming scheme ("DCC 01 - Dungeon Crawler
    // Carl") are cleaned up; the number becomes the series number.
    if (!metadata.title.isEmpty()) {
        const FileNameInfo fromTitle = parseFileName(metadata.title);
        if (!fromTitle.seriesCode.isEmpty() && fromTitle.author.isEmpty()) {
            metadata.title = fromTitle.title;
            if (metadata.seriesIndex == 0)
                metadata.seriesIndex = fromTitle.seriesIndex;
            if (metadata.series.isEmpty())
                metadata.series = fromTitle.seriesCode;
        }
    }

    // Fill the gaps from the file name.
    const FileNameInfo fromName = parseFileName(info.completeBaseName());
    if (metadata.title.isEmpty())
        metadata.title = fromName.title;
    if (metadata.authors.isEmpty() && !fromName.author.isEmpty())
        metadata.authors = { fromName.author };
    if (metadata.seriesIndex == 0)
        metadata.seriesIndex = fromName.seriesIndex;
    if (metadata.series.isEmpty() && !fromName.seriesCode.isEmpty())
        metadata.series = fromName.seriesCode;

    // A broken file is still imported with what the file name tells.
    return metadata;
}
