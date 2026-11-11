#include "filenameparser.h"

#include <QRegularExpression>

FileNameInfo parseFileName(const QString &baseName)
{
    // Optional "CODE NN - " prefix, then "Title - Author". The author part
    // contains no " - ", so titles may contain hyphens ("Nemesis-Spiele").
    static const QRegularExpression pattern(QStringLiteral(
        R"(^(?:(?<code>[\p{L}\d]{1,8})\s+(?<index>\d{1,4})\s+-\s+)?(?<title>.+?)(?:\s+-\s+(?<author>[^-]+?))?\s*$)"));

    FileNameInfo info;
    const QString name = QString(baseName).replace(u'_', u' ').simplified();
    const QRegularExpressionMatch match = pattern.match(name);
    if (!match.hasMatch()) {
        info.title = name;
        return info;
    }
    info.seriesCode = match.captured(QStringLiteral("code"));
    info.seriesIndex = match.captured(QStringLiteral("index")).toInt();
    info.title = match.captured(QStringLiteral("title")).trimmed();
    info.author = match.captured(QStringLiteral("author")).trimmed();
    return info;
}
