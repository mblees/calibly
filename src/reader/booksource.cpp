// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "booksource.h"

#include <QRegularExpression>
#include <QUrl>

BookSource::TextFormat BookSource::chapterFormat(int) const
{
    return TextFormat::Html;
}

QByteArray BookSource::resource(const QString &) const
{
    return QByteArray();
}

int BookSource::chapterOf(const QString &path) const
{
    for (int i = 0; i < chapterCount(); ++i) {
        if (chapterPath(i).compare(path, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

QString BookSource::resolvePath(const QString &basePath, const QString &href)
{
    QString target = href;
    target = target.section(u'#', 0, 0).section(u'?', 0, 0);
    target = QUrl::fromPercentEncoding(target.toUtf8());
    if (target.isEmpty())
        return basePath; // a link inside the same file

    QStringList parts;
    if (!target.startsWith(u'/')) {
        parts = basePath.split(u'/', Qt::SkipEmptyParts);
        if (!parts.isEmpty())
            parts.removeLast(); // the file name of the base
    }
    for (const QString &part : target.split(u'/', Qt::SkipEmptyParts)) {
        if (part == QLatin1String("."))
            continue;
        if (part == QLatin1String("..")) {
            if (!parts.isEmpty())
                parts.removeLast();
            continue;
        }
        parts.append(part);
    }
    return parts.join(u'/');
}

QString BookSource::fragmentOf(const QString &href)
{
    const qsizetype hash = href.indexOf(u'#');
    return hash < 0 ? QString() : QUrl::fromPercentEncoding(href.mid(hash + 1).toUtf8());
}

qint64 BookSource::textLength(const QString &html)
{
    static const QRegularExpression head(QStringLiteral("<head\\b.*?</head>"),
                                         QRegularExpression::CaseInsensitiveOption
                                             | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression tags(QStringLiteral("<[^>]*>"));
    static const QRegularExpression entities(QStringLiteral("&#?\\w+;"));
    static const QRegularExpression spaces(QStringLiteral("\\s+"));
    QString text = html;
    text.remove(head);
    text.remove(tags);
    text.replace(entities, QStringLiteral("x"));
    text.replace(spaces, QStringLiteral(" "));
    return std::max<qint64>(1, text.trimmed().size());
}
