// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "epubbook.h"

#include <QRegularExpression>
#include <QStringDecoder>
#include <QXmlStreamReader>

// QZipReader is a Qt-internal class (QtCore private API), as in the import
// (see epubreader.cpp).
#include <private/qzipreader_p.h>

namespace {

// Named entities that XHTML files use without declaring them (the XML reader
// only knows the five XML ones).
QByteArray withNumericEntities(QByteArray xml)
{
    static const QList<std::pair<QByteArray, QByteArray>> entities = {
        { "&nbsp;", "&#160;" }, { "&shy;", "&#173;" }, { "&mdash;", "&#8212;" }, { "&ndash;", "&#8211;" },
        { "&hellip;", "&#8230;" }, { "&laquo;", "&#171;" }, { "&raquo;", "&#187;" }, { "&copy;", "&#169;" },
        { "&auml;", "&#228;" }, { "&ouml;", "&#246;" }, { "&uuml;", "&#252;" }, { "&Auml;", "&#196;" },
        { "&Ouml;", "&#214;" }, { "&Uuml;", "&#220;" }, { "&szlig;", "&#223;" }, { "&eacute;", "&#233;" },
    };
    for (const auto &[name, number] : entities)
        xml.replace(name, number);
    // Drop the DOCTYPE: it may refer to a DTD the reader cannot load.
    static const QRegularExpression doctype(QStringLiteral("<!DOCTYPE[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    return QString::fromUtf8(xml).remove(doctype).toUtf8();
}

bool isFont(const QString &mediaType, const QString &path)
{
    const QString lower = path.toLower();
    return mediaType.startsWith(QLatin1String("font/")) || mediaType.contains(QLatin1String("font"))
        || mediaType == QLatin1String("application/vnd.ms-opentype") || lower.endsWith(QLatin1String(".ttf"))
        || lower.endsWith(QLatin1String(".otf")) || lower.endsWith(QLatin1String(".woff"));
}

} // namespace

QString decodeText(const QByteArray &data)
{
    if (auto encoding = QStringConverter::encodingForData(data)) {
        QStringDecoder decoder(*encoding);
        return decoder.decode(data);
    }
    static const QRegularExpression declaration(QStringLiteral("^\\s*<\\?xml[^>]*encoding\\s*=\\s*[\"']([^\"']+)[\"']"));
    const QRegularExpressionMatch match = declaration.match(QString::fromLatin1(data.left(200)));
    if (match.hasMatch()) {
        QStringDecoder decoder(match.captured(1).toLatin1().constData());
        if (decoder.isValid())
            return decoder.decode(data);
    }
    return QString::fromUtf8(data);
}

EpubBook::EpubBook() = default;
EpubBook::~EpubBook() = default;

QByteArray EpubBook::file(const QString &path) const
{
    if (!m_zip || path.isEmpty())
        return QByteArray();
    const QString name = m_namesByLowerCase.value(path.toLower(), path);
    return m_zip->fileData(name);
}

bool EpubBook::open(const QString &filePath, QString *error)
{
    const auto fail = [error](const QString &reason) {
        if (error)
            *error = reason;
        return false;
    };

    m_zip = std::make_unique<QZipReader>(filePath);
    if (!m_zip->isReadable() || m_zip->status() != QZipReader::NoError)
        return fail(QStringLiteral("not a readable EPUB (ZIP) file"));
    m_namesByLowerCase.clear();
    for (const QZipReader::FileInfo &info : m_zip->fileInfoList())
        m_namesByLowerCase.insert(info.filePath.toLower(), info.filePath);

    // container.xml names the package document (OPF).
    QString opfPath;
    QXmlStreamReader container(file(QStringLiteral("META-INF/container.xml")));
    while (!container.atEnd()) {
        if (container.readNextStartElement() && container.name() == QLatin1String("rootfile")) {
            opfPath = container.attributes().value(QStringLiteral("full-path")).toString();
            break;
        }
    }
    if (opfPath.isEmpty())
        return fail(QStringLiteral("EPUB without package document"));

    QHash<QString, ManifestItem> manifest;
    QStringList spineIds;
    QString ncxId;
    QXmlStreamReader opf(withNumericEntities(file(opfPath)));
    while (!opf.atEnd()) {
        if (!opf.readNextStartElement())
            continue;
        const QStringView name = opf.name();
        const QXmlStreamAttributes attributes = opf.attributes();
        if (name == QLatin1String("title") && m_title.isEmpty()) {
            m_title = opf.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        } else if (name == QLatin1String("item")) {
            ManifestItem item;
            item.path = resolvePath(opfPath, attributes.value(QStringLiteral("href")).toString());
            item.mediaType = attributes.value(QStringLiteral("media-type")).toString();
            item.properties = attributes.value(QStringLiteral("properties")).toString();
            manifest.insert(attributes.value(QStringLiteral("id")).toString(), item);
        } else if (name == QLatin1String("spine")) {
            ncxId = attributes.value(QStringLiteral("toc")).toString();
        } else if (name == QLatin1String("itemref")) {
            if (attributes.value(QStringLiteral("linear")) != QLatin1String("no"))
                spineIds.append(attributes.value(QStringLiteral("idref")).toString());
        }
    }

    m_chapters.clear();
    m_weights.clear();
    for (const QString &id : std::as_const(spineIds)) {
        const ManifestItem item = manifest.value(id);
        if (item.path.isEmpty())
            continue;
        m_chapters.append(item.path);
        m_weights.append(textLength(decodeText(file(item.path))));
    }
    if (m_chapters.isEmpty())
        return fail(QStringLiteral("EPUB without chapters"));

    QString navId;
    m_fontPaths.clear();
    for (auto it = manifest.cbegin(); it != manifest.cend(); ++it) {
        if (it->properties.split(u' ').contains(QStringLiteral("nav")))
            navId = it.key();
        if (ncxId.isEmpty() && it->mediaType == QLatin1String("application/x-dtbncx+xml"))
            ncxId = it.key();
        if (isFont(it->mediaType, it->path))
            m_fontPaths.append(it->path);
    }
    readContents(manifest, navId, ncxId);
    return true;
}

void EpubBook::readContents(const QHash<QString, ManifestItem> &manifest, const QString &navId, const QString &ncxId)
{
    m_contents.clear();
    if (!navId.isEmpty())
        readNav(manifest.value(navId).path);
    if (m_contents.isEmpty() && !ncxId.isEmpty())
        readNcx(manifest.value(ncxId).path);
}

void EpubBook::addContentsEntry(const QString &title, int level, const QString &basePath, const QString &href)
{
    if (title.isEmpty() || href.isEmpty())
        return;
    ContentsEntry entry;
    entry.title = title;
    entry.level = std::max(0, level);
    entry.chapter = chapterOf(resolvePath(basePath, href));
    entry.fragment = fragmentOf(href);
    if (entry.chapter >= 0)
        m_contents.append(entry);
}

// EPUB 3: <nav epub:type="toc"><ol><li><a href="…">Title</a><ol>…</ol></li></ol></nav>
void EpubBook::readNav(const QString &navPath)
{
    QXmlStreamReader xml(withNumericEntities(file(navPath)));
    bool inToc = false;
    bool tocFound = false;
    int level = -1;
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QStringView name = xml.name();
            if (name == QLatin1String("nav") && !tocFound) {
                const QString type = xml.attributes().value(QStringLiteral("epub:type")).toString()
                                     + xml.attributes().value(QStringLiteral("type")).toString();
                inToc = type.contains(QLatin1String("toc"));
            } else if (inToc && name == QLatin1String("ol")) {
                ++level;
            } else if (inToc && name == QLatin1String("a")) {
                const QString href = xml.attributes().value(QStringLiteral("href")).toString();
                const QString title = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
                addContentsEntry(title, level, navPath, href);
            }
        } else if (xml.isEndElement() && inToc) {
            if (xml.name() == QLatin1String("ol")) {
                --level;
            } else if (xml.name() == QLatin1String("nav")) {
                inToc = false;
                tocFound = true;
            }
        }
    }
}

