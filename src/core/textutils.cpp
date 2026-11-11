#include "textutils.h"

namespace TextUtils {

QString foldForSearch(const QString &text)
{
    // Decompose ("ä" -> "a" + combining diaeresis) and drop the combining marks.
    const QString decomposed = text.normalized(QString::NormalizationForm_KD);
    QString folded;
    folded.reserve(decomposed.size());
    for (const QChar c : decomposed) {
        if (c.category() != QChar::Mark_NonSpacing)
            folded.append(c);
    }
    // "ß" has no decomposition; fold it like a search for "ss" would expect.
    folded.replace(QChar(0x00DF), QStringLiteral("ss"));
    return folded.toCaseFolded();
}

QStringList searchTerms(const QString &text)
{
    return foldForSearch(text).split(u' ', Qt::SkipEmptyParts);
}

QString nameSortKey(const QString &name)
{
    const QStringList parts = name.split(u' ', Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.last();
}

} // namespace TextUtils
