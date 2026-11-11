#include "epubreader.h"

#include <QHash>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>

// QZipReader is a Qt-internal class (QtCore private API). It saves a separate
// ZIP library; if Qt ever removes it, only this file has to change.
#include <private/qzipreader_p.h>

namespace {

struct ManifestItem
{
    QString href;
    QString mediaType;
    QString properties;
};

// ISBN from a <dc:identifier>, e.g. "urn:isbn:978-3-…", "isbn:…" or an
// identifier with opf:scheme="ISBN"; empty if it is no ISBN (e.g. a UUID).
QString isbnFrom(const QString &identifier, const QString &scheme)
{
    QString value = identifier.trimmed();
    static const QRegularExpression prefix(QStringLiteral("^(urn:)?isbn:"), QRegularExpression::CaseInsensitiveOption);
    const bool marked = scheme.compare(QLatin1String("ISBN"), Qt::CaseInsensitive) == 0 || value.contains(prefix);
    value.remove(prefix);
    // ISBN-10 (last character may be X) or ISBN-13, with optional hyphens.
    static const QRegularExpression isbn(QStringLiteral("^(\\d{9}[\\dXx]|97[89]\\d{10})$"));
    const bool looksLikeIsbn = isbn.match(QString(value).remove(u'-').remove(u' ')).hasMatch();
    return marked || looksLikeIsbn ? value : QString();
}

QString plainText(QString html)
{
    static const QRegularExpression tags(QStringLiteral("<[^>]*>"));
    html.replace(QStringLiteral("<br"), QStringLiteral("\n<br"), Qt::CaseInsensitive);
    html.replace(QStringLiteral("</p>"), QStringLiteral("</p>\n"), Qt::CaseInsensitive);
    html.remove(tags);
    html.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    html.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    html.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    html.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    html.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
    html.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    return html.trimmed();
}

QString rootFilePath(const QByteArray &containerXml)
{
    QXmlStreamReader xml(containerXml);
    while (!xml.atEnd()) {
        if (xml.readNextStartElement() && xml.name() == QLatin1String("rootfile"))
            return xml.attributes().value(QStringLiteral("full-path")).toString();
    }
    return QString();
}

QString resolve(const QString &opfDir, const QString &href)
{
    const QString decoded = QUrl::fromPercentEncoding(href.toUtf8());
    return opfDir.isEmpty() ? decoded : opfDir + u'/' + decoded;
}

} // namespace

void readEpub(const QString &path, FileMetadata &metadata)
{
    QZipReader zip(path);
    if (!zip.isReadable() || zip.status() != QZipReader::NoError) {
        metadata.error = QStringLiteral("not a readable EPUB (ZIP) file");
        return;
    }

    const QString opfPath = rootFilePath(zip.fileData(QStringLiteral("META-INF/container.xml")));
    if (opfPath.isEmpty()) {
        metadata.error = QStringLiteral("EPUB without package document");
        return;
    }
    const QString opfDir = opfPath.contains(u'/') ? opfPath.section(u'/', 0, -2) : QString();

    QHash<QString, ManifestItem> manifest;
    QString coverId;
    QHash<QString, QString> collectionNames;     // EPUB 3: id -> series name
    QHash<QString, QString> collectionPositions; // EPUB 3: id -> position

    QXmlStreamReader xml(zip.fileData(opfPath));
    while (!xml.atEnd()) {
        if (!xml.readNextStartElement())
            continue;
        const QStringView name = xml.name();
        const QXmlStreamAttributes attributes = xml.attributes();

        if (name == QLatin1String("title") && metadata.title.isEmpty()) {
            metadata.title = xml.readElementText().simplified();
        } else if (name == QLatin1String("creator")) {
            const QString role = attributes.value(QStringLiteral("opf:role")).toString();
            const QString creator = xml.readElementText().simplified();
            if (!creator.isEmpty() && (role.isEmpty() || role == QLatin1String("aut")))
                metadata.authors.append(creator);
        } else if (name == QLatin1String("language") && metadata.language.isEmpty()) {
            metadata.language = xml.readElementText().trimmed().section(u'-', 0, 0).toLower();
        } else if (name == QLatin1String("identifier") && metadata.isbn.isEmpty()) {
            const QString scheme = attributes.value(QStringLiteral("opf:scheme")).toString();
            metadata.isbn = isbnFrom(xml.readElementText(), scheme);
        } else if (name == QLatin1String("publisher")) {
            metadata.publisher = xml.readElementText().simplified();
        } else if (name == QLatin1String("date") && metadata.publishedYear == 0) {
            metadata.publishedYear = xml.readElementText().trimmed().left(4).toInt();
        } else if (name == QLatin1String("description")) {
            metadata.description = plainText(xml.readElementText());
        } else if (name == QLatin1String("subject")) {
            const QString subject = xml.readElementText().simplified();
            if (!subject.isEmpty())
                metadata.tags.append(subject);
        } else if (name == QLatin1String("meta")) {
            const QString metaName = attributes.value(QStringLiteral("name")).toString();
            const QString content = attributes.value(QStringLiteral("content")).toString();
            const QString property = attributes.value(QStringLiteral("property")).toString();
            if (metaName == QLatin1String("cover")) {
                coverId = content;
            } else if (metaName == QLatin1String("calibre:series")) {
                metadata.series = content.simplified();
            } else if (metaName == QLatin1String("calibre:series_index")) {
                metadata.seriesIndex = int(content.toDouble());
            } else if (metaName == QLatin1String("calibre:rating")) {
                metadata.rating = qBound(0, int(content.toDouble() / 2 + 0.5), 5); // calibre: 0–10
            } else if (property == QLatin1String("belongs-to-collection")) {
                collectionNames.insert(attributes.value(QStringLiteral("id")).toString(),
                                       xml.readElementText().simplified());
            } else if (property == QLatin1String("group-position")) {
                collectionPositions.insert(attributes.value(QStringLiteral("refines")).toString().mid(1),
                                           xml.readElementText().trimmed());
            }
        } else if (name == QLatin1String("item")) {
            manifest.insert(attributes.value(QStringLiteral("id")).toString(),
                            { attributes.value(QStringLiteral("href")).toString(),
                              attributes.value(QStringLiteral("media-type")).toString(),
                              attributes.value(QStringLiteral("properties")).toString() });
        }
    }

    if (metadata.series.isEmpty() && !collectionNames.isEmpty()) {
        const QString id = collectionNames.constBegin().key();
        metadata.series = collectionNames.value(id);
        metadata.seriesIndex = int(collectionPositions.value(id).toDouble());
    }

    // Cover: EPUB 3 "cover-image" property, else the EPUB 2 <meta name="cover">.
    QString coverHref;
    for (auto it = manifest.cbegin(); it != manifest.cend() && coverHref.isEmpty(); ++it) {
        if (it->properties.split(u' ').contains(QLatin1String("cover-image")))
            coverHref = it->href;
    }
    if (coverHref.isEmpty() && manifest.contains(coverId))
        coverHref = manifest.value(coverId).href;
    if (!coverHref.isEmpty())
        metadata.cover.loadFromData(zip.fileData(resolve(opfDir, coverHref)));
}