// EPUB 2: <navMap><navPoint><navLabel><text>Title</text></navLabel><content src="…"/>…</navPoint></navMap>
void EpubBook::readNcx(const QString &ncxPath)
{
    QXmlStreamReader xml(withNumericEntities(file(ncxPath)));
    int level = -1;
    QString title;
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QStringView name = xml.name();
            if (name == QLatin1String("navPoint")) {
                ++level;
                title.clear();
            } else if (name == QLatin1String("text") && title.isEmpty()) {
                title = xml.readElementText().simplified();
            } else if (name == QLatin1String("content")) {
                addContentsEntry(title, level, ncxPath, xml.attributes().value(QStringLiteral("src")).toString());
            }
        } else if (xml.isEndElement() && xml.name() == QLatin1String("navPoint")) {
            --level;
        }
    }
}

QString EpubBook::chapterText(int chapter) const
{
    return chapter >= 0 && chapter < m_chapters.size() ? decodeText(file(m_chapters.at(chapter))) : QString();
}

QString EpubBook::chapterPath(int chapter) const
{
    return m_chapters.value(chapter);
}

QByteArray EpubBook::resource(const QString &path) const
{
    return file(path);
}

QList<QByteArray> EpubBook::fonts() const
{
    QList<QByteArray> fonts;
    for (const QString &path : m_fontPaths) {
        const QByteArray data = file(path);
        if (!data.isEmpty())
            fonts.append(data);
    }
    return fonts;
}

qint64 EpubBook::chapterWeight(int chapter) const
{
    return m_weights.value(chapter, 1);
}
