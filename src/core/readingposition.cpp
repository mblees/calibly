#include "readingposition.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

QString ReadingPosition::toString() const
{
    if (!isValid())
        return QString();
    QJsonObject object;
    if (chapter >= 0) {
        object.insert(QStringLiteral("chapter"), chapter);
        object.insert(QStringLiteral("offset"), offset);
    }
    if (page >= 0)
        object.insert(QStringLiteral("page"), page);
    if (!label.isEmpty())
        object.insert(QStringLiteral("label"), label);
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

ReadingPosition ReadingPosition::fromString(const QString &text)
{
    ReadingPosition position;
    if (text.isEmpty())
        return position;
    const QJsonObject object = QJsonDocument::fromJson(text.toUtf8()).object();
    position.chapter = object.value(QStringLiteral("chapter")).toInt(-1);
    position.offset = std::max(0, object.value(QStringLiteral("offset")).toInt(0));
    position.page = object.value(QStringLiteral("page")).toInt(-1);
    position.label = object.value(QStringLiteral("label")).toString();
    return position;
}
