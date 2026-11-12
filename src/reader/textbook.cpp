#include "textbook.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include "epubbook.h" // decodeText()

bool TextBook::isTextBook(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QLatin1String("txt") || suffix == QLatin1String("md") || suffix == QLatin1String("markdown");
}

bool TextBook::open(const QString &filePath, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("cannot read %1").arg(filePath);
        return false;
    }
    const QFileInfo info(filePath);
    m_folder = info.absolutePath();
    m_fileName = info.fileName();
    m_title = info.completeBaseName();
    m_text = decodeText(file.readAll());
    m_format = info.suffix().compare(QLatin1String("txt"), Qt::CaseInsensitive) == 0 ? TextFormat::PlainText
                                                                                      : TextFormat::Markdown;
    m_contents.clear();
    if (m_format == TextFormat::Markdown) {
        // "# Title" and "## Chapter" (not inside code blocks).
        static const QRegularExpression heading(QStringLiteral("^(#{1,2})\\s+(.+?)\\s*#*\\s*$"));
        bool code = false;
        for (const QString &line : m_text.split(u'\n')) {
            if (line.trimmed().startsWith(QLatin1String("```"))) {
                code = !code;
                continue;
            }
            const QRegularExpressionMatch match = heading.match(line);
            if (!code && match.hasMatch()) {
                ContentsEntry entry;
                entry.title = match.captured(2);
                entry.level = int(match.captured(1).size()) - 1;
                entry.chapter = 0;
                entry.fragment = QStringLiteral("heading:") + entry.title; // see BookLayout::offsetOfFragment()
                m_contents.append(entry);
            }
        }
    }
    return true;
}

QByteArray TextBook::resource(const QString &path) const
{
    // Only files in the folder of the book and below it.
    const QString absolute = QDir::cleanPath(QDir(m_folder).filePath(path));
    if (!absolute.startsWith(QDir::cleanPath(m_folder) + u'/', Qt::CaseInsensitive))
        return QByteArray();
    QFile file(absolute);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
